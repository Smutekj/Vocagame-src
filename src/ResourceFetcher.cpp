#include "ResourceFetcher.h"

#include "SoundSystem.h"

#include <filesystem>
#include <fstream>

#include "Components.h"
#include "serializers.h"
#if defined(__EMSCRIPTEN__)
#include <emscripten/html5.h>
#include <emscripten/fetch.h>
#endif
#include "Utils/IOUtils.h"

ResourceFetcher::ResourceFetcher(nlohmann::json meta_data, Assets &assets, int to_fetch_count, bool local)
    : m_id(meta_data.at("id")),
      m_url(meta_data.value("path", "")),
      m_to_fetch_count(to_fetch_count),
      m_is_local(local),
      m_meta_data(meta_data),
      m_assets_holder(assets)
{
}
ResourceFetcher::~ResourceFetcher() {};

#if defined(__EMSCRIPTEN__)
void onLoadSuccess2(emscripten_fetch_t *fetch)
{
    ResourceFetcher &resource_info = *(ResourceFetcher *)fetch->userData;
    try
    {
        resource_info.initialize((const unsigned char *)fetch->data, fetch->numBytes);
    }
    catch (std::exception &e)
    {
        // std::cout << "Failed to fetch resource: " << resource_info.m_id << " " << e.what() << std::endl;
    }

    emscripten_fetch_close(fetch); // Free data associated with the fetch.
}
#endif

void ResourceFetcher::doFetch()
{
    if (m_is_local)
    {
        std::string local_path = m_url;
        std::filesystem::path url = std::string{RESOURCES_DIR} + local_path;
        auto file_bytes = utils::loadBuffer(url.c_str());
        initialize((const unsigned char *)file_bytes.data(), file_bytes.size());
    }
    else
    {
#if defined(EMSCRIPTEN)
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        attr.userData = (void *)(this);
        strcpy(attr.requestMethod, "GET");
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_PERSIST_FILE;
        attr.onsuccess = onLoadSuccess2;
        attr.onerror = [](emscripten_fetch_t *fetch)
        {
            // std::cout << "ASSET " << " not loaded! " << std::endl;
            emscripten_fetch_close(fetch);
        };
        emscripten_fetch(&attr, m_url.c_str());
#endif
    }
}

void ResourceFetcher::initialize(const unsigned char *data, std::size_t num_bytes)
{
    initializeImpl(data, num_bytes);
    m_on_load_success(data, num_bytes);
    m_loaded = true;
    m_to_fetch_count--;
}

bool ResourceFetcher::isLoaded() const
{
    return m_to_fetch_count == 0;
}

FontFetcher::FontFetcher(nlohmann::json meta_data, Assets &assets, bool local)
    : ResourceFetcher(meta_data, assets, 1, local) {}

void FontFetcher::initializeImpl(const unsigned char *data, std::size_t num_bytes)
{
    std::string font_name = m_meta_data["id"];
    int font_size = m_meta_data["size"];
    m_assets_holder.fonts[font_name] = std::make_shared<::Font>(data, num_bytes, font_size);
}

TextureFetcher::TextureFetcher(nlohmann::json meta_data, Assets &assets, bool local)
    : ResourceFetcher(meta_data, assets, 1, local) {}

void TextureFetcher::initializeImpl(const unsigned char *data, std::size_t num_bytes)
{
    auto options = deserialize<TextureOptions>(m_meta_data["options"]);
    std::string id = m_meta_data["id"];
    m_assets_holder.textures.add(m_id, data, num_bytes, options);
}

JsonFetcher::JsonFetcher(nlohmann::json meta_data, Assets &assets, bool local)
    : ResourceFetcher(meta_data, assets, 1, local) {}

void JsonFetcher::initializeImpl(const unsigned char *data, std::size_t num_bytes)
{

    // m_assets_holder.textures.add(id + "-atlas", data, num_bytes);
}
SoundFetcher::SoundFetcher(nlohmann::json meta_data, Assets &assets, bool local)
    : ResourceFetcher(meta_data, assets, 1, local) {}

