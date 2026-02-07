#include "Bullet.h"
#include "Player.h"

#include "GameWorld.h"
#include "Assets.h"
#include "DrawLayer.h"

Bullet::Bullet(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, spec, ent_id, TypeId::Bullet)
{
    m_lifetime = spec.life_time;
    m_punch_strength = spec.punch_strength;
}

void Bullet::onCreation()
{
    CollisionComponent c_comp;
    c_comp.type = ObjectType::Bullet;
    c_comp.shape.convex_shapes.emplace_back(4);
    // SpriteComponent s_comp = {.layer_id = "Unit", .shader_id = "lightningBolt", .sprite = Sprite{*m_textures->get("Arrow")}};

    TimedEventComponent t_comp;
    t_comp.addEvent({[this](float t, int c)
                     { kill(); }, m_lifetime});

    m_world->m_systems.addEntity(getId(), c_comp, t_comp);
}

void Bullet::draw(LayersHolder &target, Assets &assets)
{
    Sprite s(*assets.textures.get("Arrow"));
    s.setPosition(m_pos);
    s.setScale(m_size / 2.f);
    target.getCanvas("Unit").drawSprite(s, "lightningBolt");
}
void Bullet::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (m_collision_resolvers.contains(obj.getType()))
    {
        m_collision_resolvers.at(obj.getType())(obj, c_data);
        return;
    }
    if (obj.getType() == ObjectType::Player)
    {
        auto dr = getPosition() - obj.getPosition();
        static_cast<Player &>(obj).m_impulse_vel += -dr / utils::norm(dr) * m_punch_strength;
        kill();
    }
    if (obj.getType() == ObjectType::Wall)
    {
        kill();
    }
}
