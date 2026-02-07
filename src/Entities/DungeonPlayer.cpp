#include "DungeonPlayer.h"

#include "Utils/RandomTools.h"
#include "GameWorld.h"
#include "Bullet.h"
#include "Texts.h"
#include "DrawLayer.h"
#include "Assets.h"
#include "VisualEffects.h"

#include <SDL2/SDL_keycode.h>

#include <glm/trigonometric.hpp>

DungeonPlayer::DungeonPlayer(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, spec, ent_id, TypeId::Player), m_effects_maker(world), m_cdt(*spec.cdt), m_player_vision(m_cdt)
{
    CollisionComponent c_comp;
    c_comp.type = ObjectType::Player;
    c_comp.shape.convex_shapes.emplace_back(4);
    m_world->m_systems.addEntityDelayed(getId(), c_comp);
}



void DungeonPlayer::onKeyPress(SDL_Keycode key)
{
    if ((key == SDLK_w || key == SDLK_UP) && !m_sprinting)
    {
        m_vel.y = m_speed;
        m_accelerating = true;
    }
    if ((key == SDLK_s || key == SDLK_DOWN ) && !m_sprinting)
    {
        m_vel.y = -m_speed;
        m_accelerating = true;
    }
    if ((key == SDLK_a || key == SDLK_LEFT) )
    {
        m_vel.x = -m_speed;
        m_accelerating = true;
    }
    if ((key == SDLK_d || key == SDLK_RIGHT))
    {
        m_vel.x = +m_speed;
        m_accelerating = true;
    }
}

void DungeonPlayer::onKeyRelease(SDL_Keycode key)
{
    if (key == SDLK_w || key == SDLK_UP || key == SDLK_s || key == SDLK_DOWN)
    {
        m_accelerating = false ;
        m_vel.y = 0.;
    }
    if (((key == SDLK_d || key == SDLK_RIGHT)) || ((key == SDLK_a || key == SDLK_LEFT)))
    {
        m_vel.x = 0.f;
        m_accelerating = false ;
    }
}

void DungeonPlayer::update(float dt)
{

    m_angle = utils::dir2angle(m_vel);
    m_pos += m_vel * dt;
}

void DungeonPlayer::onCreation()
{
}
void DungeonPlayer::draw(LayersHolder &layers, Assets &assets)
{
    m_player_vision.onTriangulationChange();
    auto& light_shader = layers.getCanvas("Light").getShader("Light");
    m_player_vision.constructField(getPosition(), utils::angle2dir(m_angle));
    m_player_vision.getDrawVertices(light_shader,m_verts,{1,1,1,1},200);
    layers.getCanvas("Light").drawVertices(m_verts.m_vertices, "VisionLight");

    // RectangleSimple light_rect;
    // light_rect.setPosition(m_pos);
    // light_rect.setScale(600, 600);
    // light_rect.m_color = Color{1, 1, 1, 1.0};
    // layers.getCanvas("Light").drawRectangle(light_rect, "Light");

    auto &canvas = layers.getCanvas("Unit");
    Sprite snake_part(*assets.textures.get("SnakeHead"));
    snake_part.setPosition(getPosition());
    snake_part.setRotation(utils::radians(m_angle - 90));
    snake_part.setScale(m_size);
    canvas.drawSprite(snake_part);

}



void DungeonPlayer::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (m_collision_resolvers.contains(obj.getType()))
    {
        m_collision_resolvers.at(obj.getType())(obj, c_data);
        return;
    }

    if (obj.getType() == ObjectType::TextBubble)
    {
        auto &text = static_cast<TextBubble &>(obj);
        text.m_player_is_standing = true;
        if (!text.firstTouch())
        {
            return;
        }
        text.first_touch = false;
        WordGuessedEvent wg;
        bool was_correct = static_cast<TextBubble &>(obj).isCorrect();
        wg.was_correct = was_correct;
        wg.entity_id = obj.getId();
        m_world->p_messenger->send(wg);
    }
    if (obj.getType() == ObjectType::Wall)
    {
        move(c_data.separation_axis * c_data.minimum_translation); 
    }
    if (obj.getType() == ObjectType::Box)
    {
        auto normal = c_data.separation_axis;
        auto dv_normal = utils::dot(normal, m_vel - obj.m_vel);
        if(dv_normal > 0.f)
        {
            move(c_data.separation_axis * c_data.minimum_translation); 
        }
    }
    
}

