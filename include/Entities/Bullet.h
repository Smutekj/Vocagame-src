#pragma once

#include "../GameObject.h"
#include "Components.h"

class Bullet : public GameObject 
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}

        float life_time = 10.f;
        float punch_strength = 100.f;
        SpriteSpec sprite; 
    };

public:
    Bullet(GameWorld &world, const Spec &spec, int ent_id = -1);
    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

private:
    float m_lifetime = 10.f;
    float m_punch_strength = 100.f;
};
BOOST_DESCRIBE_STRUCT(Bullet::Spec, (GameObjectSpec), (life_time, punch_strength));
