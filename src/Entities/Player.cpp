#include "Player.h"
#include "Pickup.h"

#include "DrawLayer.h"
#include "Assets.h"

#include "Utils/RandomTools.h"
#include "SoundSystem.h"
#include "GameWorld.h"
#include "Texts.h"

#include <nlohmann/json.hpp>
#include <fstream>

float modFloat(float x, float period)
{
    return x - std::floor(x / period) * period;
}
float sawFloat(float x, float period)
{
    return abs(x - std::floor(x / period) * period - period / 2.f);
}
template <class T>
T interpolate(T v1, T v2, float x)
{
    return v1 + (v2 - v1) * x;
}

float clamp(float x, float lowerlimit = 0.0f, float upperlimit = 1.0f)
{
    if (x < lowerlimit)
        return lowerlimit;
    if (x > upperlimit)
        return upperlimit;
    return x;
}

float smoothstep(float edge0, float edge1, float x)
{
    // Scale, and clamp x to 0..1 range
    x = clamp((x - edge0) / (edge1 - edge0));
    return x * x * (3.0f - 2.0f * x);
}
Player::Player(GameWorld *world, int ent_id)
    : GameObject(world, ent_id, ObjectType::Player),
      m_vertex_colors(9, Color{1, 1, 2, 1}),
      m_verts(9)
{
    m_max_acc = 35.f;
    m_max_vel = 120.f;

    int n_verts = m_verts.size();
    float width = 20.f;
    Vertex v;
    float angle = 90;
    float d_angle = 360. / n_verts;
    for (int i = 0; i < n_verts; ++i)
    {
        utils::Vector2f pos = utils::angle2dir(angle);
        m_verts[i] = Vertex{.pos = pos};
        angle += d_angle;
    }
}

void Player::switchColorAnim(Player::ColorAnim anim)
{
    if (m_active_color_anim_id != -1)
    {
        m_timers.removeEvent(m_active_color_anim_id);
    }

    std::function<void(float, int)> animation = [](float t, int c) {};

    using Anim = Player::ColorAnim;
    if (anim == Anim::Breathing)
    {
        float period = 3.;
        animation = [=, this, init_colors = m_vertex_colors](float t, int c)
        {
            float tp = sawFloat(t, period);
            float time_mask = smoothstep(0, period / 2.f, t);
            float time_maskp = (smoothstep(0, period / 2.f, tp) - smoothstep(period / 2.f, period, tp));
            for (int i = 0; i < m_vertex_colors.size(); ++i)
            {
                auto start_color = interpolate(init_colors[i], Color{2., 2., 2., 1}, time_mask);
                m_vertex_colors[i] = interpolate(start_color, Color{1, 1, 6, 1}, time_maskp);
            }
        };
    }
    else if (anim == Anim::Success)
    {
        float wave_duration = 1.;
        float space_factor = 12.f;
        animation = [this, wave_duration, space_factor, init_colors = m_vertex_colors](float t, int c)
        {
            for (int i = 0; i < m_vertex_colors.size(); ++i)
            {
                float tp = t / wave_duration;

                float x = (float(i) / m_vertex_colors.size()) * space_factor;

                float exponent = sawFloat(x - tp * space_factor, space_factor);
                float exponent_back = sawFloat(x + tp * space_factor, space_factor);

                float wave_forward = 15 * std::exp(-(exponent * exponent));
                float wave_backward = 15 * std::exp(-(exponent_back * exponent_back));

                m_vertex_colors[i].g = wave_forward + 1.;
                m_vertex_colors[i].g += wave_backward + 1.;
            }
            if (t > wave_duration)
            {
                switchColorAnim(Anim::Breathing);
            }
        };
    }
    else if (anim == Anim::Failure)
    {
        float wave_duration = 1.;
        float space_factor = 12.f;
        animation = [this, wave_duration, space_factor](float t, int c)
        {
            for (int i = 0; i < m_vertex_colors.size(); ++i)
            {
                float tp = t / wave_duration;

                float x = (float(i) / m_vertex_colors.size()) * space_factor;

                float exponent = sawFloat(x - tp * space_factor, space_factor);
                float exponent_back = sawFloat(x + tp * space_factor, space_factor);

                float wave_forward = 15 * std::exp(-(exponent * exponent));
                float wave_backward = 15 * std::exp(-(exponent_back * exponent_back));

                m_vertex_colors[i].r = wave_forward + 1.;
                m_vertex_colors[i].r += wave_backward + 1.;
            }
            if (t > wave_duration)
            {
                switchColorAnim(Anim::Breathing);
            }
        };
    }
    m_active_color_anim_id = m_timers.addInfiniteEvent(animation, 0.f, 0.f);
}

