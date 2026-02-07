#pragma once

#include "ObjectRegistry.h"

#include <typeindex>
#include "Utils/Vector2.h"
#include "describe.hpp"

struct GameObjectSpec
{
    GameObjectSpec(std::type_index type_id = typeid(GameObjectSpec));

public:
    utils::Vector2f pos = {0.f};
    utils::Vector2f size = {5.f};
    utils::Vector2f vel = {0.f};
    float angle = 0.f;
    float max_speed = 100.f;
    float max_acc = 100.f;

    TypeId obj_type;

    std::type_index rtti;
};
BOOST_DESCRIBE_STRUCT(GameObjectSpec, (), (pos, size, vel, angle, max_speed, max_acc, obj_type, rtti));

/* template <class EntityType>
EntityType::Spec createSpec()
{
   typename EntityType::Spec spec;
   spec.obj_type = getTypeId<EntityType>();
   return spec; 
} */