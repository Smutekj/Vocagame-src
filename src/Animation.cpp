#include "Animation.h"

#include <nlohmann/json.hpp>
#include <fstream>

#include "TextureAtlas.h"
#include "IOUtils.h"

AnimationSystem::AnimationSystem(utils::ContiguousColony<AnimationComponent, int> &comps, std::filesystem::path animations_texture_dir, std::filesystem::path animations_data_dir)
    : m_components(comps), m_animations_data_dir(animations_data_dir)
{
    m_atlases.setBaseDirectory(animations_texture_dir);
}

using json = nlohmann::json;

void AnimationSystem::registerAnimation(TextureAtlas &atlas, const std::string &id)
{
    const auto &frame_id2texrect = atlas.getIds();

    int min_num = std::numeric_limits<int>::max();
    std::vector<int> frame_nums;

    for (const auto &[filename, texrect] : frame_id2texrect)
    {
        auto id_pos = filename.find_last_not_of("0123456789");

        if (id_pos == std::string::npos)
        {
            id_pos = 0;
        }
        else
        {
            id_pos++; //! the first number is one to the right
        }
        frame_nums.push_back(std::stoi(filename.substr(id_pos)));
        min_num = std::min(frame_nums.back(), min_num);
    }
    std::for_each(frame_nums.begin(), frame_nums.end(), [min_num](auto &num)
                  { num -= min_num; });

    m_frame_data[id] = {atlas.getTextureP(), {}};

    //! add texrect info
    auto &rects = m_frame_data.at(id).tex_rects;
    rects.resize(frame_nums.size());
    int i = 0;
    for (auto &[frame_id, rect] : frame_id2texrect)
    {
        rects[frame_nums[i]] = rect;
        i++;
    }
}

void AnimationSystem::update(float dt)
{
    for (auto &comp : m_components.data)
    {
        comp.tex_rect = m_frame_data.at(comp.id).tex_rects.at(comp.current_frame_id);
        comp.texture_size = m_frame_data.at(comp.id).p_texture->getSize();
        comp.time += dt;
        float frame_duration = comp.cycle_duration / m_frame_data.at(comp.id).tex_rects.size();
        if (comp.time > frame_duration)
        {
            comp.time = 0.;
            comp.tex_rect = getNextFrame(comp);
            if (comp.current_frame_id == 0)
            {
                comp.n_repeats_left--;
            }
        }
    }
}

Rect<int> AnimationSystem::getNextFrame(AnimationComponent &comp)
{
    auto &frames = m_frame_data.at(comp.id).tex_rects;
    auto frame_count = frames.size();
    comp.texture_size = m_frame_data.at(comp.id).p_texture->getSize();
    comp.texture_id = m_frame_data.at(comp.id).p_texture->getHandle();
    comp.current_frame_id = (comp.current_frame_id + 1) % frame_count;
    return frames.at(comp.current_frame_id);
}

Animation::Animation(utils::Vector2i texture_size, int sprites_x, int sprites_y,
                     float life_time, int repeats_count, bool inverted)
    : m_tex_x(sprites_x - 1), m_tex_y(sprites_y - 1),
      m_sprites_x(sprites_x), m_sprites_y(sprites_y),
      m_life_time(life_time), m_repeats_count(repeats_count), m_inverted(inverted)
{
    if (m_inverted)
    {
        m_dx = 1;
        m_dy = 1;
        m_tex_x = 0;
        m_tex_y = 0;
    }
    m_texture_rect_size = {texture_size.x / m_sprites_x, texture_size.y / m_sprites_y};
    m_frame_time = m_life_time / (m_sprites_x * m_sprites_y);
}

void Animation::setFrameTime(int new_m_frame_time)
{
    m_frame_time = new_m_frame_time;
}

void Animation::setLifeTime(int new_m_life_time)
{
    m_life_time = new_m_life_time;
    if (m_repeats_count == 1)
    {
        m_frame_time = new_m_life_time / (m_sprites_x * m_sprites_y);
    }
}

void Animation::setTime(int new_time)
{
    m_time = new_time;
    int frame_ind = (new_time / m_frame_time);
    m_tex_x = frame_ind % m_sprites_x;
    m_tex_y = frame_ind / m_sprites_x;
}

