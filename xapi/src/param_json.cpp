// SPDX-License-Identifier: GPL-2.0

#include <xapi/param_json.hpp>
#include <fstream>

std::vector<std::string> ParamJson::m_cmdOption;

ParamJson::ParamJson(std::string fileName)
{
    std::ifstream jsonFile(fileName);
    if (!jsonFile.is_open()) {
        apilog::warn("open json file {} failed...", fileName);
        return;
    }

    jsonFile >> m_jsonObj;
    if (m_jsonObj.is_discarded()) {
        apilog::warn("parse {} data failed...", fileName);
        return;
    }

    m_jsonState = true;
    applyCmdOption();
}

void ParamJson::parseCmdOption(int argc, char **argv)
{
    m_cmdOption.clear();
    if (argv == nullptr)
        return;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-c") {
            if (i + 1 >= argc) {
                apilog::warn("option -c needs a value");
                break;
            }
            m_cmdOption.push_back(argv[++i]);
        } else if (arg.size() > 3 && arg.compare(0, 3, "-c+") == 0) {
            m_cmdOption.push_back(arg.substr(2));
        }
    }
}

void ParamJson::applyCmdOption()
{
    for (const auto& opt : m_cmdOption) {
        if (opt.empty() || opt[0] != '+') {
            apilog::warn("cmd option {} must start with '+'", opt);
            continue;
        }

        auto eq = opt.find('=');
        if (eq == std::string::npos || eq < 2) {
            apilog::warn("cmd option {} missing '=value'", opt);
            continue;
        }

        std::string path = opt.substr(1, eq - 1);
        std::string value = opt.substr(eq + 1);
        if (path.empty() || path[0] != '/')
            path.insert(path.begin(), '/');

        Json::json_pointer ptr;
        try {
            ptr = Json::json_pointer(path);
        }
        catch (const std::exception &) {
            apilog::warn("invalid json path {}", path);
            continue;
        }

        Json *node = nullptr;
        try {
            node = &m_jsonObj.at(ptr);
        }
        catch (const std::exception &) {
            apilog::warn("no json item for {}", path);
            continue;
        }

        Json parsed;
        bool parsedOk = true;
        try {
            parsed = Json::parse(value);
        }
        catch (const std::exception &) {
            parsedOk = false;
        }

        bool compatible = false;
        if (parsedOk) {
            compatible = (node->is_number() && parsed.is_number()) || (node->type() == parsed.type());
        }

        if (compatible) {
            *node = parsed;
        } else if (node->is_string()) {
            *node = value;
        } else {
            apilog::warn("json param type error for {}", path);
            continue;
        }

        apilog::info("override {}={}", path, node->dump());
    }
}

int32_t ParamJson::splitStr(std::string strSrc, char splitChar, std::vector<std::string>& output)
{
    if (strSrc.empty() || !splitChar)
        return -1;

    std::string strContent = strSrc;
    std::string strTemp;
    std::string::size_type begin = 0, end = 0;
    while (1) {
        end = strContent.find(splitChar, begin);
        if (end == std::string::npos) {
            strTemp = strContent.substr(begin, strContent.length());
            if (!strTemp.empty())
                output.push_back(strTemp);
            break;
        }

        strTemp = strContent.substr(begin, end-begin);
        begin = end + 1;
        output.push_back(strTemp);
    }

    return static_cast<int32_t>(output.size());
}
