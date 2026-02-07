#include "SoundSystem.h"

#if defined(EMSCRIPTEN)
#include <emscripten.h>
EM_JS(char *, playBuffer2, (const char *buffer_id), {
    console.log(UTF8ToString(buffer_id));
    var buf = Module.soundBuffers[UTF8ToString(buffer_id)];
    var src = Module.sfxCtx.createBufferSource();
    src.buffer = buf;
    src.connect(Module.sfxCtx.destination);
    src.start();
});
#endif

SoundSystem *SoundSystem::p_instance = nullptr;

void SoundSystem::play(const std::string &id, float distance)
{
#if defined(__EMSCRIPTEN__)
    playBuffer2(id.c_str());
#else
    assert(SoundSystem::get().m_chunks.contains(id));
    if (distance < 0.f)
    {
        // std::cout << "WARNING! Cannot play sound at negative distance!" << std::endl;
        return;
    }

    float max_distance = SoundSystem::get().m_max_distance;
    distance = std::min(max_distance, distance); //! truncate the distance
    auto max_dist_i = std::numeric_limits<u_int8_t>::max();
    u_int8_t dist_i = ((distance / max_distance) * max_dist_i);

    int channel_id = Mix_PlayChannel(-1, SoundSystem::get().m_chunks.at(id), 0);
    Mix_SetDistance(channel_id, dist_i);
#endif
}

int SoundSystem::getVolume()
{
    return SoundSystem::get().volume;
}
void SoundSystem::setVolume(int volume)
{
    SoundSystem::get().volume = volume;
    
    Mix_Volume(-1, volume);
}