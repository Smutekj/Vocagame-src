#pragma once

#include "../GameObject.h"


class Trigger : public GameObject 
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {
            obj_type = TypeId::Trigger;
        }
    };

public:
    Trigger(GameWorld &world, const Spec &spec, int ent_id = -1);

private:
};
BOOST_DESCRIBE_STRUCT(Trigger::Spec, (GameObjectSpec), ());
