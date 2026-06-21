// SPDX-License-Identifier: GPL-2.0

#include <video/stereo_object_match.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

#include <xapi/easylog.hpp>

namespace {

/** PASCAL VOC 20 + background, same order as MobileNet-SSD Caffe deploy. */
constexpr const char* k_class_cn[] = {
    "背景",
    "飞机",
    "自行车",
    "鸟",
    "船",
    "瓶子",
    "巴士",
    "汽车",
    "猫",
    "椅子",
    "牛",
    "餐桌",
    "狗",
    "马",
    "摩托车",
    "人",
    "盆栽植物",
    "羊",
    "沙发",
    "火车",
    "显示器",
};

constexpr int k_num_classes = static_cast<int>(sizeof(k_class_cn) / sizeof(k_class_cn[0]));

struct Detection
{
    int          cls {};
    float        conf {};
    cv::Rect2f   box_norm {};  // 0..1 relative to 300x300 input
};

std::vector<Detection> run_mobilenet_ssd(cv::dnn::Net& net, const cv::Mat& bgr, float conf_thresh)
{
    std::vector<Detection> out;
    if (bgr.empty()) {
        return out;
    }

    cv::Mat blob = cv::dnn::blobFromImage(bgr, 0.007843, cv::Size(300, 300), cv::Scalar(127.5, 127.5, 127.5), true, false);
    net.setInput(blob);
    cv::Mat raw = net.forward();
    if (raw.empty()) {
        return out;
    }

    cv::Mat det;
    if (raw.dims == 4 && raw.size[3] == 7) {
        const int n = raw.size[2];
        det = cv::Mat(n, 7, CV_32F, raw.ptr<float>());
    } else if (raw.dims == 2 && raw.cols == 7) {
        det = raw;
    } else {
        return out;
    }

    for (int i = 0; i < det.rows; ++i) {
        const float conf = det.at<float>(i, 2);
        if (conf < conf_thresh) {
            continue;
        }
        const int cls = static_cast<int>(det.at<float>(i, 1));
        if (cls < 0 || cls >= k_num_classes || cls == 0) {
            continue;  // skip background
        }

        Detection d;
        d.cls = cls;
        d.conf = conf;
        d.box_norm.x = det.at<float>(i, 3);
        d.box_norm.y = det.at<float>(i, 4);
        d.box_norm.width = det.at<float>(i, 5) - det.at<float>(i, 3);
        d.box_norm.height = det.at<float>(i, 6) - det.at<float>(i, 4);
        out.push_back(d);
    }
    return out;
}

/** Map normalized SSD box to pixel rect on full image (same aspect as blob resize). */
cv::Rect2f normBoxToImage(const Detection& d, int img_w, int img_h)
{
    const float sx = static_cast<float>(img_w);
    const float sy = static_cast<float>(img_h);
    return {d.box_norm.x * sx, d.box_norm.y * sy, d.box_norm.width * sx, d.box_norm.height * sy};
}

float vertical_overlap_ratio(const cv::Rect2f& a, const cv::Rect2f& b)
{
    const float ay1 = a.y;
    const float ay2 = a.y + a.height;
    const float by1 = b.y;
    const float by2 = b.y + b.height;
    const float inter = std::max(0.f, std::min(ay2, by2) - std::max(ay1, by1));
    const float union_h = std::max(ay2, by2) - std::min(ay1, by1);
    if (union_h <= 0.f) {
        return 0.f;
    }
    return inter / union_h;
}

}  // namespace

StereoObjectMatch::StereoObjectMatch()
{
    tryLoadNet();
    m_lastLog = std::chrono::steady_clock::now();
}

bool StereoObjectMatch::tryLoadNet()
{
    const char* env_proto = std::getenv("CARCTRL_SSD_PROTO");
    const char* env_model = std::getenv("CARCTRL_SSD_MODEL");
    const std::string proto = env_proto ? env_proto : "MobileNetSSD_deploy.prototxt";
    const std::string model = env_model ? env_model : "MobileNetSSD_deploy.caffemodel";

    try {
        m_net = cv::dnn::readNetFromCaffe(proto, model);
    } catch (const cv::Exception& e) {
        ctrllog::warn("stereo-object: failed to load SSD model ({}): {}", proto, e.what());
        m_net = cv::dnn::Net();
    }

    if (m_net.empty()) {
        return false;
    }

#if CV_VERSION_MAJOR >= 4
    m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
#endif
    m_ready = true;
    ctrllog::info("stereo-object: MobileNet-SSD loaded (proto={}, weights={})", proto, model);
    return true;
}

void StereoObjectMatch::tryDetectAndLog(const cv::Mat& rectifiedLeftBgr, const cv::Mat& rectifiedRightBgr)
{
    if (!m_ready) {
        if (!m_warned) {
            m_warned = true;
            ctrllog::warn(
                "stereo-object: SSD not loaded; place MobileNetSSD_deploy.prototxt and "
                "MobileNetSSD_deploy.caffemodel in cwd or set CARCTRL_SSD_PROTO / CARCTRL_SSD_MODEL");
        }
        return;
    }

    ++m_frameCounter;
    if ((m_frameCounter % 12) != 0) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (std::chrono::duration_cast<std::chrono::seconds>(now - m_lastLog).count() < 2) {
        return;
    }
    m_lastLog = now;

    const std::vector<Detection> left_dets = run_mobilenet_ssd(m_net, rectifiedLeftBgr, 0.35f);
    const std::vector<Detection> right_dets = run_mobilenet_ssd(m_net, rectifiedRightBgr, 0.35f);

    const int w = rectifiedLeftBgr.cols;
    const int h = rectifiedLeftBgr.rows;
    if (w <= 0 || h <= 0) {
        return;
    }

    std::set<int> matched_classes;

    for (const auto& dl : left_dets) {
        const cv::Rect2f boxL = normBoxToImage(dl, w, h);
        const float cxL = boxL.x + boxL.width * 0.5f;
        const float cyL = boxL.y + boxL.height * 0.5f;

        for (const auto& dr : right_dets) {
            if (dr.cls != dl.cls) {
                continue;
            }

            const cv::Rect2f boxR = normBoxToImage(dr, w, h);
            const float cxR = boxR.x + boxR.width * 0.5f;
            const float cyR = boxR.y + boxR.height * 0.5f;

            const float disp = cxL - cxR;
            if (disp < 8.f || disp > 0.55f * static_cast<float>(w)) {
                continue;
            }

            if (vertical_overlap_ratio(boxL, boxR) < 0.35f) {
                continue;
            }

            if (std::fabs(cyL - cyR) > 0.06f * static_cast<float>(h)) {
                continue;
            }

            matched_classes.insert(dl.cls);
        }
    }

    if (matched_classes.empty()) {
        return;
    }

    std::string joined;
    for (int c : matched_classes) {
        if (c > 0 && c < k_num_classes) {
            if (!joined.empty()) {
                joined += ", ";
            }
            joined += k_class_cn[c];
        }
    }

    if (joined.empty()) {
        return;
    }

    ctrllog::info("双目共同可见物品: {}", joined);
}
