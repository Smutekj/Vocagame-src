#pragma once

#include <Texture.h>
#include <Font.h>

#include "TextureAtlas.h"

// #include "nlohmann/json.hpp"

struct Assets
{
    TextureHolder textures;
    std::unordered_map<std::string, std::shared_ptr<Font>> fonts;
    TextureHolder2 atlases;
    std::unordered_map<std::string, TextureHolder2> atlases2;
    // std::unordered_map<std::string, nlohmann::json> jsons;
    //! sounds...
};
