#include "Spawner.h"
#include "Player.h"
#include "GameWorld.h"

Spawner::Spawner(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, TypeId::Spawner)
{
    m_period = spec.period;
    m_delay = spec.delay;
    m_spawn_count= spec.spawn_count;
    
    m_spawnee = spec.spawned_spec;
    m_spawnee.obj_type = TypeId::TextBubble; 
}

void Spawner::onCreation()
{
    TimedEventComponent t_comp;
    t_comp.addEvent({[this](float t, int c)
                     {
                         m_spawnee.pos = getPosition();
                         m_world->createObject(m_spawnee);
                     },
                     m_period, m_delay, m_spawn_count});

    m_world->m_systems.addEntity(getId(), t_comp);
}
void Spawner::draw(LayersHolder &target, Assets& assets)
{
}
