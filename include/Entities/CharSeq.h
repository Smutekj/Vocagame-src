#pragma once

#include "../GameObject.h"

class Font;

class CharSeq: public GameObject 
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}

        float life_time = 10.f;
        std::string sequence;
        Font* p_font = nullptr;
    };

public:
    CharSeq(GameWorld &world, const Spec &spec, int ent_id = -1);
    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

private:
    float m_lifetime = 10.f;
    Font* m_font = nullptr;
    std::string m_sequence;
};
BOOST_DESCRIBE_STRUCT(CharSeq::Spec, (GameObjectSpec), (life_time));
