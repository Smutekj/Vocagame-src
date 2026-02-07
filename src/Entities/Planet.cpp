#include "Planet.h"
#include "Player.h"

#include "GameWorld.h"
#include "Assets.h"
#include "DrawLayer.h"

Planet::Planet(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, spec, ent_id, TypeId::Planet)
{
    CollisionComponent c_comp;
    c_comp.type = ObjectType::Planet;
    c_comp.shape.convex_shapes.emplace_back(64);
    for(auto& p : c_comp.shape.convex_shapes[0].points)
    {
        p *= 2/sqrt(2);
    }
    // SpriteComponent s_comp = {.layer_id = "Unit", .shader_id = "lightningBolt", .sprite = Sprite{*m_textures->get("Arrow")}};
    m_world->m_systems.addEntityDelayed(getId(),  c_comp);


} 

void Planet::onCreation()
{

}

void Planet::draw(LayersHolder &target, Assets& assets)
{
    Sprite s(*assets.textures.get("Planet-9"));
    s.setPosition(m_pos);
    s.setScale(m_size);
    target.getCanvas("Unit").drawSprite(s);
}
void Planet::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
}


