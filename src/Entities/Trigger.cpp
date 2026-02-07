#include "Trigger.h"

#include "GameWorld.h"

Trigger::Trigger(GameWorld &world, const Spec &spec, int ent_id)
: GameObject(&world, ent_id, TypeId::Trigger)
{
    CollisionComponent c_comp;
    c_comp.shape.convex_shapes.emplace_back(4);
    c_comp.type = ObjectType::Trigger;
    world.m_systems.addEntityDelayed(ent_id, c_comp);
}