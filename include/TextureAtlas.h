#pragma once

#include <Texture.h>
#include <Sprite.h>

#include <nlohmann/json_fwd.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <filesystem>

struct TextureAtlas
{
    TextureAtlas(std::filesystem::path atlas_dir_path, std::string atlas_img_name, std::string atlas_data_name);
    TextureAtlas(std::shared_ptr<Texture> texture, const nlohmann::json &frame_data);

    Recti get(const std::string &id) const;

    Texture &getTexture();
    std::shared_ptr<Texture> getTextureP();

    const std::unordered_map<std::string, Recti> &getIds() const;

private:
    Recti parseFrame(const nlohmann::json &frame_data);

private:
    std::shared_ptr<Texture> m_atlas_texture;
    std::unordered_map<std::string, Recti> m_id2texture_rect;
    std::filesystem::path m_atlas_path;
};

//! \class TextureHolder
//! \brief holds textures based on id given by string
class TextureHolder2
{

public:
    bool addAtlas(std::filesystem::path file_path, std::string file_name);
    bool addAtlas(std::shared_ptr<Texture> texture, const nlohmann::json &atlas_data);
    bool addAtlas(std::string atlas_id, std::shared_ptr<Texture> texture, const nlohmann::json &atlas_data);

    TextureAtlas &get(const std::string &id);

    Texture &getTexture(const std::string &id);
    bool contains(const std::string &id) const;
    Recti getRect(const std::string &id) const;
    Sprite getSprite(const std::string &id);

private:
    std::vector<std::shared_ptr<TextureAtlas>> m_atlases;
    std::unordered_map<std::string, std::size_t> m_tex_id2atlas_id;
    std::unordered_map<std::string, std::size_t> m_atlas_ids;

    std::filesystem::path m_resources_path;
};
