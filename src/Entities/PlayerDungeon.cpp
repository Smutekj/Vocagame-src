#include "PlayerDungeon.h"

#include "GameWorld.h"
#include "Assets.h"
#include "DrawLayer.h"

PlayerDungeon::PlayerDungeon(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, spec, ent_id, TypeId::Player)
{
}

void PlayerDungeon::onCreation()
{
    CollisionComponent c_comp;
    c_comp.type = ObjectType::Player;
    c_comp.shape.convex_shapes.emplace_back(4);
    // SpriteComponent s_comp = {.layer_id = "Unit", .shader_id = "lightningBolt", .sprite = Sprite{*m_textures->get("Arrow")}};

    TimedEventComponent t_comp;
    t_comp.addEvent({[this](float t, int c)
                     { kill(); }, m_lifetime});

    m_world->m_systems.addEntity(getId(), c_comp, t_comp);
}

void PlayerDungeon::draw(LayersHolder &target, Assets &assets)
{
    RectangleSimple s;
    s.setPosition(m_pos);
    s.setScale(m_size);
    target.getCanvas("Unit").drawRectangle(s);
}
void PlayerDungeon::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if(obj.getType() == TypeId::Box)
    {

    }
}

void PlayerDungeon::update(float dt)
{
    
}