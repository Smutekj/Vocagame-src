
#pragma once

#include "../GameObject.h"
#include "Components.h"

class Planet : public GameObject 
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {
            obj_type = TypeId::Planet;
        }

        SpriteSpec sprite;
    };

public:
    Planet(GameWorld &world, const Spec &spec, int ent_id = -1);
    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

private:
    std::string m_texture_id = "";
};
BOOST_DESCRIBE_STRUCT(Planet::Spec, (GameObjectSpec), (sprite));