void Animation::update(float dt)
{
    m_time += dt;
    if (m_time >= m_frame_time)
    {
        m_time = 0.f;
        animateSprite();
    }
}

Rect<int> Animation::getCurrentTextureRect() const
{
    return {m_texture_rect_size.x * m_tex_x, m_texture_rect_size.y * m_tex_y,
            m_texture_rect_size.x, m_texture_rect_size.y};
}

void Animation::animateSprite()
{
    m_tex_x += m_dx;
    if (m_tex_x >= m_sprites_x || m_tex_x < 0)
    {
        m_tex_y += m_dy;
        m_tex_x = (m_tex_x + m_sprites_x) % m_sprites_x;
    }
    if (m_tex_y >= m_sprites_y || m_tex_y < 0)
    {
        m_tex_y = (m_tex_y + m_sprites_y) % m_sprites_y;
    }

    if (m_inverted)
    {
        if (m_tex_x == 0 && m_tex_y == 0)
        {
            m_repeats_count--;
        }
    }
    else
    {
        if (m_tex_x == m_sprites_x - 1 && m_tex_y == m_sprites_y - 1)
        {
            m_repeats_count--;
        }
    }
}

TimedEventId addAnimation(std::function<void(float, int)> animator, float duration, float delay, TimedEventManager &timers)
{
    auto id = timers.addInfiniteEvent(animator, 0.f, delay);
    //! remove animation after finishing
    timers.addTimedEvent([id, &timers](float t, int c)
                         { timers.removeEvent(id); },
                         0, duration + delay);

    return id;
}

void interpolate(Transform &changee, const Transform &start, const Transform &end, float x)
{
    changee.setPosition(start.getPosition() + x * (end.getPosition() - start.getPosition()));
    changee.setScale(start.getScale() + x * (end.getScale() - start.getScale()));
    changee.setRotation(start.getRotation() + x * (end.getRotation() - start.getRotation()));
}

TimedEventId transformAnimation(float duration, float delay, Transform start_transform, Transform final_transform, Transform &trans, TimedEventManager &timers)
{

    return addAnimation([&timers, &trans, start_transform, final_transform, duration, delay](float t, int c)
                        {
                            
                            float x = t / (duration);
                            interpolate(trans, start_transform, final_transform, x); },
                        duration, delay, timers);
}

TimedEventId scaleAnimation(float duration, float delay, float scale_increase, float &scale, TimedEventManager &timers)
{
    return addAnimation([&timers, &scale, duration, scale_increase, delay](float t, int c)
                        {
                            float max_scale = 1 + scale_increase ;
                            float x = t / (duration / 2.f);
                            scale = t < duration/2 ? 1. + scale_increase*(x)  : max_scale  - (scale_increase)*(x - 1.f); },
                        duration, delay, timers);
}
TimedEventId impulseAnimation(float duration, float scale_increase, Transform &trans, TimedEventManager &timers)
{
    auto old_scale = trans.getScale();
    return addAnimation([&timers, &trans, old_scale, duration, scale_increase](float t, int c)
                        {
                            float max_scale = 1 + scale_increase;
                            float x = t / (duration / 2.f);
                            float scale = t < duration / 2 ? 1. + scale_increase * (x) : max_scale - (scale_increase) * (x - 1.f);
                            trans.setScale(old_scale * scale);
                        },
                        duration, 0.f, timers);
}

TimedEventId positionAnimation(float duration, float delay,
                               utils::Vector2f start_position,
                               utils::Vector2f final_position,
                               Transform &transformable, TimedEventManager &timers)
{
    Transform start_trans = transformable;
    start_trans.setPosition(start_position);
    Transform final_trans = transformable;
    final_trans.setPosition(final_position);
    return transformAnimation(duration, delay, start_trans, final_trans, transformable, timers);
}

TimedEventId positionAnimation(float duration, float delay,
                               utils::Vector2f start_pos,
                               utils::Vector2f final_position,
                               utils::Vector2f &position, TimedEventManager &timers)
{
    return addAnimation([&timers, &position, duration, start_pos, final_position, delay](float t, int c)
                        { 
                            float time_left = duration - t;
                            position = start_pos + t / duration * (final_position - start_pos); },
                        duration, delay, timers);
}
