
#include "Spawners.h"


SpawnerI::SpawnerI(GameWorld &world, int max_alive_count)
    : m_world(world), m_max_alive_count(max_alive_count)
{
    //! remove dead entities from set of spawned entities
    m_on_death = std::make_unique<PostBox<EntityDiedEvent>>(*world.p_messenger, [this](auto &events)
                                                            {
            for (auto &e: events) {
                    m_spawned_ids.erase(e.id);
                
            } });
}

SpawnerI::~SpawnerI()
{
    if (m_kill_on_destriuction)
    {
        for (auto id : m_spawned_ids)
        {
        }
    }
}

void SpawnerI::setMaxCount(int max_count)
{
    m_max_alive_count = max_count;
}
