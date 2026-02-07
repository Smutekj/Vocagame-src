
#pragma once

#include "Vector2.h"
#include "Systems/TimedEvent.h"
#include "Particles.h"
#include "Rect.h"
#include "Color.h"
#include "Sprite.h"

class GameObject;

#include "boost/describe.hpp"

#include "ObjectRegistry.h"


struct SpriteSpec
{
    std::string texture_id = "BrickWall";
    std::string layer_id = "Unit";
    std::string shader_id = "";
    ColorByte color = {255, 255, 255, 255};
    Recti tex_rect;
    utils::Vector2i tex_size;
    bool scalable = false;
};

struct HealthComponent
{
    float max_hp;
    float hp = max_hp;
    float hp_regen;
};

struct TimedEventComponent
{
    int addEvent(TimedEvent event)
    {
        next_id++;
        events.insert({next_id, event});
        return next_id;
    }
    std::unordered_map<int, TimedEvent> events;

private:
    int next_id = 0; //! TODO: this is fucked up and will fix it later
};

enum class ComponentId
{
    Boid,
    Shield,
    Hp
};


enum class AnimationId
{
    Shield,
    FrontShield,
    FrontShield2,
    BlueExplosion,
    PurpleExplosion,
    GermanFlag,
    GreenBeam,
};

struct AnimationComponent
{
    std::string id;
    unsigned int texture_id = 0;
    float time = 0.;
    float cycle_duration = 1.;
    int current_frame_id = 0;
    int n_repeats_left = 1;
    Rect<int> tex_rect = {0, 0, 0, 0};
    utils::Vector2i texture_size = {1, 1};
};

struct SpriteComponent
{
    std::string layer_id;
    std::string shader_id = "SpriteDefault";
    Sprite sprite;
};



struct TransformComponent{
    std::shared_ptr<GameObject> p_entity;
    
    utils::Vector2f target_pos;
    utils::Vector2f target_size;
    float target_angle;
    
    float duration = 1.f;
};

struct PathStep
{
    utils::Vector2f target;
    float speed = 50.f;
    float delay = 0.f;
    float distance_slowdown = 0.f;
};
BOOST_DESCRIBE_STRUCT(PathStep, (), (target, speed, delay, distance_slowdown));

struct Path
{
    bool cyclic = true;
    bool reversed = false;
    std::vector<PathStep> steps;

    std::size_t stepsCount() const
    {
        return steps.size();
    }

    int getNextStepId(int current_path_id)
    {
        int path_delta_id = 1 - 2 * reversed;
        current_path_id += path_delta_id;

        if (current_path_id >= stepsCount())
        {
            if (cyclic)
            {
                current_path_id = 0;
            }
            else
            {
                // reversed = !reversed;
                current_path_id = stepsCount() - 2;
            }
        }
        if (current_path_id < 0)
        {
            if (cyclic)
            {
                current_path_id = stepsCount() - 1;
            }
            else
            {
                // reversed = !reversed;
                current_path_id = 1;
            }
        }
        return current_path_id;
    }
};
BOOST_DESCRIBE_STRUCT(Path, (), (cyclic, steps));


struct PathComponent
{
    Path path;
    int current_step = 0;
    std::function<void(GameObject&)> on_reaching_end = [](GameObject&){};
};

struct TextureSpec
{
    std::string id;
    std::string path;
    TextureOptions options;
};
BOOST_DESCRIBE_STRUCT(TextureOptions, (), (wrap_x, wrap_y));
BOOST_DESCRIBE_STRUCT(TextureSpec, (), (id, path, options));
