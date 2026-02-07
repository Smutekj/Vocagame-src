#pragma once

#include <deque>

#include <VertexArray.h>
#include <Triangulation.h>

#include "GameObject.h"
#include "EffectsFactory.h"
#include "VisibilityField.h"

using SDL_Keycode = int;

class DungeonPlayer : public GameObject
{
    public:
    enum class HorizDirection 
    {
        Left = -1,
        None = 0,
        Right = 1
    };
    enum class VertDirection
    {
        Down = -1,
        None = 0,
        Up  = 1,
    };

public:
    struct Spec : public GameObjectSpec
    {

        Spec() : GameObjectSpec(typeid(Spec))
        {
            obj_type = TypeId::Player;
        }

        cdt::Triangulation<cdt::Vector2i> *cdt;
    };

public:
    DungeonPlayer(GameWorld &world, const Spec &spec, int ent_id = -1);
    virtual void onCreation() override;
    virtual void update(float dt) override;
    virtual void draw(LayersHolder &target, Assets &assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

    //! should i refactor these?
    void onKeyPress(SDL_Keycode keysym);
    void onKeyRelease(SDL_Keycode key);

private:
    bool m_sprinting = false;
    bool m_accelerating = false;
    bool m_is_turning_right = false;
    bool m_is_turning_left = false;

public:
    float m_sprint_speedup = 1.69; // nice

    HorizDirection m_horiz_dir = HorizDirection::None;
    VertDirection m_vert_dir = VertDirection::Up;

private:
    float m_speed = 200.f;
    EffectsFactory m_effects_maker;

    cdt::Triangulation<cdt::Vector2i> &m_cdt;
    VertexArray m_verts;
    VisionField m_player_vision;
};
