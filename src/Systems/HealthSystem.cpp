#include "HealthSystem.h"

HealthSystem::HealthSystem(utils::ContiguousColony<HealthComponent, int> &comps, PostOffice &m_messenger)
    : m_components(comps), p_messenger(&m_messenger)
{
    auto on_dmg_receival = [&comps](const std::deque<DamageReceivedEvent> &messages)
    {
        for (auto &msg : messages)
        {
            if(comps.contains(msg.receiver_id))
            {
                comps.get(msg.receiver_id).hp -= msg.dmg;
            }
        }
    };

    m_postbox = std::make_unique<PostBox<DamageReceivedEvent>>(m_messenger, on_dmg_receival);
}

void HealthSystem::preUpdate(float dt, EntityRegistryT &entities)
{
}

void HealthSystem::postUpdate(float dt, EntityRegistryT &entities)
{
    auto comp_count = m_components.data.size();
    for (std::size_t comp_id = 0; comp_id < comp_count; ++comp_id)
    {
        auto &comp = m_components.data[comp_id];
        if(comp.hp <= 0.)
        {
            entities.at(m_components.data_ind2id.at(comp_id))->kill();
        }
    }
}

void HealthSystem::update(float dt)
{
    auto comp_count = m_components.data.size();
    for (std::size_t comp_id = 0; comp_id < comp_count; ++comp_id)
    {
        auto &comp = m_components.data[comp_id];
        comp.hp += comp.hp_regen * dt;
        comp.hp = std::min(comp.hp, comp.max_hp);
    }
}
