#include "GameWorld.h"

#include <chrono>

#include "Utils/RandomTools.h"

#include "Factories.h"

//! EXPERIMENTAL ARENA FOR STORING OBJECTS!
ObjectArena arena;

ObjectArena &GameWorld::getArena() const
{
    return arena;
}
void GameWorld::registerObject(int type_id, std::size_t size)
{
    arena.registerObject(type_id, size);
}
void *GameWorld::allocateObject(int type_id)
{
    return arena.allocateObject(type_id);
}
void GameWorld::deallocateObject(int type_id, void *obj_p)
{
    return arena.deallocateObject(type_id, obj_p);
}


template <class... EntityTypes>
std::unordered_map<TypeId, std::unique_ptr<FactoryBase>> createFactories(GameWorld &world)
{
    std::unordered_map<TypeId, std::unique_ptr<FactoryBase>> factories;
    ((factories[getTypeId<EntityTypes>()] = std::make_unique<EntityFactory2<EntityTypes>>(world)), ...);
    return factories;
}

GameWorld::GameWorld(PostOffice &messenger)
    : p_messenger(&messenger), m_systems(m_entities), m_collision_system(messenger, m_systems.getComponents<CollisionComponent>())
{
    m_factories = createFactories<TYPE_LIST>(*this);
    registerSerializers();
}

std::size_t GameWorld::getNActiveEntities(ObjectType type)
{
    auto &entities = m_entities.data();
    auto n_enemies = std::count_if(entities.begin(), entities.end(), [type](auto &obj)
                                   { return obj->getType() == type; });
    return n_enemies;
}

std::shared_ptr<GameObject> GameWorld::insertObject(std::function<std::shared_ptr<GameObject>(int)> obj_maker)
{
    auto new_obj = obj_maker(m_entities.reserveIndexForInsertion());
    m_to_add.push_back(new_obj);
    return new_obj;
}

GameObject &GameWorld::addObject3(ObjectType type)
{
    auto maker = [this, type](int id)
    {
        return std::make_shared<GameObject>(this, id, type);
    };
    auto entity_p = insertObject(maker);
    return *entity_p;
}

void GameWorld::addQueuedEntities()
{
    while (!m_to_add.empty())
    {
        auto new_object = m_to_add.front();
        auto new_id = new_object->getId(); //! the object already has id because we reserved it
        m_entities.insertAt(new_id, new_object);
        assert(new_id == m_entities.at(new_id)->getId());

        p_messenger->send(EntityCreatedEvent{new_id, new_object->getPosition()});

        m_entities.at(new_id)->onCreation();
        if (m_systems.has<CollisionComponent>(new_id))
        {
            m_collision_system.insertObject(*m_entities.at(new_id));
        }

        if (m_entities.at(new_id)->isRoot())
        {
            m_root_entities.insertAt(new_id, new_id);
        }

        m_to_add.pop_front();
    }
}

void removeEntity(GameObject *entity,
                  GameSystems &systems,
                  EntityRegistryT &entities,
                  utils::DynamicObjectPool2<int> &root_entities,
                  Collisions::CollisionSystem &collision_system)
{
    auto id = entity->getId();
    entity->onDestruction();

    //! destroy also the object's children and remove it from roots if necessary
    if (entity->isRoot())
    {
        root_entities.remove(id);
        //! children become roots
        for (auto p_child : entity->m_children)
        {
            p_child->m_parent = nullptr;
            auto child_id = p_child->getId();
            assert(!root_entities.contains(child_id));
            root_entities.insertAt(child_id, child_id);
        }
    }
    else
    { //! entity has a parent so it should be removed from it's children
        entity->m_parent->removeChild(entity);
    }

    if (systems.has<CollisionComponent>(id))
    {
        collision_system.removeObject(*entity);
    }
    systems.removeEntity(id);
    entities.remove(id);
}

void GameWorld::removeParent(GameObject &child)
{
    if (!child.m_parent)
    {
        return;
    }

    //! children become roots
    for (auto p_child : child.m_parent->m_children)
    {
        p_child->m_parent = nullptr;
        auto child_id = p_child->getId();
        assert(!m_root_entities.contains(child_id));
        m_root_entities.insertAt(child_id, child_id);
    }
}
void GameWorld::removeQueuedEntities()
{

    //! we destroy the objects from children to parents, this way it doesn't get fucked
    //! TODO: The entity-tree logic should not be done in GameWorld, Add it's own thing for that
    std::deque<GameObject *> to_destroy;
    std::unordered_set<int> already_destroyed; //! for
    while (!m_to_destroy.empty())
    {
        auto object = m_to_destroy.back();
        m_to_destroy.pop_back();

        assert(!already_destroyed.contains(object->getId()));
        already_destroyed.insert(object->getId());

        to_destroy.push_front(object.get());

        for (auto p_child : object->m_children)
        {
            assert(!already_destroyed.contains(p_child->getId()));
            already_destroyed.insert(p_child->getId());

            to_destroy.push_front(p_child);
        }
    }
    m_to_destroy.clear();

    for (auto object : to_destroy)
    {
        p_messenger->send(EntityDiedEvent{object->getType(), object->getId(), object->getPosition()});
        removeEntity(object, m_systems, m_entities, m_root_entities, m_collision_system);
    }
}

void GameWorld::destroyObject(int entity_id)
{
    m_to_destroy.push_back(m_entities.at(entity_id));
}

void GameWorld::update(float dt)
{

    m_systems.preUpdate(dt);
    m_collision_system.preUpdate(dt, m_entities);
    m_systems.update(dt);
    m_systems.postUpdate(dt);

    std::deque<GameObject *> to_update;
    for (auto id : m_root_entities.data())
    {
        to_update.push_back(m_entities.at(id).get());
    }
    //! we update the entities starting from parents ending with children
    while (!to_update.empty())
    {
        auto current = to_update.front();
        to_update.pop_front();
        current->updateAll(dt);
        // current->update(dt);
        if (current->isDead())
        {
            destroyObject(current->getId());
        }

        for (auto child : current->m_children)
        {
            to_update.push_back(child);
        }
    }

    addQueuedEntities();
    removeQueuedEntities();
}

void GameWorld::checkComponentsConsistency()
{
}

void GameWorld::draw(LayersHolder &layers, Assets &assets, Renderer &window, const View &camera_view)
{
    View extended_camera = camera_view;
    extended_camera.setSize(camera_view.getSize() * 2.f);

    for (auto &obj : m_entities.data())
    {
        Rectf obj_rect;
        auto size = obj->getSize();
        obj_rect.pos_x = obj->getPosition().x - size.x / 2.f;
        obj_rect.pos_y = obj->getPosition().y - size.y / 2.f;
        obj_rect.width = size.x;
        obj_rect.height = size.y;
        if (extended_camera.intersects(obj_rect))
        {
            obj->draw(layers, assets);
        }
    }

#ifdef DEBUG
    checkComponentsConsistency();
#endif
}

GameObject &GameWorld::createObject(const GameObjectSpec &spec)
{
return m_factories.at(spec.obj_type)->create(spec);
}
    Collisions::CollisionSystem &GameWorld::getCollisionSystem()
    {
        return m_collision_system;
    }

    GameObject *GameWorld::get(int entity_id)
    {
        return m_entities.at(entity_id).get();
    }

    EntityRegistryT &GameWorld::getEntities()
    {
        return m_entities;
    }