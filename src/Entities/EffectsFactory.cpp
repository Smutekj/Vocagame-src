#include "EffectsFactory.h"

#include "Factories.h"
#include "GameWorld.h"

#include "VisualEffects.h"
#include "Utils/RandomTools.h"

using namespace utils;

void starParticleEffect(utils::Vector2f pos, FactoryBase &particle_factory)
{
    ParticleTexObject::Spec spec;
    spec.pos = pos;
    spec.tex_id = "Star";
    spec.part_acc = {0, -200.f};
    spec.size = {20};
    spec.start_min_speed = 150.f;
    spec.start_max_speed = 200.f;
    spec.spread_radius_min = 40.f;
    spec.spread_radius_max = 50.f;
    spec.spread_angle_min = 45.f;
    spec.spread_angle_max = 180 - 45.f;
    spec.max_particles = 69;
    spec.life_time = 10.f;
    spec.particle_lifetime = 1.0f;
    spec.period = 0.02f;
    spec.n_repeats = 1;

    particle_factory.create(spec);
}
void fireworksParticleEffect1(utils::Vector2f pos, FactoryBase &particle_factory)
{
    //! make particle effect or something idk
    ParticleObject::Spec spec;
    spec.pos = pos;
    spec.layer_id = "Bloom";
    spec.part_acc = {0, -200.f};
    spec.start_min_speed = 150.f;
    spec.start_max_speed = 150.f;
    spec.life_time = 10.f;
    spec.particle_lifetime = 1.0f;
    spec.period = 0.f;
    spec.n_repeats = -1;
    spec.start_color = {20.f * randf(), 100.f * randf(), 2.f * randf(), 1.};
    spec.end_color = spec.start_color;
    // spec.end_color = {0.f * randf(), 100.*randf(), randf(), 1.};
    particle_factory.create(spec);
}

EffectsFactory::EffectsFactory(GameWorld &world)
    : m_world(world)
{
    m_factories[TypeId::AnimatedSprite] = std::make_shared<EntityFactory2<AnimatedSprite>>(world);
    m_factories[TypeId::ParticleTexObject] = std::make_shared<EntityFactory2<ParticleTexObject>>(world);
    m_factories[TypeId::ParticleObject] = std::make_shared<EntityFactory2<ParticleObject>>(world);
    m_factories[TypeId::VisualObject] = std::make_shared<EntityFactory2<VisualObject>>(world);
    
    registerFactories();
}

void EffectsFactory::registerFactories()
{
    m_makers[EffectId::StarBurst] = [this](EffectSpec spec)
    {
        starParticleEffect(spec.pos, *m_factories.at(TypeId::ParticleTexObject));
    };
    m_makers[EffectId::Fireworks1] = [this](EffectSpec spec)
    {
        fireworksParticleEffect1(spec.pos, *m_factories.at(TypeId::ParticleObject));
    };
    m_makers[EffectId::BlueExplosion] = [this](EffectSpec spec)
    {
        AnimatedSprite::Spec s;
        s.pos = spec.pos;
        s.size = spec.size;
        s.n_repeats = 1;
        s.period = spec.life_time;
        s.anim_id = "BlueExplosion";
        m_factories.at(TypeId::AnimatedSprite)->create(s);
    };
    m_makers[EffectId::PurpleExplosion] = [this](EffectSpec spec)
    {
        AnimatedSprite::Spec s;
        s.pos = spec.pos;
        s.size = spec.size;
        s.n_repeats = 1;
        s.period = spec.life_time;
        s.anim_id = "PurpleExplosion";
        m_factories.at(TypeId::AnimatedSprite)->create(s);
    };
}

void EffectsFactory::create(EffectId id, utils::Vector2f pos, utils::Vector2f size, float life_time)
{
    m_makers.at(id)({pos, size, life_time});
}