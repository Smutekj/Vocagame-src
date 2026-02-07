#pragma once

#include "Assets.h"
#include "nlohmann/json.hpp"
class ResourceFetcher
{

public:
    ResourceFetcher(nlohmann::json meta_data, Assets &assets, int to_fetch_count = 1, bool local = true);
    virtual ~ResourceFetcher();

    void doFetch();
    void initialize(const unsigned char *data, std::size_t num_bytes);
    bool isLoaded() const;

private:
    virtual void initializeImpl(const unsigned char *data, std::size_t num_bytes) {}

public:
    bool m_is_local = true;
    bool m_loaded = false;
    std::filesystem::path m_url;
    std::string m_id;
    std::function<void(const unsigned char *, std::size_t)> m_on_load_success =
        [](const unsigned char *data, std::size_t num_bytes) {};

    nlohmann::json m_meta_data;

    Assets &m_assets_holder;

protected:
    int m_to_fetch_count;
};

class FontFetcher : public ResourceFetcher
{
public:
    FontFetcher(nlohmann::json meta_data, Assets &assets, bool local = true);


private:
    virtual void initializeImpl(const unsigned char *data, std::size_t num_bytes) override;
};

class TextureFetcher : public ResourceFetcher
{
public:
    TextureFetcher(nlohmann::json meta_data, Assets &assets, bool local = true);
        

private:
    virtual void initializeImpl(const unsigned char *data, std::size_t num_bytes);
};

class JsonFetcher : public ResourceFetcher
{
public:
    JsonFetcher(nlohmann::json meta_data, Assets &assets, bool local = true);

private:
    virtual void initializeImpl(const unsigned char *data, std::size_t num_bytes) override;
};
class TextureAtlasFetcher : public ResourceFetcher
{
public:
    TextureAtlasFetcher(nlohmann::json meta_data, Assets &assets, bool local = true);
    virtual ~TextureAtlasFetcher();

    //! fetch json after fetching texture
    virtual void initializeImpl(const unsigned char *data, std::size_t num_bytes) override;
};

class AnimationFetcher : public ResourceFetcher
{
public:
    AnimationFetcher(nlohmann::json meta_data, Assets &assets, bool local = true);
    virtual ~AnimationFetcher();

    //! fetch json after fetching texture
    virtual void initializeImpl(const unsigned char *data, std::size_t num_bytes) override;
};

class SoundFetcher : public ResourceFetcher
{
public:
    SoundFetcher(nlohmann::json meta_data, Assets &assets, bool local = true);

private:
    virtual void initializeImpl(const unsigned char *data, std::size_t num_bytes);
};

std::shared_ptr<ResourceFetcher> makeFetcher(const std::string &type_name, nlohmann::json meta_data, Assets &assets, bool local = true);
