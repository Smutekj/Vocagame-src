#pragma once

#include <SDL_mixer.h>

#include <unordered_map>
#include <filesystem>
#include <cassert>
#include <functional>

struct Channel
{
    int id = 0;
    u_int8_t volume = 69;
    bool playing = false;
};

class SoundSystem
{
    static SoundSystem *p_instance;
    static std::array<Channel, MIX_CHANNELS> m_channels;

private:
    void onChannelFinish(int channel_id)
    {
        m_channels.at(channel_id).playing = false;
    }

    SoundSystem()
    {
    }
    ~SoundSystem()
    {
        delete p_instance;
    }

public:
    static int getVolume();
    static void setVolume(int volume);

    static void setMaxDistance(float max_distance)
    {
        SoundSystem::get().m_max_distance = max_distance;
    }

    static void play(const std::string &id)
    {
        play(id, 0.);
    }
    static void play(const std::string &id, float distance);

    static std::unordered_map<std::string, Mix_Chunk *> getSounds()
    {
        return SoundSystem::get().m_chunks;
    }

    static bool registerSound(const std::string &id, const unsigned char *data, std::size_t num_bytes)
    {
        SoundSystem::get().m_chunks[id] = Mix_LoadWAV_RW(SDL_RWFromMem((void *)data, num_bytes), 0);
        if (!SoundSystem::get().m_chunks.at(id))
        {
            // printf("Failed to load sound effect id: %s!\nSDL_mixer Error: %s\n", id.c_str(), Mix_GetError());
            return false;
        }
        return true;
    };
    static bool registerSound(const std::string &id, std::filesystem::path wav_path)
    {
        SoundSystem::get().m_chunks[id] = Mix_LoadWAV((const char *)wav_path.c_str());
        if (!SoundSystem::get().m_chunks.at(id))
        {
            // printf("Failed to load sound effect at path %s\n ! SDL_mixer Error: %s\n", wav_path.c_str(), Mix_GetError());
            return false;
        }
        return true;
    }

public:
    static SoundSystem &get()
    {
        if (!p_instance)
        {
            p_instance = new SoundSystem();
            p_instance->setVolume(MIX_MAX_VOLUME/2);
        }
        return *p_instance;
    }

private:
    int volume = MIX_MAX_VOLUME/2;

    float m_max_distance = 600.f;
    std::unordered_map<std::string, Mix_Chunk *> m_chunks;
    std::unordered_map<std::string, Mix_Music *> m_songs;
};
