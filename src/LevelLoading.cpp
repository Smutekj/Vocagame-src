#include "LevelLoading.h"

#include "serializers.h"
#include "Utils/IOUtils.h"


using json = nlohmann::json;

CaveSpec2 readCaveSpec(const json &lvl_data)
{
    CaveSpec2 spec;

    if (lvl_data.contains("named_points"))
    {
        auto points = lvl_data.at("named_points");
        if(points.contains("start"))
        {
            spec.start = {points.at("start")[0], points.at("start")[1]};
        }
        if(points.contains("end"))
        {
            spec.end = {points.at("end")[0], points.at("end")[1]};
        }
    }
    if(lvl_data.contains("origin"))
    {
        spec.origin = {lvl_data["origin"][0].get<float>(), lvl_data["origin"][1].get<float>()};
    }
    if(lvl_data.contains("level_size"))
    {
        spec.level_size = {lvl_data["level_size"][0].get<float>(), lvl_data["level_size"][1].get<float>()};
    }
    spec.bounds = {lvl_data["bounds"][0].get<float>(),
                   lvl_data["bounds"][1].get<float>(),
                   lvl_data["bounds"][2].get<float>(),
                   lvl_data["bounds"][3].get<float>()};
    for (auto &spec_json : lvl_data.at("objects"))
    {
        std::string type_name = spec_json["obj_type"];
        try
        {
            auto type_id = stringToEnum<ObjectType>(type_name);
            auto new_spec = deserializeSpec(type_id, spec_json);
            if (new_spec)
            {
                spec.objects.push_back(new_spec);
            }
        }
        catch (std::exception &e)
        {
            // std::cout << "Enum does not exist: " << e.what() << std::endl;
        }
    }
    return spec;
}
std::unordered_map<std::string, CaveSpec2> readCaveSpecs3(const std::filesystem::path &json_file_path)
{
    auto cave_data = utils::loadJson(json_file_path.c_str());

    std::unordered_map<std::string, CaveSpec2> specs;
    for (auto &[key, value] : cave_data.items())
    {
        specs[key] = readCaveSpec(value);
    }

    return specs;
}

std::vector<CaveSpec2> readCaveSpecs2(const std::filesystem::path &json_file_path)
{
    auto cave_data = utils::loadJson(json_file_path.c_str());

    std::vector<CaveSpec2> specs;
    for (auto &[key, value] : cave_data.items())
    {
        specs.push_back(readCaveSpec(value));
    }

    return specs;
}