void Player::switchEyeAnim(Player::EyeAnim anim)
{

    if (m_active_eye_anim_id != -1)
    {
        m_timers.removeEvent(m_active_eye_anim_id);
    }

    std::function<void(float, int)> eye_animation = [](float t, int c) {};

    float base_eye_y_pos = m_size.x / 7.f;
    float base_eye_x_disp = 0.f;
    float base_eye_distance = m_size.x / 6.f;
    Color base_eye_color = Color{6, 6, 0, 1};
    Vec2 base_eye_size = m_size.x / 6.f;

    float init_x_disp = eye_x_disp;
    float init_y = eye_y_pos;
    float init_eye_distance = eye_distance;
    Color init_eye_color = eye_color;
    Vec2 init_size = eye_size;

    using Anim = Player::EyeAnim;
    if (anim == Anim::Jumping)
    {
        float wave_duration = 1.f;
        eye_animation = [=, this](float t, int c)
        {
            float time_mask = (smoothstep(0, wave_duration / 2.f, t) - smoothstep(wave_duration / 2.f, wave_duration, t));

            eye_x_disp = interpolate(init_x_disp, base_eye_x_disp, time_mask);
            eye_distance = interpolate(init_eye_distance, base_eye_distance - m_size.x / 10.f, time_mask);
            eye_y_pos = interpolate(init_y, base_eye_y_pos + m_size.x / 10.f, time_mask);
            eye_size = interpolate(init_size, base_eye_size + Vec2{-m_size.y / 20.f, +m_size.y / 10.f}, time_mask);
            if (t > wave_duration)
            {
                switchEyeAnim(Anim::Breathing);
            }
        };
    }
    else if (anim == Anim::Breathing)
    {
        float period = 3.f;
        eye_animation = [=, this](float t, int c)
        {
            float tp = sawFloat(t, period);
            float time_maskp = (smoothstep(0, period / 2.f, tp) - smoothstep(period / 2.f, period, tp));
            float time_mask = smoothstep(0, period / 2.f, t);

            eye_size = interpolate(base_eye_size, base_eye_size + Vec2{m_size.y / 15.f}, time_maskp);
            eye_distance = interpolate(init_eye_distance, base_eye_distance, time_mask);
            eye_x_disp = interpolate(init_x_disp, base_eye_x_disp, time_mask);
            eye_y_pos = interpolate(init_y, base_eye_y_pos, time_mask);
            eye_color = interpolate(interpolate(init_eye_color, base_eye_color, time_mask), Color{20, 3, 0, 1}, time_mask * time_maskp);
        };
    }
    else if (anim == Anim::RunningRight)
    {
        float period = 0.2;
        eye_animation = [=, this](float t, int c)
        {
            float time_mask = smoothstep(0, period, t);
            float x_disp = 1. + 0.5 * time_mask * sawFloat(t, 0.5);

            eye_y_pos = init_y;
            eye_x_disp = interpolate(init_x_disp, m_size.x / 6.f * x_disp, time_mask);
            eye_distance = interpolate(init_eye_distance, m_size.x / 8.f, time_mask);
            eye_color = interpolate(init_eye_color, Color{0, 25, 0, 1}, time_mask);
        };
    }
    else if (anim == Anim::StoppingRight)
    {
        float period = 0.2;
        eye_animation = [=, this](float t, int c)
        {
            float time_mask = smoothstep(0, period, t);
            eye_x_disp = interpolate(init_x_disp, 0.f, time_mask);
            eye_distance = interpolate(init_eye_distance, m_size.x / 6.f, time_mask);
            eye_color = interpolate(init_eye_color, base_eye_color, time_mask);
            if (t > period)
            {
                switchEyeAnim(Anim::Breathing);
            }
        };
    }
    else if (anim == Anim::RunningLeft)
    {
        float period = 0.2;
        eye_animation = [=, this](float t, int c)
        {
            float time_mask = smoothstep(0, period, t);
            eye_y_pos = init_y;
            eye_x_disp = interpolate(init_x_disp, -m_size.x / 10.f, time_mask);
            eye_distance = interpolate(init_eye_distance, m_size.x / 8.f, time_mask);
            eye_color = interpolate(init_eye_color, Color{0, 15, 0, 1}, time_mask);
        };
    }
    else if (anim == Anim::StoppingLeft)
    {
        float period = 0.2;
        eye_animation = [=, this](float t, int c)
        {
            float time_mask = smoothstep(0, period, t);
            eye_x_disp = interpolate(init_x_disp, 0.f, time_mask);
            eye_distance = interpolate(init_eye_distance, m_size.x / 6.f, time_mask);
            eye_color = interpolate(init_eye_color, base_eye_color, time_mask);
            if (t > period)
            {
                switchEyeAnim(Anim::Breathing);
            }
        };
    }

    m_active_eye_anim_id = m_timers.addInfiniteEvent(eye_animation, 0.f, 0.f);
}

