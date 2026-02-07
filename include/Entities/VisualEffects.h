#pragma once

#include "../GameObject.h"
#include "../Animation.h"
#include <Particles.h>

class LayersHolder;

struct AnimSpec : public GameObjectSpec
{

    AnimationId anim_id;
    int n_repeats = 1;
    float period = 1.f;
};

class VisualObject : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {
            obj_type = TypeId::VisualObject;
        }
        float life_time = -1;
        SpriteSpec sprite;
    };

public:
    VisualObject(GameWorld &world, const Spec &spec, int ent_id = -1);
    virtual ~VisualObject() override {};

    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;

private:
    Spec m_spec;
};
BOOST_DESCRIBE_STRUCT(VisualObject::Spec, (GameObjectSpec), (life_time, sprite));

class AnimatedSprite : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}
        std::string anim_id;
        int n_repeats = -1;
        float period = 0.5f;
    };

public:
    AnimatedSprite(GameWorld &world, const Spec &spec, int ent_id = -1);
    virtual ~AnimatedSprite() override {};

    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;

private:
    Spec m_spec;
};
BOOST_DESCRIBE_STRUCT(AnimatedSprite::Spec, (GameObjectSpec), (anim_id, n_repeats, period));

class ParticleObject : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}

        Color start_color = {10, 1, 0, 1};
        Color end_color = {1, 1, 1, 1};

        float angle_vel = 0.f;
        float start_max_speed = 100.f;
        float start_min_speed = 0.f;
        float spread_radius_min = 0.f;
        float spread_radius_max = 0.f;
        float spread_angle_min = 0.f;
        float spread_angle_max = 360;
        utils::Vector2f start_vel = {0.f, 0.f};
        utils::Vector2f part_acc = {0.f, 0.f};

        std::string layer_id = "Bloom";

        int max_particles = 100;
        int n_repeats = 1;
        float particle_lifetime = 0.5f;
        float period = 0.05f;

        float life_time = -1.f;
    };

public:
    ParticleObject(GameWorld &world, const Spec &spec, int ent_id = -1);
    virtual ~ParticleObject() override {};

    virtual void onCreation() override;
    virtual void update(float dt) override;
    virtual void draw(LayersHolder &target, Assets& assets) override;

private:
    Spec m_spec;
    Particles m_particles;
};
BOOST_DESCRIBE_STRUCT(ParticleObject::Spec, (GameObjectSpec), (part_acc, start_min_speed, start_max_speed, spread_angle_min, spread_angle_max, spread_radius_min, spread_radius_max, start_color, end_color, layer_id, life_time, period, max_particles));

class StarEmitter : public GameObject
{

public:
    StarEmitter(GameWorld *world, TextureHolder &textures, int ent_id = -1);

    virtual void update(float dt) final;
    virtual void draw(LayersHolder &target, Assets& assets) override;

private:
    TexturedParticles m_particles;
    float m_time = 0.f;
    float m_lifetime = 1.5f;
};

class ParticleTexObject : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}

        float angle_vel = 0.f;
        float start_max_speed = 100.f;
        float start_min_speed = 0.f;
        float spread_radius_min = 0.f;
        float spread_radius_max = 0.f;
        float spread_angle_min = 0.f;
        float spread_angle_max = 360;
        utils::Vector2f start_vel = {0.f, 0.f};
        utils::Vector2f part_acc = {0.f, 0.f};

        std::string tex_id = "";
        std::string layer_id = "Unit";

        int max_particles = 100;
        int n_repeats = 1;
        float particle_lifetime = 0.5f;
        float period = 0.05f;

        float life_time = -1.f;
    };

    ParticleTexObject(GameWorld &world, const Spec &spec, int ent_id = -1);

    virtual void onCreation() override;
    virtual void update(float dt) override;
    virtual void draw(LayersHolder &target, Assets& assets) override;

private:
    float m_life_time = 1.f;
    std::string m_layer_id = "Unit";
    std::string m_tex_id = "";
    TexturedParticles m_particles;
};
BOOST_DESCRIBE_STRUCT(ParticleTexObject::Spec, (GameObjectSpec), (part_acc, start_min_speed, start_max_speed, spread_angle_min, spread_angle_max, spread_radius_min, spread_radius_max, layer_id, life_time, period, max_particles, particle_lifetime));