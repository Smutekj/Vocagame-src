#include "TimedEventSystem.h"

TimedEventSystem::TimedEventSystem(utils::ContiguousColony<TimedEventComponent, int> &comps)
    : m_components(comps)
{
}

void TimedEventSystem::preUpdate(float dt, EntityRegistryT &entities)
{
}

void TimedEventSystem::postUpdate(float dt, EntityRegistryT &entities)
{

}

void TimedEventSystem::update(float dt)
{
    std::vector<int> comps_to_remove;

    auto comp_count = m_components.data.size();
    for (std::size_t comp_id = 0; comp_id < comp_count; ++comp_id)
    {
        auto &comp = m_components.data[comp_id];

        std::vector<int> finished_events;
        for (auto& [event_id, event] : comp.events)
        { 
            event.update(dt);
            if(event.getRepeatsLeft() == 0 && !event.isInfinite())
            {
                finished_events.push_back(event_id);               
            }
        }
        for(auto event_id : finished_events)
        {
            comp.events.erase(event_id);
        }

        //! if there are no more events, delete the component
        if(comp.events.empty())
        {
            comps_to_remove.push_back(m_components.data_ind2id.at(comp_id));
        }
    }

    for(auto id : comps_to_remove)
    {
        m_components.erase(id);
    }
}