void Player::switchHullAnim(Player::HullAnim anim)
{
    if (m_active_hull_anim_id != -1)
    {
        m_timers.removeEvent(m_active_hull_anim_id);
    }

    auto init_verts = m_verts;
    auto base_verts = m_verts;
    float angle = 90;
    for (int i = 0; i < m_verts.size(); ++i)
    {
        base_verts[i].pos = utils::angle2dir(angle);
        angle += 360. / m_verts.size();
    }

    std::function<void(float, int)> hull_anim = [](float t, int c) {};
    using Anim = Player::HullAnim;
    if (anim == Anim::Breathing)
    {
        float period = 3.f;
        hull_anim = [=, this](float t, int c)
        {
            float tp = modFloat(t, period);
            float time_maskp = smoothstep(0, period / 2., tp) - smoothstep(period / 2., period - 0.01, tp);
            float time_mask = smoothstep(0, period / 2., t);

            int n_verts = m_verts.size();
            for (int i = 0; i < n_verts; ++i)
            {
                auto start_pos = interpolate(init_verts[i].pos, base_verts[i].pos, time_mask);
                utils::Vector2f final_pos = {base_verts[i].pos.x * (1.25), base_verts[i].pos.y * (1.1)};
                m_verts[i].pos.x = interpolate(start_pos.x, final_pos.x, time_maskp);
               
                //! we need to interpolate the bottom verts to the base state before we start periodic motion
                if ((i != 4 && i != 5) || t < period/2.f)
                {
                    m_verts[i].pos.y = interpolate(start_pos.y, final_pos.y, time_maskp);
                }
            };
        };
    }
    else if (anim == Anim::Jumping)
    {
        float duration = 1.f;
        hull_anim = [=, this](float t, int c)
        {
            float tp = t / duration;
            float time_mask = smoothstep(0, duration / 2., t) - smoothstep(duration / 2., duration - 0.01, t);

            int n_verts = m_verts.size();
            float space_factor = 12.f;
            float angle = 90;
            float d_angle = 360. / n_verts;
            for (int i = 0; i < n_verts; ++i)
            {
                utils::Vector2f pos = utils::angle2dir(angle);
                m_verts[i].pos.x = interpolate(init_verts[i].pos.x, pos.x * 0.8f, time_mask);
                if (i != 4 && i != 5)
                {
                    m_verts[i].pos.y = interpolate(init_verts[i].pos.y, pos.y * 1.5f, time_mask);
                }
                angle += d_angle;
            };
            if (t > duration)
            {
                //! get back to either moving or breathing if no movement
                if (!m_moving_left && !m_moving_right)
                {
                    switchHullAnim(HullAnim::Breathing);
                }
                else
                {
                    switchHullAnim(m_moving_left ? HullAnim::RunningLeft : HullAnim::RunningRight);
                }
            }
        };
    }
    else if (anim == Anim::RunningLeft)
    {

        float start_time = 0.2f;
        hull_anim = [=, this, dir = -1](float t, int c)
        {
            float time_mask = smoothstep(0, start_time, t);
            float x_disp = 0.25 * dir + 0.6 * time_mask * sawFloat(t, 0.5);

            int n_verts = m_verts.size();
            float space_factor = 12.f;
            float angle = 90;
            float d_angle = 360. / n_verts;

            for (int i = 0; i < n_verts; ++i)
            {
                utils::Vector2f pos = utils::angle2dir(angle);
                float y = (pos.y + 1.) / 1.5f;
                if (i != 4 && i != 5)
                {
                    float new_pos = pos.x + y * x_disp;
                    m_verts[i].pos.x = interpolate(init_verts[i].pos.x, new_pos, time_mask);
                }
                angle += d_angle;
            };
        };
    }
    else if (anim == Anim::RunningRight)
    {
        float start_time = 0.2f;
        hull_anim = [=, this, dir = 1](float t, int c)
        {
            float time_mask = smoothstep(0, start_time, t);
            float x_disp = 0.25 * dir + 0.6 * time_mask * sawFloat(t, 0.5);

            int n_verts = m_verts.size();
            float space_factor = 12.f;
            float angle = 90;
            float d_angle = 360. / n_verts;
            for (int i = 0; i < n_verts; ++i)
            {
                utils::Vector2f pos = utils::angle2dir(angle);
                if (i != 4 && i != 5)
                {
                    float y = (pos.y + 1.) / 1.5f;
                    float new_pos = pos.x + y * x_disp;
                    m_verts[i].pos.x = interpolate(init_verts[i].pos.x, new_pos, time_mask);
                }
                angle += d_angle;
            };
        };
    }
    else if (anim == Anim::StoppingRight)
    {
        float start_time = 0.2f;
        hull_anim = [this, start_time, dir = 1, init_verts = m_verts](float t, int c)
        {
            float time_mask = smoothstep(0, start_time, t);
            int n_verts = m_verts.size();
            float space_factor = 12.f;
            float angle = 90;
            float d_angle = 360. / n_verts;
            for (int i = 0; i < n_verts; ++i)
            {
                utils::Vector2f pos = utils::angle2dir(angle);
                if (i != 4 && i != 5)
                {
                    m_verts[i].pos.x = interpolate(init_verts[i].pos.x, pos.x, time_mask);
                }
                angle += d_angle;
            };
            if (t > start_time)
            {
                switchHullAnim(Anim::Breathing);
            }
        };
    }
    else if (anim == Anim::StoppingLeft)
    {
        float start_time = 0.2f;
        hull_anim = [this, start_time, dir = -1, init_verts = m_verts](float t, int c)
        {
            float time_mask = smoothstep(0, start_time, t);

            int n_verts = m_verts.size();
            float space_factor = 12.f;
            float angle = 90;
            float d_angle = 360. / n_verts;
            for (int i = 0; i < n_verts; ++i)
            {
                utils::Vector2f pos = utils::angle2dir(angle);
                if (i != 4 && i != 5)
                {
                    m_verts[i].pos.x = interpolate(init_verts[i].pos.x, pos.x, time_mask);
                }
                angle += d_angle;
            };
            if (t > start_time)
            {
                switchHullAnim(Anim::Breathing);
            }
        };
    }
    m_active_hull_anim_id = m_timers.addInfiniteEvent(hull_anim, 0.f, 0.f);
}

