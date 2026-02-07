#include "Pickup.h"

#include <Font.h>
#include <DrawLayer.h>

#include "CollisionSystem.h"
#include "GameWorld.h"
#include "Animation.h"
#include "Polygon.h"
#include "Utils/RandomTools.h"
#include "SoundSystem.h"
#include "Player.h"
#include "VisualEffects.h"

#include "Assets.h"

Pickup::Pickup(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, ObjectType::Pickup), m_type(spec.type)
{
}
void Pickup::onCreation()
{
    CollisionComponent c_comp;
    Polygon shape = {4};

    shape.setScale(m_size / 2.);
    c_comp.shape.convex_shapes.push_back(shape);
    c_comp.type = ObjectType::Pickup;
    m_world->m_systems.add(c_comp, getId());
}

void Pickup::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    GameObject::onCollisionWith(obj, c_data);

    if (obj.getType() == ObjectType::Player)
    {
        kill();
    }
}

void Pickup::draw(LayersHolder &layers, Assets& assets)
{
}

void BoomBox::onCreation()
{
    CollisionComponent c_comp;
    c_comp.type = ObjectType::BoomBox;
    c_comp.shape.convex_shapes.emplace_back(4);
    m_world->m_systems.addEntity(getId(), c_comp);
}

BoomBox::BoomBox(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, ObjectType::BoomBox),
      m_boom_radius(spec.explosion_radius),
      m_boom_time(spec.timer),
      m_boom_strength(spec.explosion_strength)
{
}


void BoomBox::update(float dt)
{
    m_pos += m_vel * dt;
}

void BoomBox::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (m_collision_resolvers.contains(obj.getType()))
    {
        m_collision_resolvers.at(obj.getType())(obj, c_data);
    }
    if (obj.getType() == ObjectType::Player)
    {
        if (!m_is_ticking)
        {
            startTicking();
        }
    }
}

void BoomBox::startTicking()
{

    m_is_ticking = true;

    TimedEvent ticking = {[this](float t, int c)
                          {
                              if (c >= 4)
                              {
                                  explode();
                                  return;
                              }
                              if (c % 2 == 0)
                              {
                                  float player_dist = utils::dist(m_world->m_player->getPosition(), m_pos);
                                  SoundSystem::play("CountTick", player_dist / 2.f);
                                  m_bomb_sprite.setColor({255, 0, 0, 255});
                              }
                              else
                              {
                                  m_bomb_sprite.setColor({255, 255, 255, 255});
                              }
                          },
                          0.5f, 0.f, 5};

    TimedEventComponent t_comp;
    t_comp.addEvent(ticking);
    m_world->m_systems.addDelayed(t_comp, getId());
}

void BoomBox::explode()
{
    float player_dist = utils::dist(m_world->m_player->getPosition(), m_pos);
    if (player_dist < m_boom_radius)
    {
        auto impact_dir = m_world->m_player->getPosition() - m_pos;
        impact_dir /= utils::norm(impact_dir);
        m_world->m_player->m_impulse_vel += impact_dir * m_boom_strength;
    }

    //! kill near objects
    for (auto type : {ObjectType::BoomBox, ObjectType::Wall, ObjectType::TextBubble})
    {
        auto close_objects = m_world->getCollisionSystem().findNearestObjectInds(type, m_pos, m_size.x * 1.5f);
        for (auto obj_id : close_objects)
        {
            auto obj = m_world->get(obj_id);
            float distance = utils::dist(obj->getPosition(), m_pos);
            if (distance < m_size.x * 1.5f)
            {
                obj->kill();
            };
        }
    }

    auto close_objects = m_world->getCollisionSystem().findNearestObjectInds(ObjectType::BoomBox, m_pos, m_size.x * 1.5f);
    for (auto obj_id : close_objects)
    {
        auto obj = m_world->get(obj_id);
        float distance = utils::dist(obj->getPosition(), m_pos);
        if (distance < m_size.x * 1.5f)
        {
            static_cast<BoomBox *>(obj)->startTicking();
        };
    }
    SoundSystem::play("Explosion1", player_dist / 2.f);

    //! add explosion
    AnimatedSprite::Spec spec;
    spec.anim_id = "PurpleExplosion";
    spec.n_repeats = 1;
    spec.pos = m_pos;
    spec.size = m_size * 2.68;
    m_world->createObject(spec);
    kill();
}

void BoomBox::draw(LayersHolder &layers, Assets& assets)
{
    m_bomb_sprite.setTexture(*assets.textures.get("TNT"));
    auto &canvas = layers.getCanvas("Unit");
    m_bomb_sprite.setPosition(m_pos);
    m_bomb_sprite.setScale(m_size / 2.f);
    canvas.drawSprite(m_bomb_sprite);
}

#include "Factories.h"
#include "Bullet.h"

GunBlock::GunBlock(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, ObjectType::GunBlock)
{
    m_cooldown = spec.cooldown;
    m_strength = spec.strength;
    m_dmg = spec.dmg;
    m_speed = spec.speed;

    m_bullet_factory = std::make_unique<EntityFactory2<Bullet>>(world);
}

void GunBlock::update(float dt)
{
}
void GunBlock::draw(LayersHolder &layers, Assets& assets)
{
    auto &target = layers.getCanvas("Unit");

    Sprite base(*assets.textures.get("Turrets"));

    base.setPosition(m_pos);
    base.setTexture(*assets.textures.get("Turrets"));
    base.m_tex_rect = {0, 0, 144, 196};
    base.setScale(m_size.x / 2.f, m_size.y / 2.f);
    base.setRotation(utils::radians(m_angle - 90));
    target.drawSprite(base);

    /*     base.setRotation(utils::to_radains * (m_angle - 90));
        base.m_tex_rect = {285, 197 - 94 - 71, 74, 94};
       target.drawSprite(base);    */
}

void GunBlock::onCreation()
{
    TimedEvent shoot = {[this](float t, int c)
                        {
                            Bullet::Spec spec;
                            auto dir = utils::angle2dir(m_angle);
                            spec.pos = m_pos + dir * m_size.x / 2.f;
                            spec.vel = dir * m_speed;
                            // spe
                            spec.size = m_size / 2.f;
                            spec.punch_strength = m_strength;
                            m_bullet_factory->create(spec);
                        },
                        TimedEventType::Infinite, m_cooldown, m_cooldown};

    TimedEventComponent t_comp;
    t_comp.addEvent(shoot);
    m_world->m_systems.addEntity(getId(), t_comp);
}