void SoundFetcher::initializeImpl(const unsigned char *data, std::size_t num_bytes)
{
    SoundSystem::registerSound(m_id, data, num_bytes);
}


TextureAtlasFetcher::TextureAtlasFetcher(nlohmann::json meta_data, Assets &assets, bool local)
: ResourceFetcher(meta_data, assets, 2, local) {}

TextureAtlasFetcher::~TextureAtlasFetcher() {}
void TextureAtlasFetcher::initializeImpl(const unsigned char *data, std::size_t num_bytes)
{
    std::string id = m_meta_data["id"];
    m_assets_holder.textures.add(id + "-atlas", data, num_bytes);

    ResourceFetcher *json_fetcher = new JsonFetcher{m_meta_data, m_assets_holder, m_is_local};
    json_fetcher->m_id = m_id;
    json_fetcher->m_url = m_meta_data["json_path"].get<std::string>();
    json_fetcher->m_on_load_success = [this, json_fetcher, id = m_id](const unsigned char *data, std::size_t num_bytes)
    {
        nlohmann::json atlas_data = nlohmann::json::parse(data, data + num_bytes);
        try
        {
            m_assets_holder.atlases.addAtlas(m_assets_holder.textures.get(id + "-atlas"), atlas_data);
            m_assets_holder.atlases2[id] = {};
            m_assets_holder.atlases2[id].addAtlas(id, m_assets_holder.textures.get(id + "-atlas"), atlas_data); 
            
            // std::cout << "Atlas " << id << " success!" << std::endl;
        }
        catch (std::exception &e)
        {
            // std::cout << e.what() << std::endl;
        }
        m_to_fetch_count--;
        // delete json_fetcher;
    };
    json_fetcher->doFetch();
}
AnimationFetcher::AnimationFetcher(nlohmann::json meta_data, Assets &assets, bool local)
: ResourceFetcher(meta_data, assets, 2, local) {}

AnimationFetcher::~AnimationFetcher() {}
void AnimationFetcher::initializeImpl(const unsigned char *data, std::size_t num_bytes)
{
    std::string id = m_meta_data["id"];
    m_assets_holder.textures.add(id + "-atlas", data, num_bytes);

    ResourceFetcher *json_fetcher = new JsonFetcher{m_meta_data, m_assets_holder, false};
    json_fetcher->m_id = m_id;
    json_fetcher->m_url = m_meta_data["json_path"].get<std::string>();
    json_fetcher->m_on_load_success = [this, json_fetcher, id = m_id](const unsigned char *data, std::size_t num_bytes)
    {
        nlohmann::json atlas_data = nlohmann::json::parse(data, data + num_bytes);
        try
        {
            m_assets_holder.atlases.addAtlas(m_assets_holder.textures.get(id + "-atlas"), atlas_data);
            // std::cout << "Atlas " << id << " success!" << std::endl;
        }
        catch (std::exception &e)
        {
            // std::cout << e.what() << std::endl;
        }
        m_to_fetch_count--;
        // delete json_fetcher;
    };
    json_fetcher->doFetch();
}
std::shared_ptr<ResourceFetcher> makeFetcher(const std::string &type_name,
                                             nlohmann::json meta_data,
                                             Assets &assets,
                                             bool local)
{
    if (type_name == "Font")
    {
        return std::make_shared<FontFetcher>(meta_data, assets, local);
    }
    else if (type_name == "Sound")
    {
        return std::make_shared<SoundFetcher>(meta_data, assets, local);
    }
    else if (type_name == "Texture")
    {
        return std::make_shared<TextureFetcher>(meta_data, assets, local);
    }
    else if (type_name == "Atlas")
    {
        return std::make_shared<TextureAtlasFetcher>(meta_data, assets, local);
    }
    else if (type_name == "Animation")
    {
        return std::make_shared<AnimationFetcher>(meta_data, assets, local);
    }
    return nullptr;
}