void Player::onRunStart(int dir)
{
    if (!m_moving_left && dir < 0)
    {
        switchEyeAnim(EyeAnim::RunningLeft);
        switchHullAnim(HullAnim::RunningLeft);
    }
    if (!m_moving_right && dir > 0)
    {
        switchEyeAnim(EyeAnim::RunningRight);
        switchHullAnim(HullAnim::RunningRight);
    }
    m_moving_left = dir < 0;
    m_moving_right = dir > 0;
};

void Player::onRunEnd(int dir)
{
    m_moving_right = false;
    m_moving_left = false;
    switchEyeAnim(dir > 0 ? EyeAnim::StoppingRight : EyeAnim::StoppingLeft);
    switchHullAnim(dir > 0 ? HullAnim::StoppingRight : HullAnim::StoppingLeft);
};

void Player::onJump()
{
    switchHullAnim(HullAnim::Jumping);
    // switchEyeAnim(EyeAnim::Jumping);

    // if(m_jump_count <= 1)
    {
        SoundSystem::play("JumpShort");
        m_jump_count++;
        m_vel.y = m_jump_vel;
        // m_jump_on_cooldown = true;
        m_is_jumping = true;
        m_is_falling = false;
        for (auto &platform_id : m_touching_platforms_ids)
        {
            // static_cast<TextBubble&>(*m_world->get(platform_id)).m_player_is_standing = false;
        }
        m_touching_platforms_ids.clear();
        // m_platform_vel *= 0.f;
    }
}

