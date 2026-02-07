#pragma once

#include "../GameObject.h"
#include "../Systems/TimedEventManager.h"
#include "Components.h"

class Wall : public GameObject
{
    enum class Type
    {
        Static,
        Moving
    };

    BOOST_DESCRIBE_NESTED_ENUM(Type, Static, Moving);

public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}

        SpriteSpec s_sprite;
    };

public:
    Wall(GameWorld &world, const Spec &spec, int ent_id = -1, ObjectType type = ObjectType::Wall);

    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

private:
    Color m_line_color = {0.f, 7.f, 0.f, 1.f};
    SpriteSpec m_sprite_spec;
};
BOOST_DESCRIBE_STRUCT(Wall::Spec, (GameObjectSpec), (s_sprite));

class PathingWall : public GameObject
{

public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}

        SpriteSpec s_sprite;
        float speed = 100.f;
        Path path;
    };

public:
    PathingWall(GameWorld &world, const Spec &spec, int ent_id = -1);

    virtual void update(float dt) override;
    virtual void onCreation() override;
    virtual void draw(LayersHolder &layers, Assets& assets) override;

    void setPath(const Path &path, float speed);

private:
    void moveToNextPoint();

private:
    float m_speed = 50.f;
    Path m_path;
    int m_current_path_id = 0;
    int m_path_delta_id = 1; //! -1 for moving backwards
    TimedEventManager m_reached_next_point;
    Color m_line_color = {0.f, 7.f, 0.f, 1.f};
    SpriteSpec m_sprite_spec;
};
BOOST_DESCRIBE_STRUCT(PathingWall::Spec, (GameObjectSpec), (path, s_sprite));


class ElectroWall : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}
        float dmg = 1.f;
        float on_duration = 2.f;
        float off_duration = 2.f;
        ColorByte color = {255, 0, 25, 255};
    };

public:
    ElectroWall(GameWorld &world, const Spec &spec,  int ent_id = -1);

    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

private:
    bool m_is_on = true;

    ColorByte m_color = {255, 255, 255, 255};
    float m_dmg;
    float m_on_duration;
    float m_off_duration;
};
BOOST_DESCRIBE_STRUCT(ElectroWall::Spec, (GameObjectSpec), (color, dmg, on_duration, off_duration))