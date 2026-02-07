#include "PathSystem.h"

#include "Renderer.h"
#include "DrawLayer.h"
#include "Particles.h"
#include "../Components.h"

PathSystem::PathSystem(utils::ContiguousColony<PathComponent, int> &sprites, LayersHolder &layers)
    : m_components(sprites), m_layers(layers)
{
}

void PathSystem::preUpdate(float dt, EntityRegistryT &entities)
{
    auto &comps = m_components.data;
    auto &ids = m_components.data_ind2id;
    for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
    {
        auto &p_entity = entities.at(ids[comp_id]);
        auto &comp = comps[comp_id];
        auto pos = p_entity->getPosition();
        int current_step = comp.current_step;
        const auto &step = comp.path.steps.at(current_step);
        auto target_pos = step.target;
        float distance_left = utils::norm(target_pos - pos);
        auto vel = (target_pos - pos) / distance_left * step.speed;
        if (dt * step.speed > distance_left) //! if i will step into target point
        {
            if (comp.current_step == comp.path.stepsCount() - 1) //! reached final step
            {
                comp.on_reaching_end(*p_entity);
            }
            comp.current_step = comp.path.getNextStepId(current_step);
        }
        p_entity->m_vel = vel; //! should path override velocity?
        // p_entity->setPosition(pos + vel*dt);
    }
}
void PathSystem::postUpdate(float dt, EntityRegistryT &entities)
{
}
void PathSystem::update(float dt)
{
    auto &comps = m_components.data;
    auto &ids = m_components.data_ind2id;
    for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
    {
        auto &path = comps[comp_id];
    }
}