void Player::update(float dt)
{
    m_timers.update(dt);

    if (m_jump_count >= 1)
    {
        m_jump_timer -= dt;
        if (m_jump_timer <= 0.f)
        {
            // m_jump_count = 0;
            m_jump_timer = m_jump_cooldown;
        }
    }

    if (m_moving_left)
    {
        //! slower turning when shooting laser
        m_vel.x -= (5000. * dt);
        m_vel.x = std::max(-m_move_speed, m_vel.x);
        setAngle(180.f);
    }
    if (m_moving_right)
    {
        m_vel.x += (5000. * dt);
        m_vel.x = std::min(m_move_speed, m_vel.x);
        setAngle(0.f);
    }

    m_vel.y -= (m_gravity + (m_gravity_falling - m_gravity) * m_is_falling) * dt;
    m_vel.y = std::max(m_vel.y, -m_fall_speed_limit);
    if (m_vel.y < 0.f && m_is_jumping)
    {
        m_is_jumping = false;
        m_is_falling = true;
    }

    //! receive platform vel
    if (m_standing_platform)
    {
        // m_vel.x
    }
    m_pos += (m_vel + m_impulse_vel) * dt;
    //! speed fallout
    m_vel.x -= m_slow_factor * m_vel.x * dt;
    // m_rotation -= utils::degrees((m_vel.x + m_impulse_vel.x) / m_size.x * dt);
    m_impulse_vel *= std::max(0.f, 1.f - 1.5f * dt);
}

