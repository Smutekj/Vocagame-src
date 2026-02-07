#pragma once

#include <unordered_map>
#include <memory>
#include <typeindex>

#include "Utils/ContiguousColony.h"
#include "Vector2.h"

#include "Systems/System.h"
#include "CollisionSystem.h"
#include "Components.h"


template <class... ComponentTypes>
class ComponentWorld
{
public:
    ComponentWorld(EntityRegistryT &entity_registry)
        : m_entity_registry(entity_registry)
    {
    }

    void registerSystem(std::shared_ptr<SystemI> p_system)
    {
        m_systems2[typeid(*p_system)] = p_system;
    }

    template <class ComponentType>
    ComponentType &get(int entity_id)
    {
        // assert(has<ComponentType>(entity_id));
        return std::get<ComponentHolder<ComponentType>>(m_components).get(entity_id);
    }

    template <class ComponentType>
    bool has(int entity_id) const
    {
        return std::get<ComponentHolder<ComponentType>>(m_components).has(entity_id);
    }

    template <class ...Components>
    void addEntityDelayed(int entity_id, Components&&... comps)
    {
        (addDelayed(std::forward<Components>(comps), entity_id),...);
    }

    template <class ComponentType>
    void addDelayed(ComponentType&& comp, int entity_id)
    {
        using Decayed = std::decay_t<ComponentType>;  // strips & and const
        std::get<ComponentHolder<Decayed>>(m_components).addDelayed(std::move(comp), entity_id);
    }
    template <class ComponentType>
    void add(ComponentType&& comp, int entity_id)
    {
        using Decayed = std::decay_t<ComponentType>;  // strips & and const
        std::get<ComponentHolder<Decayed>>(m_components).add(std::move(comp), entity_id);
    }

    template <class ComponentType>
    void remove(int entity_id)
    {
        // m_ecs.removeComponent<ComponentType>(entity_id);
        std::get<ComponentHolder<ComponentType>>(m_components).erase(entity_id);
    }
    template <class ComponentType>
    void removeDelayed(int entity_id)
    {
        // m_ecs.removeComponent<ComponentType>(entity_id);
        std::get<ComponentHolder<ComponentType>>(m_components).eraseDelayed(entity_id);
    }
    void removeEntity(int entity_id)
    {
        // m_ecs.removeEntity(entity_id);        
        std::apply([entity_id](auto &&...comp_holder)
                   { (comp_holder.erase(entity_id), ...); }, m_components);
    }

    template <class ComponentType>
    utils::ContiguousColony<ComponentType, int> &getComponents()
    {
        return std::get<ComponentHolder<ComponentType>>(m_components).getComponents();
    }

    template <class... Components>
    void addEntity(int id, Components&&... comps)
    {
        (add(std::forward<Components>(comps), id), ...);
    }

    void preUpdate(float dt)
    {
        for (auto &[system_id, system] : m_systems2)
        {
            system->preUpdate(dt, m_entity_registry);
        }
    }
    void update(float dt)
    {
        for (auto &[system_id, system] : m_systems2)
        {
            system->update(dt);
        }
    }

    void postUpdate(float dt)
    {
        for (auto &[system_id, system] : m_systems2)
        {
            system->postUpdate(dt, m_entity_registry);
        }

        //! add components to their holders at the end of update steps
        std::apply(([](auto&&... comps){(comps.addWaiting(),...);}), m_components);
        std::apply(([](auto&&... comps){(comps.removeWaiting(),...);}), m_components);
    }

private:
    EntityRegistryT &m_entity_registry;

    std::unordered_map<std::type_index, std::shared_ptr<SystemI>> m_systems2;
    std::tuple<ComponentHolder<ComponentTypes>...> m_components;
};

using GameSystems = ComponentWorld<TransformComponent,
                                   HealthComponent,
                                   AnimationComponent,
                                   PathComponent,
                                   CollisionComponent,
                                   TimedEventComponent,
                                   SpriteComponent
                                   >;
