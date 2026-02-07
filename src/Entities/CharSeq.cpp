#include "CharSeq.h"
#include "Player.h"
#include "../GameWorld.h"

#include <Text.h>
#include <Font.h>
#include <DrawLayer.h>

CharSeq::CharSeq(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, spec, ent_id, TypeId::CharSeq)
{
    m_lifetime = spec.life_time;
    m_sequence = spec.sequence;
    m_font = spec.p_font;
}

void CharSeq::onCreation()
{
    CollisionComponent c_comp;
    c_comp.type = ObjectType::CharSeq;
    c_comp.shape.convex_shapes.emplace_back(4);

    TimedEventComponent t_comp;
    t_comp.addEvent({[this](float t, int c)
                     { kill(); }, m_lifetime});

    m_world->m_systems.addEntity(getId(), c_comp, t_comp);
}
void CharSeq::draw(LayersHolder &target, Assets& assets)
{
    Text seq(m_sequence);
    seq.setFont(m_font);
    seq.centerAround(m_pos);
    seq.setColor({255,255,255,255});

    auto bb = seq.getBoundingBox();
    m_size = {bb.width, bb.height};

    target.getCanvas("Unit").drawText2(seq);

}
void CharSeq::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (obj.getType() == ObjectType::Player)
    {
        m_world->p_messenger->send(CharacterGuessEvent{.entity_id = getId(), .sequence = m_sequence});
        kill();
    }
}
