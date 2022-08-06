
#include <ioapi/param_json.hpp>
#include <fstream>

ParamJson::ParamJson(std::string& fileName) :
    m_jsonState(false)
{
    jsonParse(fileName);
}

void ParamJson::jsonParse(std::string& fileName)
{
    std::ifstream jsonFile(fileName);
    if (!jsonFile.is_open())
    {
        std::cout << "open json file " << fileName << " failed..." << std::endl;
        return;
    }

    jsonFile >> m_jsonObj;
    if (m_jsonObj.is_discarded())
    {
        std::cout << "parse " << fileName << " data failed..." << std::endl;
        return;
    }

    m_jsonState = true;
}
