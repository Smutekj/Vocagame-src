#include "VisualEffects.h"
#include "DrawLayer.h"
#include "Utils/RandomTools.h"
#include "GameWorld.h"
#include "Walls.h"
#include "Assets.h"

VisualObject::VisualObject(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, ObjectType::VisualObject), m_spec(spec)
{
    if (m_spec.life_time > 0.f)
    {
        TimedEvent kill_switch = {[this](float t, int c)
                                  { kill(); },
                                  m_spec.life_time};
        TimedEventComponent t_comp;
        t_comp.addEvent(kill_switch);
        m_world->m_systems.addEntity(getId(), t_comp);
    };
}

void VisualObject::onCreation()
{
}

void VisualObject::draw(LayersHolder &target, Assets &assets)
{
    auto tex_p = assets.textures.get(m_spec.sprite.texture_id);
    Sprite s(*tex_p);
    // s.m_tex_rect = m_spec.sprite.tex_rect;
    s.setPosition(m_pos);
    s.setScale(m_size / 2.f);
    s.setRotation(utils::radians(m_angle));
    s.setColor(m_spec.sprite.color);

    if (m_spec.sprite.shader_id.size() == 0)
    {
        target.getCanvas(m_spec.sprite.layer_id).drawSprite(s);
    }
    else
    {
        target.getCanvas(m_spec.sprite.layer_id).drawSprite(s, m_spec.sprite.shader_id);
    }
}

AnimatedSprite::AnimatedSprite(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, ObjectType::AnimatedSprite), m_spec(spec)
{
}

void AnimatedSprite::onCreation()
{
    AnimationComponent anim;
    anim.id = m_spec.anim_id;
    anim.cycle_duration = m_spec.period;
    anim.n_repeats_left = m_spec.n_repeats;

    if (m_spec.n_repeats != -1)
    {
        TimedEvent kill_switch = {[this](float t, int c)
                                  { kill(); },
                                  0.f, anim.n_repeats_left * anim.cycle_duration};
        TimedEventComponent t_comp;
        t_comp.addEvent(kill_switch);
        m_world->m_systems.addEntity(getId(), t_comp);
    };

    m_world->m_systems.addEntity(getId(), anim);
}

void AnimatedSprite::draw(LayersHolder &target, Assets &assets)
{
    auto &a_comp = m_world->m_systems.get<AnimationComponent>(getId());

    auto &canvas = target.getCanvas("Unit");
    Sprite the_sprite;
    the_sprite.m_tex_size = a_comp.texture_size;
    the_sprite.m_tex_rect = a_comp.tex_rect;
    the_sprite.m_texture_handles[0] = a_comp.texture_id;
    the_sprite.setPosition(m_pos);
    the_sprite.setRotation(m_angle);
    the_sprite.setScale(m_size / 2.f);
    if (a_comp.texture_id > 0)
    {
        canvas.drawSprite(the_sprite);
    }
}

StarEmitter::StarEmitter(GameWorld *world, TextureHolder &textures, int ent_id)
    : m_particles(*textures.get("Star")), GameObject(world, ent_id, ObjectType::AnimatedSprite)
{
    m_particles.setEmitter([](utils::Vector2f spawn_pos)
                           {
        Particle p;
        auto rand_angle =  utils::randf(0, 360);
        p.pos = spawn_pos + 20 * utils::angle2dir(rand_angle);
        p.color = {1,1,1,1};
        p.scale = 10.f;
        p.vel = 30.*utils::angle2dir(rand_angle);
        return p; });
    m_particles.setUpdater([](Particle &p, float dt)
                           {
        p.pos += p.vel*dt;
        p.color.a -= 255 / (p.life_time / dt);
        if(p.color.a < 0)
        {
            p.color.a = 0;
        } });

    m_particles.setRepeat(false);
}

void StarEmitter::update(float dt)
{
    m_time += dt;
    if (m_lifetime > 0.f && m_time > m_lifetime)
    {
        kill();
    }
    m_particles.setSpawnPos(m_pos);
    m_particles.update(dt);
}
void StarEmitter::draw(LayersHolder &layers, Assets &assets)
{
    m_particles.draw(layers.getCanvas("Unit"));
}

