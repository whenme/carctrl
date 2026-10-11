// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <xapi/easylog.hpp>

using Json = nlohmann::json;

class ParamJson
{
public:
    ParamJson(std::string fileName);
    virtual ~ParamJson() = default;

    /**
     * @brief parse command options that overwrite json values
     *
     * Option form: -c +/path/item=value
     * Example: -c +/param/param0=5 overwrites /param/param0 from the json file.
     * Call this before constructing ParamJson. Repeat -c to overwrite more items.
     */
    static void parseCmdOption(int argc, char **argv);

    /**
     * @brief get parameters from json item
     *
     * @param item: json item name with slash seperated, chip/gpu_param/device_id
     * @param param: output value, vector or basic type.
     * @return true: success to get parameters
     *         false: failed to get parameters for item not exist or output type incorrect.
     */
    [[nodiscard]] bool getJsonParam(std::string item, auto& param)
    {
        std::vector<std::string> name;
        int32_t count = splitStr(item, '/', name);
        if ((count <= 0) || (m_jsonState == false))
            return false;

        Json obj;
        try {
            obj = m_jsonObj.at(name[0]);
            for (int32_t i = 1; i < count; i++) {
                obj = obj.at(name[i]);
            }
        }
        catch (std::exception &) {
            apilog::warn("no json item for {}", item);
            return false;
        }

        try {
            obj.get_to(param);
        }
        catch (std::exception &) {
            apilog::warn("json param or output type error for {}", item);
            return false;
        }

        return true;
    }

private:
    int32_t splitStr(std::string strSrc, char splitChar, std::vector<std::string>& output);
    void applyCmdOption();

    static std::vector<std::string> m_cmdOption;
    Json m_jsonObj;
    bool m_jsonState {false};
};
