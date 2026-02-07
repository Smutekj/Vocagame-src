#pragma once

#include "GameObject.h"
#include "Components.h"


class PlayerDungeon : public GameObject 
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {
            obj_type = TypeId::Player;
        }
    };

public:
    PlayerDungeon(GameWorld &world, const Spec &spec, int ent_id);
    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;
    virtual void update(float dt)override;

private:
    float m_lifetime = 10.f;
    float m_punch_strength = 100.f;
};
