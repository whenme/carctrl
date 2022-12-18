// SPDX-License-Identifier: GPL-2.0

#include <ioapi/easylog.hpp>
#include <ioapi/param_json.hpp>
#include <fstream>

ParamJson::ParamJson(std::string fileName)
{
    std::ifstream jsonFile(fileName);
    if (!jsonFile.is_open()) {
        easylog::warn("open json file {} failed...", fileName);
        return;
    }

    jsonFile >> m_jsonObj;
    if (m_jsonObj.is_discarded()) {
        easylog::warn("parse {} data failed...", fileName);
        return;
    }

    m_jsonState = true;
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
