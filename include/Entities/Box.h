#pragma once

#include "GameObject.h"

struct PlayerTouchedBox
{
    std::string text;
    std::string definition;
};


class Box : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {

        Spec() : GameObjectSpec(typeid(Spec)) {
            obj_type = TypeId::Box;
        }
        
        std::string text;
        std::string definition;
    };

public:
    Box(GameWorld &world, const Spec &spec, int ent_id);
    virtual void update(float dt)override;
    virtual void draw(LayersHolder &target, Assets &assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

    bool isTouchedByPlayer() const;

    std::string getText() const; 
    std::string getDefinition() const; 
private:
bool m_touches_wall = false;
    bool m_touches_player = false;
    std::string m_text;
    std::string m_definition;
    
    utils::Vector2f m_push_vel = {0,0};

};