void Player::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (obj.getType() == ObjectType::Wall ||
        obj.getType() == ObjectType::BoomBox ||
        obj.getType() == ObjectType::PathingWall)
    {
        auto surface_norm = c_data.separation_axis;
        auto surface_trav = {surface_norm.y, -surface_norm.x};
        auto rel_vel = m_vel - obj.m_vel;
        float v_in_norm = utils::dot(surface_norm, rel_vel);
        if (v_in_norm < 0.f)
        {
            if (m_is_falling)
            {
                SoundSystem::play("Kick", 200);
            }
            m_is_falling = false;
            m_is_jumping = false;

            if (m_standing_platform_id != obj.getId())
            {
                m_standing_platform_id = obj.getId();
                // m_vel += obj.m_vel;
            }

            move(surface_norm * c_data.minimum_translation);
            m_vel -= v_in_norm * surface_norm;
            m_platform_vel = obj.m_vel;
            //! reset jump cd
            m_jump_count = 0;
        }
    }

    if (obj.getType() == TypeId::TextPlatform || obj.getType() == TypeId::TextBubble)
    {
    }

    if (obj.getType() == TypeId::TextPlatform)
    {
        auto &text = static_cast<TextBubble &>(obj);
        auto surface_norm = c_data.separation_axis;
        auto rel_vel = m_vel - obj.m_vel;
        float v_in_norm = utils::dot(surface_norm, rel_vel);
        //! player stands on top
        if (surface_norm.y > 0.f && v_in_norm <= 0.f)
        {
            move(surface_norm * c_data.minimum_translation);
            m_vel -= v_in_norm * surface_norm;
            if (text.firstTouch())
            {
                m_world->p_messenger->send<WordGuessedEvent>(

                    {.entity_id = text.getId(),
                     .translation = text.getTranslation(),
                     .shown_form = text.getText(),
                     .correct_form = text.getCorrectForm(),
                     .was_correct = text.isCorrect()});
                switchColorAnim(text.isCorrect() ? ColorAnim::Success : ColorAnim::Failure);
            }

            text.first_touch = false;
        }
    }
    if (obj.getType() == TypeId::TextBubble)
    {
        auto &text = static_cast<TextBubble &>(obj);
        m_jump_count = 0;
        //! do not let player through if incorrect
        auto surface_norm = c_data.separation_axis;
        auto rel_vel = m_vel - obj.m_vel;
        float v_in_norm = utils::dot(surface_norm, rel_vel);
        move(surface_norm * c_data.minimum_translation);
        if (v_in_norm < 0.f && !text.isCorrect())
        {
            m_vel -= v_in_norm * surface_norm;
            //! throw player away!
            if (!text.isCorrect() && text.firstTouch())
            {
                auto dr = m_pos - obj.getPosition();
                dr /= utils::norm(dr);
                health--;
                m_impulse_vel += dr * 500.f;
            }
        }

        if (text.firstTouch())
        {
            m_world->p_messenger->send<WordGuessedEvent>(

                {.entity_id = text.getId(),
                 .translation = text.getTranslation(),
                 .shown_form = text.getText(),
                 .correct_form = text.getCorrectForm(),
                 .was_correct = text.isCorrect()});
            switchColorAnim(text.isCorrect() ? ColorAnim::Success : ColorAnim::Failure);
        }
        text.first_touch = false;
    }
    //! allow movement from bottom but not from top
}

