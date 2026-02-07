#pragma once

#include <functional>
#include <unordered_map>

#include "ObjectRegistry.h"
#include "GameObjectSpec.h"

class GameWorld;
class TextureHolder;
class LayersHolder;
struct Assets;

struct CollisionData
{
    utils::Vector2f separation_axis;
    float minimum_translation = -1;
    bool belongs_to_a = true;
    utils::Vector2f contact_point = {0, 0};
};

enum class EffectType
{
    ParticleEmiter,
    AnimatedSprite,
};

struct PlayerEntity;
namespace Collisions
{
    class CollisionSystem;
}

struct Transform2D
{
    utils::Vector2f trans;
    utils::Vector2f scale;
    float angle;           //! degrees!
    utils::Vector2f pivot; //! degrees!
};

class GameObject
{

public:
    GameObject(GameWorld *world, const GameObjectSpec& spec, int id, ObjectType type);
    GameObject(GameWorld *world, int id, ObjectType type);

    virtual ~GameObject() = default;

    virtual void update(float dt);
    virtual void onCreation();
    virtual void onDestruction();
    virtual void draw(LayersHolder &target, Assets& assets) ;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data);

    void kill();
    bool isDead() const;

    void updateAll(float dt);

    const utils::Vector2f getPosition() const;
    void setPosition(utils::Vector2f new_position);
    void move(utils::Vector2f by);

    float getAngle() const;
    void setAngle(float angle);

    int getId() const;
    ObjectType getType() const;

    void setSize(utils::Vector2f size);
    const utils::Vector2f &getSize() const;

    void setDestructionCallback(std::function<void(int, ObjectType)> callback);
    
    bool isRoot() const;
    void addChild(GameObject *child);
    void removeChild(GameObject *child);
    bool isParentOf(GameObject *child) const;

public:
    utils::Vector2f m_vel = {0, 0};
    utils::Vector2f m_acc = {0, 0};
    float m_max_vel = 70.f;
    float m_max_acc = 250.f;


    std::vector<GameObject *> m_children;
    GameObject *m_parent = nullptr;

    std::unordered_map<ObjectType, std::function<void(GameObject &, CollisionData &)>> m_collision_resolvers;
    utils::Vector2f m_pivot = {0.f, 0.f};

protected:
    int m_id;

    //! transform data
    Transform2D m_transform;

    utils::Vector2f m_pos;
    float m_angle = 0;
    utils::Vector2f m_size = {1, 1};

    GameWorld *m_world;

    bool m_is_dead = false;

private:
    std::function<void(int, ObjectType)> m_on_destruction_callback = [](int, ObjectType) {};

    ObjectType m_type;
};