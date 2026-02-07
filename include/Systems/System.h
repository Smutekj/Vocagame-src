#pragma once

#include "../GameObject.h"

#include "../Utils/ContiguousColony.h"
#include "../Utils/ObjectPool.h"

#include <memory>
#include <queue>

using EntityRegistryT = utils::DynamicObjectPool2<std::shared_ptr<GameObject>>;

class SystemI
{

public:
    virtual void preUpdate(float dt, EntityRegistryT& entities) = 0;
    virtual void update(float dt) = 0;
    virtual void postUpdate(float dt, EntityRegistryT& entities) = 0;
    
    virtual ~SystemI(){};
};

template <class ComponentType>
class ComponentHolder
{
public:
    ComponentType &get(int entity_id)
    {
        return m_components.get(entity_id);
    }

    utils::ContiguousColony<ComponentType, int>  &getComponents()
    {
        return m_components;
    }

    bool has(int entity_id) const
    {
        return m_components.contains(entity_id);
    }

    void addDelayed(ComponentType&& comp, int entity_id)
    {
        m_to_add.push({std::move(comp), entity_id});
    }

    void addWaiting()
    {
        while(!m_to_add.empty())
        {
            auto& [comp, id] = m_to_add.front();
            add(std::move(comp), id);
            m_to_add.pop();
        }
    }

    void removeWaiting()
    {
        while(!m_to_remove.empty())
        {
            auto id = m_to_remove.front();
            m_to_remove.pop();
            m_components.erase(id);
        }
    }
    void add(ComponentType&& comp, int entity_id)
    {
        m_components.insert(entity_id, std::move(comp));
    }

    void erase(int entity_id)
    {
        if(has(entity_id))
        {
            m_components.erase(entity_id);
        }
    }
    void eraseDelayed(int entity_id)
    {
        m_to_remove.push(entity_id);
    }

private:
    std::queue<std::pair<ComponentType, int>> m_to_add;
    std::queue<int> m_to_remove;
    
    utils::ContiguousColony<ComponentType, int> m_components;
};

// template <class ComponentType>
// class System : public SystemI
// {
// public:
//     virtual void preUpdate(float dt) override {}
//     virtual void postUpdate(float dt) override {}
//     virtual void update(float dt) override
//     {
//         auto comp_count = m_components.data.size();
//         for (std::size_t comp_id = 0; comp_id < comp_count; ++comp_id)
//         {
//             updater(m_components.data[comp_id], dt);
//         }
//     }

// private:
//     utils::ContiguousColony<ComponentType, int> m_components;

// public:
//     std::function<void(ComponentType &, float)> pre_updater;
//     std::function<void(ComponentType &, float)> updater;
//     std::function<void(ComponentType &, float)> post_updater;
// };