ParticleObject::ParticleObject(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, TypeId::ParticleObject), m_spec(spec), m_particles(spec.max_particles)
{
    m_particles.setLifetime(spec.life_time);

    m_particles.setEmitter([spec](utils::Vector2f pos)
                           {
    Particle p;
    p.scale = spec.size;
    p.color =  {10.f*utils::randf(), 30.f*utils::randf(), 10.f*utils::randf(), 1.f};
    p.pos = pos + utils::angle2dir(utils::randf(spec.spread_angle_min, spec.spread_angle_max)) * utils::randf(spec.spread_radius_min, spec.spread_radius_max);
    p.vel = spec.vel + utils::angle2dir(utils::randf(0, 360.f)) * utils::randf(spec.start_min_speed, spec.start_max_speed); 
    p.life_time = spec.particle_lifetime;
    p.acc = spec.part_acc;
    return p; });

    m_particles.setUpdater([spec](Particle &p, float dt)
                           {
        p.vel += p.acc * dt;
        p.pos += p.vel * dt; });
    m_particles.setPeriod(spec.period);
    m_particles.setRepeat(spec.n_repeats <= 0);
}

void ParticleObject::onCreation()
{
    TimedEventComponent t_comp;
    if (m_spec.life_time > 0.f)
    {
        // float life_time = std::min({m_spec.life_time, m_spec.n_repeats * m_particles.getPeriod()});
        TimedEvent kill_event = {[this](float t, int c)
                                 {
                                     kill();
                                 },
                                 m_spec.life_time, m_spec.life_time, 1};

        t_comp.addEvent(kill_event);
        m_world->m_systems.addEntity(getId(), t_comp);
    }
    else if (m_spec.n_repeats > 0)
    {
        float life_time = m_spec.n_repeats * m_spec.period * m_spec.max_particles;
        TimedEvent kill_event = {[this](float t, int c)
                                 {
                                     kill();
                                 },
                                 0.f, life_time, 1};

        t_comp.addEvent(kill_event);
        m_world->m_systems.addEntity(getId(), t_comp);
    }
}

void ParticleObject::update(float dt)
{
    m_particles.setSpawnPos(m_pos);
    m_particles.update(dt);
}

void ParticleObject::draw(LayersHolder &layers, Assets &assets)
{
    auto canvas_p = layers.getCanvasP(m_spec.layer_id);

    if (canvas_p)
    {
        m_particles.draw(*canvas_p);
    }
}

ParticleTexObject::ParticleTexObject(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, TypeId::ParticleTexObject),
      m_layer_id(spec.layer_id), m_particles(spec.max_particles),
      m_tex_id(spec.tex_id),
      m_life_time(spec.life_time)
{

    m_particles.setEmitter([spec](utils::Vector2f pos)
                           {
    Particle p;
    p.scale = spec.size;
    p.pos = pos + utils::angle2dir(utils::randf(spec.spread_angle_min, spec.spread_angle_max)) * utils::randf(spec.spread_radius_min, spec.spread_radius_max);
    p.vel = spec.vel + utils::angle2dir(utils::randf(0.f, 360.f)) * utils::randf(spec.start_min_speed, spec.start_max_speed); 
    p.life_time = spec.particle_lifetime;
    p.acc = spec.part_acc;
    return p; });

    m_particles.setUpdater([spec](Particle &p, float dt)
                           {
        p.vel += p.acc * dt;
        p.pos += p.vel * dt; });
    m_particles.setPeriod(spec.period);
    m_particles.setRepeat(spec.n_repeats > 1);
}

void ParticleTexObject::onCreation()
{
    TimedEventComponent t_comp;
    if (m_life_time > 0.f)
    {
        TimedEvent kill_event = {[this](float t, int c)
                                 {
                                     kill();
                                 },
                                 m_life_time, m_life_time, 1};

        t_comp.addEvent(kill_event);
        m_world->m_systems.addEntity(getId(), t_comp);
    }
}

void ParticleTexObject::update(float dt)
{
    m_particles.setSpawnPos(m_pos);
    m_particles.update(dt);
}

void ParticleTexObject::draw(LayersHolder &layers, Assets &assets)
{
    auto canvas_p = layers.getCanvasP(m_layer_id);
    m_particles.setTexture(*assets.textures.get(m_tex_id));

    if (canvas_p)
    {
        m_particles.draw(*canvas_p);
    }
}