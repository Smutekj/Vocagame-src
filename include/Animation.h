#pragma once

#include <filesystem>

#include <Rect.h>
#include <Texture.h>
#include <Transform.h>

#include "Components.h"
#include "Systems/System.h"
#include "TimedEventManager.h"

class TextureAtlas;

class AnimationSystem : public SystemI
{
public:
    AnimationSystem(utils::ContiguousColony<AnimationComponent, int> &comps,
                    std::filesystem::path animations_dir,
                    std::filesystem::path animations_data_dir);

    void registerAnimation(TextureAtlas &atlas, const std::string &id);

    virtual void preUpdate(float dt, EntityRegistryT &entities) override {}
    virtual void update(float dt) override;
    virtual void postUpdate(float dt, EntityRegistryT &entities) override {}

private:
    Rect<int> getNextFrame(AnimationComponent &comp);

private:
    struct FrameData
    {
        std::shared_ptr<Texture> p_texture;
        std::vector<Rect<int>> tex_rects;
    };

    utils::ContiguousColony<AnimationComponent, int> &m_components;
    std::unordered_map<std::string, FrameData> m_frame_data;

    TextureHolder m_atlases;
    std::filesystem::path m_animations_data_dir;
};

class Animation
{

public:
    Animation(utils::Vector2i texture_size, int n_sprites_x, int n_sprites_y,
              float life_time, int n_repeats = 1,
              bool inverted = false);

    void setFrameTime(int new_frame_time);
    void setLifeTime(int new_life_time);
    void setTime(int new_time);
    void update(float dt);
    Rect<int> getCurrentTextureRect() const;

private:
    void animateSprite();

private:
    float m_time = 0;
    float m_frame_time = 5;
    float m_life_time;

    int m_tex_x;
    int m_tex_y;

    int m_sprites_x;
    int m_sprites_y;

    int m_repeats_count = -1;

    int m_dx = -1;
    int m_dy = -1;

    bool m_inverted = false;

    utils::Vector2i m_texture_rect_size;
};


TimedEventId addAnimation(std::function<void(float, int)> animator, float duration, float delay, TimedEventManager &timers);
TimedEventId scaleAnimation(float duration, float delay, float scale_increase, float &scale, TimedEventManager &timers);
TimedEventId impulseAnimation(float duration, float scale_increase, Transform &trans, TimedEventManager &timers);
TimedEventId positionAnimation(float duration, float delay,
                               utils::Vector2f start_pos,
                               utils::Vector2f final_position,
                               utils::Vector2f &position, TimedEventManager &timers);

TimedEventId positionAnimation(float duration, float delay,
                               utils::Vector2f start_pos,
                               utils::Vector2f final_position,
                               Transform &transformable, TimedEventManager &timers);
TimedEventId transformAnimation(float duration, float delay, Transform start_transform, Transform final_transform, Transform &trans, TimedEventManager &timers);