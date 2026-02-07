#include "TextureAtlas.h"

#include "Utils/IOUtils.h"

#include <fstream>

TextureAtlas::TextureAtlas(std::filesystem::path atlas_dir_path, std::string atlas_img_name, std::string atlas_data_name)
{
    using json = nlohmann::json;
    m_atlas_path = (atlas_dir_path.string() + atlas_data_name);
    json atlas_data = utils::loadJson(m_atlas_path.c_str());

    auto &frame_array = atlas_data.at("frames");
    for (auto &frame_item : frame_array.items())
    {
        auto &frame = frame_item.value();
        std::string id = frame.at("filename");
        m_id2texture_rect[id] = parseFrame(frame.at("frame"));
    }

    m_atlas_texture = std::make_shared<Texture>(atlas_dir_path.string() + atlas_img_name);
};

TextureAtlas::TextureAtlas(std::shared_ptr<Texture> texture, const nlohmann::json &atlas_data)
    : m_atlas_texture(texture)
{
    auto &frame_array = atlas_data.at("frames");
    for (auto &frame_item : frame_array.items())
    {
        auto &frame = frame_item.value();
        std::string id = frame.at("filename");
        m_id2texture_rect[id] = parseFrame(frame.at("frame"));
    }
};

Recti TextureAtlas::get(const std::string &id) const
{
    return m_id2texture_rect.at(id);
}

Texture &TextureAtlas::getTexture()
{
    return *m_atlas_texture;
}
std::shared_ptr<Texture> TextureAtlas::getTextureP()
{
    return m_atlas_texture;
}

Recti TextureAtlas::parseFrame(const nlohmann::json &frame_data)
{
    return {frame_data.value("x", 0), frame_data.value("y", 0), frame_data.value("w", 1), frame_data.value("h", 1)};
}

bool TextureHolder2::addAtlas(std::filesystem::path file_path, std::string file_name)
{
    m_atlases.emplace_back(std::make_shared<TextureAtlas>(file_path, file_name + ".png", file_name + ".json"));
    
    m_atlas_ids[file_name] = m_atlases.size() - 1;

    const auto &ids = m_atlases.back()->getIds();
    for (const auto &[id, rect] : ids)
    {
        m_tex_id2atlas_id[id] = m_atlases.size() - 1;
    }

    return true;
}

bool TextureHolder2::addAtlas(std::shared_ptr<Texture> texture, const nlohmann::json &atlas_data)
{
    m_atlases.emplace_back(std::make_shared<TextureAtlas>(texture, atlas_data));
    
    std::string atlas_id = atlas_data.at("meta").at("image");
    atlas_id = utils::removeFileExtension(atlas_id); 
    m_atlas_ids[atlas_id] = m_atlases.size() - 1;

    const auto &ids = m_atlases.back()->getIds();
    for (const auto &[id, rect] : ids)
    {
        m_tex_id2atlas_id[id] = m_atlases.size() - 1;
    }

    return true;
}
bool TextureHolder2::addAtlas(std::string prefix, std::shared_ptr<Texture> texture, const nlohmann::json &atlas_data)
{
    m_atlases.emplace_back(std::make_shared<TextureAtlas>(texture, atlas_data));
    
    std::string atlas_id = atlas_data.at("meta").at("image");
    atlas_id = utils::removeFileExtension(atlas_id); 
    m_atlas_ids[atlas_id] = m_atlases.size() - 1;

    const auto &ids = m_atlases.back()->getIds();
    for (const auto &[id, rect] : ids)
    {
        m_tex_id2atlas_id[prefix + "_" + id] = m_atlases.size() - 1;
    }

    return true;
}
const std::unordered_map<std::string, Recti> &TextureAtlas::getIds() const
{
    return m_id2texture_rect;
}

Texture &TextureHolder2::getTexture(const std::string &id)
{
    return m_atlases.at(m_tex_id2atlas_id.at(id))->getTexture();
}

TextureAtlas &TextureHolder2::get(const std::string &id)
{
    return *m_atlases.at(m_atlas_ids.at(id));
}

bool TextureHolder2::contains(const std::string &id) const
{
    return m_tex_id2atlas_id.contains(id);
}

Recti TextureHolder2::getRect(const std::string &id) const
{
    return m_atlases.at(m_tex_id2atlas_id.at(id))->get(id);
}

Sprite TextureHolder2::getSprite(const std::string &id)
{
    Sprite new_sprite(getTexture(id));
    new_sprite.m_tex_rect = getRect(id);
    return new_sprite;
}