#pragma once

#include <memory>
#include <deque>
#include <unordered_map>

#include <Sprite.h>
#include <Text.h>

#include "../GameObject.h"
#include "Components.h"

struct PlayerEntity;
class GameWorld;
class Animation;
class FactoryBase;
class Font;

namespace Collisions
{
    class CollisionSystem;
}

class Pickup : public GameObject
{

public:
    enum class Type
    {
        Health,
        Speed,
        TimeSlow
    };
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}

        Type type;
        int level = 0;
        float life_time = 10.f;
    };

public:
    Pickup(GameWorld &world, const Spec &spec, int ent_id = -1);

    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

private:
    Type m_type;
};
BOOST_DESCRIBE_STRUCT(Pickup::Spec, (GameObjectSpec), (type, level, life_time));



class GunBlock : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {

        Spec() : GameObjectSpec(typeid(Spec)) {}

        float cooldown = 1.5f;
        float speed = 75.f;
        float dmg = 1.f;
        float strength = 400.f;
    };

public:
    GunBlock(GameWorld &world, const Spec &spec,  int ent_id = -1);

    virtual void update(float dt) override;
    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;

public:
private:
    std::unique_ptr<FactoryBase> m_bullet_factory;

    float m_cooldown;
    float m_speed;
    float m_dmg;
    float m_strength;
};

BOOST_DESCRIBE_STRUCT(GunBlock::Spec, (GameObjectSpec), (cooldown, speed, dmg, strength));



class BoomBox : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}

        std::string texture_id = "TNT";
        float dmg = 1.f;
        float timer = 1.f;
        float explosion_radius = 100.f;
        float explosion_strength = 200.f;
    };

public:
    BoomBox(GameWorld &world, const Spec &spec, int ent_id = -1);

    virtual void update(float dt) override;
    virtual void onCreation() override;
    virtual void onDestruction() override {}
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

    void startTicking();

private:
    void explode();

    Sprite m_bomb_sprite;
    bool m_is_ticking = false;
    float m_boom_radius = 30.f;
    float m_boom_time = 2.f;
    float m_boom_strength = 200.f;
};
BOOST_DESCRIBE_STRUCT(BoomBox::Spec, (GameObjectSpec), (texture_id, dmg, timer, explosion_radius, explosion_strength));
