#pragma once

#include "GameObject.h"
#include "Vertex.h"

class Meteor : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)){
            obj_type = TypeId::Meteor;
        }
        
        float mass = 1.;
        float inertia = 1.;
        float angle_vel = 1.;
    };

public:
    Meteor(GameWorld &world, const Spec& spec, int ent_id);

    virtual void update(float dt) override;
    virtual void onCreation() override;
    virtual void onDestruction() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

    void initializeRandomMeteor(float radius);
    public:
    utils::Vector2f m_impulse_vel = {0.f};
private:

    float max_dist_from_player = 1000;
    
    float m_max_impulse_vel = 100.f;
    float m_impulse_decay = 1.5f;
    float m_ass = 1.; //! just for the keks
    float m_inertia = 1.; 
    float m_angle_vel = 0.;

    utils::Vector2f m_center_tex;
    utils::Vector2f m_center_offset;

    std::vector<Vertex> m_verts;

};