void Player::drawOctagon(utils::Vector2f center, float radius, float rotation, Color color, Renderer &canvas)
{
    int n_verts = 9;

    float width = 10.f;
    std::vector<Vertex> v_array(9 * 6);
    Vertex v;

    RectangleSimple line;
    line.m_color = color;
    float angle = -90 + rotation;
    float d_angle = 360. / n_verts;
    float line_len = radius * std::sin(utils::radians(d_angle / 2.)) * 2.;
    float dist_to_center = radius * std::cos(utils::radians(d_angle / 2.));
    for (int i = 0; i < 9; ++i)
    {
        int i_prev = (i + n_verts - 1) % n_verts;

        auto pos_l = center + m_verts[i_prev].pos * radius;
        auto pos_r = center + m_verts[i].pos * radius;
        auto pos_lw = center + m_verts[i_prev].pos * (radius - width);
        auto pos_rw = center + m_verts[i].pos * (radius - width);
        v_array[i * 6 + 0] = Vertex{.pos = pos_l, .color = m_vertex_colors[i_prev], .tex_coord = {0., 1.}};
        v_array[i * 6 + 1] = Vertex{.pos = pos_r, .color = m_vertex_colors[i], .tex_coord = {1., 1.}};
        v_array[i * 6 + 2] = Vertex{.pos = pos_rw, .color = m_vertex_colors[i], .tex_coord = {1., 0.}};
        v_array[i * 6 + 3] = Vertex{.pos = pos_rw, .color = m_vertex_colors[i], .tex_coord = {1., 0.}};
        v_array[i * 6 + 4] = Vertex{.pos = pos_lw, .color = m_vertex_colors[i_prev], .tex_coord = {0., 0.}};
        v_array[i * 6 + 5] = Vertex{.pos = pos_l, .color = m_vertex_colors[i_prev], .tex_coord = {0., 1.}};

        // canvas.drawRectangle(line, "gradientX");
        angle += d_angle;
    }

    canvas.drawVertices(v_array, "gradientY");
}

void Player::draw(LayersHolder &layers, Assets &assets)
{

    auto &target = layers.getCanvas("Unit");

    m_player_shape.setPosition(m_pos);
    m_player_shape.setScale(m_size / 2.f);
    m_player_shape.setRotation(utils::radians(m_angle));
    m_player_shape.setTexture(*assets.textures.get("PlayerShip"));
    // target.drawSprite(m_player_shape);

    drawOctagon(m_pos, m_size.x / 2., m_rotation, {1, 1, 1, 1}, layers.getCanvas("Bloom"));
    //! draw eyes
    Sprite eye_s(*assets.textures.get("PlayerShip"));
    RectangleSimple eye;
    utils::Vector2f left_eye_pos = {m_pos.x - eye_distance / 2.f + eye_x_disp, m_pos.y + eye_y_pos};
    utils::Vector2f right_eye_pos = {m_pos.x + eye_distance / 2.f + eye_x_disp, m_pos.y + eye_y_pos};
    eye.setPosition(left_eye_pos);
    eye.setScale(eye_size);
    eye.m_color = eye_color;

    layers.getCanvas("Bloom").drawRectangle(eye, "eye");
    eye.setPosition(right_eye_pos);
    layers.getCanvas("Bloom").drawRectangle(eye, "eye");
}

void Player::onCreation()
{
    eye_y_pos = m_size.x / 7.f;
    eye_distance = m_size.x / 6.f;
    eye_size = m_size.x / 6.f;

    switchHullAnim(HullAnim::Breathing);
    switchEyeAnim(EyeAnim::Breathing);
    switchColorAnim(ColorAnim::Breathing);

    CollisionComponent c_comp;
    Polygon shape = {0};
    for (auto &v : m_verts)
    {
        shape.points.push_back(v.pos);
    }
    c_comp.shape.convex_shapes.push_back(shape);
    c_comp.type = ObjectType::Player;

    HealthComponent h_comp = {100, 100, 1.};
    // AnimationComponent a_comp = {.id = AnimationId::FrontShield2, .cycle_duration = 0.25f};
    TimedEventComponent t_comp;

    m_world->m_systems.addEntity(getId(), c_comp, h_comp, t_comp);
}

void Player::onDestruction()
{
    GameObject::onDestruction();
}

float Player::getHp() const
{
    if (m_world->m_systems.has<HealthComponent>(getId()))
    {
        return m_world->m_systems.get<HealthComponent>(getId()).hp;
    }
    return 0.;
}

float Player::getHpRatio() const
{
    if (m_world->m_systems.has<HealthComponent>(getId()))
    {
        auto &comp = m_world->m_systems.get<HealthComponent>(getId());
        return comp.hp / comp.max_hp;
    }
    return 0.;
}
