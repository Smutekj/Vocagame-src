#pragma once

#include <filesystem>
#include <unordered_map>
#include "nlohmann/json_fwd.hpp"

#include "GameObjectSpec.h"
#include <Rect.h>

struct CaveSpec2 : public GameObjectSpec
{

    int type;

    Rectf bounds;
    utils::Vector2f level_size;
    utils::Vector2f origin = {0,0};
    utils::Vector2f start = {0,0};
    utils::Vector2f end = {0,0};

    std::vector<std::shared_ptr<GameObjectSpec>> objects;
};

std::unordered_map<std::string, CaveSpec2> readCaveSpecs3(const std::filesystem::path &json_file_path);
std::vector<CaveSpec2> readCaveSpecs2(const std::filesystem::path &json_file_path);

