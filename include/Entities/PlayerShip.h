#pragma once

#include "GameObject.h"

#include <Sprite.h>
#include <Particles.h>

enum class BoosterState
{
    Boosting,
    Ready,
    CoolingDown,
    Disabled,
};

class GameWorld;

class PlayerShip : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec()
        {
            obj_type = ObjectType::Player;
        }
        float turn_speed = 180;
        float slow_factor = 0.5;
        float boost_factor = 1.5;
        float max_fuel = 100.;
    };

public:
    PlayerShip(GameWorld &world, const Spec &spec, int ent_id);

    virtual void update(float dt) override;
    virtual void draw(LayersHolder &target, Assets &assets) override;
    virtual void onCreation() override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

    float getHp() const;
    float getHpRatio() const;

    void activateShield();
    void deactivateShield();

    void onBoostDown();
    void onBoostUp();

private:
    void fixAngle();
    void boost(float dt);

public:
    utils::Vector2f m_impulse_vel = {0,0}; 
    float speed = 0.f;
    BoosterState booster = BoosterState::Ready;
    bool m_shocked = false;

    int m_money = 100;

    float m_laser_timer = 0.f;

    bool m_is_shooting_laser = false;
    bool m_is_turning_left = false;
    bool m_is_turning_right = false;

    bool m_passed_speed_gate = false;
    bool m_accelerating = false;
    bool m_deccelerating = false;

    float m_boost_max_speed = 150.f;
    float m_boost_factor = 2.f;
    float m_slow_boost_factor = 0.3f;
    float m_slow_factor = 1.5f;
    float acceleration = 1.5f;

    float m_angle_vel = 180.69f;

    float m_max_fuel = 100.;
    float m_fuel = 100.;

    float boost_time = 0.f;
    float max_boost_time = 100.f;

    float boost_heat = 0.f;
    float max_boost_heat = 100.f;

    float health = 100;
    float max_health = 100;

    float shield_lifetime = 5.;
    float max_shield_hp = 10;
    float shield_timeleft = shield_lifetime;
    bool shield_active = false;

    float m_deactivated_time = -1.f;

    Sprite m_player_shape;
    std::shared_ptr<Particles> m_particles_left;
    std::shared_ptr<Particles> m_particles_right;

private:
    int m_shield_id = -1;
};
