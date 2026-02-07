#pragma once

#include "Utils/ObjectPool.h"

#include <functional>
#include <unordered_map>
#include <queue>

#include "FactoryBase.h"

#include "ComponentSystem.h"
#include "CollisionSystem.h"

#include "PostOffice.h"
#include "ObjectArena.h"

class ToolBoxUI;
struct Assets;
struct Player;

class GameWorld
{

public:
    GameWorld(PostOffice &messenger);

    Collisions::CollisionSystem &getCollisionSystem();
    EntityRegistryT &getEntities();
    GameObject *get(int entity_id);

    //! checks whether components that exist have existing entities
    void checkComponentsConsistency();

    GameObject &createObject(const GameObjectSpec &spec);
    std::shared_ptr<GameObject> insertObject(std::function<std::shared_ptr<GameObject>(int)> obj_maker);

    void destroyObject(int entity_id);
    GameObject &addObject3(ObjectType type);
    std::size_t getNActiveEntities(ObjectType type);

    void update(float dt);
    void draw(LayersHolder &layers, Assets &assets, Renderer &window, const View &camera_view);

    void removeParent(GameObject &child);

    ObjectArena &getArena() const;
    void registerObject(int type_id, std::size_t size);
    void *allocateObject(int type_id);
    void deallocateObject(int type_id, void *obj_p);

private:
    void addQueuedEntities();
    void removeQueuedEntities();

public:
    GameSystems m_systems;
    Player *m_player;

    Collisions::CollisionSystem m_collision_system;
    PostOffice *p_messenger = nullptr;

private:
    EntityRegistryT m_entities;

    utils::DynamicObjectPool2<int> m_root_entities;

    std::deque<std::shared_ptr<GameObject>> m_to_add;
    std::deque<std::shared_ptr<GameObject>> m_to_destroy;

    std::unordered_map<TypeId, std::unique_ptr<FactoryBase>> m_factories;

    friend ToolBoxUI;
};
