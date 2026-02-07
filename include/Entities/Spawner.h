#pragma once

#include "Texts.h"


class Spawner : public GameObject 
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}

        float delay = 0.f;
        float period = 3.f;
        int spawn_count = -1;
        TypeId  spawned_type;
        TextBubble::Spec spawned_spec;
    };

public:
    Spawner(GameWorld &world, const Spec &spec, int ent_id = -1);
    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;

private:
    float m_period;
    float m_delay;
    int m_spawn_count = -1;
    TextBubble::Spec m_spawnee;
};
BOOST_DESCRIBE_STRUCT(Spawner::Spec, (GameObjectSpec), (period, spawned_spec, spawn_count));
