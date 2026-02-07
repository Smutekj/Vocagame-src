#pragma once

#include <concepts>
#include <type_traits>

#include "../GameWorld.h"
#include "../SoundSystem.h"

#include "Specs.h"
#include "../serializers.h"


#include "../ObjectArena.h"
#include <memory>

#include "ObjectRegistry.h"

#include "Pickup.h"
#include "Box.h"
#include "Texts.h"
#include "Walls.h"
#include "Meteor.h"
#include "CharSeq.h"
#include "Spawner.h"
#include "Trigger.h"
#include "Player.h"
#include "Bullet.h"
#include "FactoryBase.h"
#include "VisualEffects.h"
#include "Planet.h"

//! THESE MACROS Create a enum with all registered Types and connects it to types themselves!!
/*------------------- */
template <typename T>
constexpr TypeId getTypeId();
#define MAKE_ID(type) \
    template <>       \
    constexpr TypeId getTypeId<type>() { return TypeId::type; }
OBJ_LIST(MAKE_ID)
#undef MAKE_ID
/*------------------- */

template <class EntityType>
struct Deleter
{

    Deleter(ObjectArena& arena) : arena(arena){}

    void operator()(EntityType *p_obj) 
    {
        std::destroy_at(p_obj);
        int type_id = static_cast<int>(getTypeId<EntityType>());
        arena.deallocateObject(type_id, (void *)p_obj);
    }
    ObjectArena& arena;
};

template <class EntityType, typename Spec=EntityType::Spec>
std::shared_ptr<EntityType> makeObject(GameWorld& world, Spec&& spec, int ent_id)  
{
    
    return std::make_shared<EntityType>(world,std::forward<Spec>(spec), ent_id);
    // void *location = world.allocateObject(static_cast<int>(getTypeId<EntityType>()));
    // auto *p_obj = std::construct_at(static_cast<EntityType *>(location), world,  std::forward<Spec>(spec), ent_id);
    // return std::shared_ptr<EntityType>(p_obj, Deleter<EntityType>(world.getArena()));
}



template <class T>
concept EntitySpecifier =
    requires { typename T::IdType; } &&
    requires(T spec) {
        spec.type;
        { static_cast<std::size_t>(spec.type) } -> std::same_as<std::size_t>;
    } &&
    std::is_same_v<decltype(T::type), typename T::IdType>;



template <class SpecType>
concept IsSpec = std::is_base_of_v<GameObjectSpec, SpecType> &&
                 requires(SpecType s) {
                     s.rtti == typeid(s);
                 };

template <class EntityType>
concept IsRegistered = IsSpec<typename EntityType::Spec> &&
                       requires() {
                           getTypeId<EntityType>();
                       };
    
template <IsRegistered EntityType>
class EntityFactory2 : public FactoryBase
{
public:
    using Spec = EntityType::Spec;
    using FactoryFunc = std::function<GameObject &(EntityType &, const Spec &)>;
    
    static_assert(std::is_base_of_v<GameObjectSpec, Spec>);
    
public:
    EntityFactory2(GameWorld &world) : m_world(world)
    {
        world.registerObject(static_cast<int>(getTypeId<EntityType>()), sizeof(EntityType));
    }

private:
    virtual std::shared_ptr<GameObjectSpec> makeSpec() final
    {
        auto new_spec = std::make_shared<Spec>();
        new_spec->obj_type = getTypeId<EntityType>();
        return new_spec;
    }

    virtual GameObject &createImpl(const GameObjectSpec &spec) final
    {
        
        assert(spec.rtti == typeid(Spec));
        
        const auto &the_spec = static_cast<const Spec &>(spec);

        auto object_maker = [the_spec, this](int ent_id){
            return makeObject<EntityType>(m_world, the_spec, ent_id);
        };
       
        auto new_entity = std::static_pointer_cast<EntityType>(m_world.insertObject(object_maker));


        m_world.p_messenger->send<NewEntity<EntityType>>({new_entity});
        m_world.p_messenger->send<NewEntityEvent>({new_entity->getId(), new_entity});

        new_entity->setPosition(spec.pos);
        new_entity->m_vel = spec.vel;
        new_entity->setSize(spec.size);
        new_entity->setAngle(spec.angle);
        return *new_entity;
    }

protected:
    GameWorld &m_world;
};




