#include "Box.h"

#include "Components.h"
#include "GameWorld.h"
#include "Assets.h"
#include "DrawLayer.h"

Box::Box(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, spec, ent_id, TypeId::Box),
      m_text(spec.text),
      m_definition(spec.definition)
{

    CollisionComponent c_comp;
    c_comp.type = ObjectType::Box;
    c_comp.shape.convex_shapes.emplace_back(4);
    // SpriteComponent s_comp = {.layer_id = "Unit", .shader_id = "lightningBolt", .sprite = Sprite{*m_textures->get("Arrow")}};

    TimedEventComponent t_comp;
    m_world->m_systems.addEntityDelayed(getId(), c_comp, t_comp);
}
void Box::draw(LayersHolder &target, Assets &assets)
{
    Sprite s(*assets.textures.get("MetalBox1"));
    s.setPosition(m_pos);
    s.setScale(m_size / 2.f);
    target.getCanvas("Unit").drawSprite(s);
}

void Box::update(float dt)
{
    m_pos += m_vel * dt;
    m_vel = 0.;
    m_touches_wall = false;
}

void Box::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (obj.getType() == ObjectType::Player)
    {
        if (!m_touches_player) //! first touch with player
        {
            m_world->p_messenger->send(PlayerTouchedBox{.text = m_text, .definition = m_definition});
        }
        m_touches_player = true;
        Vec2 normal = c_data.separation_axis;
        auto rel_vel = obj.m_vel - m_vel;
        auto dv_in_norm = utils::dot(rel_vel, normal);
        //! if player moves into box, he pushes it
        if (dv_in_norm < 0.f && !m_touches_wall)
        {
            m_vel = obj.m_vel;
        }
    }
    if (obj.getType() == ObjectType::Wall)
    {
        auto normal = c_data.separation_axis;
        auto dv_normal = utils::dot(normal, m_vel - obj.m_vel);
        if (dv_normal > 0.f )
        {
            m_touches_wall = true;
            move(-c_data.separation_axis * c_data.minimum_translation);
        }
    }
}

bool Box::isTouchedByPlayer() const
{
    return m_touches_player;
}

std::string Box::getText() const
{
    return m_text;
}

std::string Box::getDefinition() const
{
    return m_definition;
}