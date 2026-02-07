#include "Snake.h"

#include "Utils/RandomTools.h"
#include "GameWorld.h"
#include "Bullet.h"
#include "Texts.h"
#include "DrawLayer.h"
#include "Assets.h"
#include "VisualEffects.h"

#include <SDL2/SDL_keycode.h>

#include <glm/trigonometric.hpp>

Snake::Snake(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, spec, ent_id, TypeId::Snake), m_effects_maker(world)
{
    PastPoint start = {.pos = spec.pos, .radius = spec.turn_radius, .speed = speed, .time = spec.time};
    time_x = spec.time;
    turn_radius = spec.turn_radius;
    setPosition(spec.pos);
    past_points.push_front(start);

    //! add tail
    utils::Vector2f tail_pos = m_pos;
    float link_time = time_x;
    for (int i = 0; i < spec.tail_size; ++i)
    {
        tail_pos.x -= link_dist;
        link_time -= link_dist / speed;
        tail.push_back({.angle = m_angle, .pos = tail_pos});
    }
    past_points.push_back(PastPoint{.pos = tail_pos, .radius = spec.turn_radius, .speed = speed, .time = link_time});
}

void Snake::onEvent(SnakeClientEvent event)
{
    //! direction changed event
    if (event.type == "control" && event.dir != dir)
    {
        dir = event.dir;
        past_points.push_front({m_angle, m_pos, turn_radius, speed, time_x, dir});
    }
    if (event.type == "poop")
    {
        eat_times.push_front({(time_x + (tail.size() + 1) * link_dist / speed), time_x, m_pos, m_angle});
    }
}

void Snake::onKeyPress(SDL_Keycode key)
{
    if ((key == SDLK_w || key == SDLK_UP) && !m_sprinting)
    {
        onSprint();
        m_sprinting = true;
    }
    // if (key == SDLK_f)
    // {
    //     turn_radius += 10.f;
    //     past_points.push_front({m_angle, m_pos, turn_radius, speed, time_x, dir});
    // }
    // if (key == SDLK_g)
    // {
    //     turn_radius -= 10.f;
    //     past_points.push_front({m_angle, m_pos, turn_radius, speed, time_x, dir});
    // }
    if ((key == SDLK_a || key == SDLK_LEFT) && dir != +1)
    {
        dir = 1;
        past_points.push_front({m_angle, m_pos, turn_radius, speed, time_x, dir});
    }
    if ((key == SDLK_d || key == SDLK_RIGHT) && dir != -1)
    {
        dir = -1;
        past_points.push_front({m_angle, m_pos, turn_radius, speed, time_x, dir});
    }
    if (key == SDLK_LCTRL)
    {
        onShoot();
    }
    // if (key == SDLK_SPACE)
    // {
    //     eat_times.push_front({(time_x + (tail.size() + 1) * link_dist / speed), time_x, m_pos, m_angle});
    // }
}

void Snake::onSprint()
{
    onSpeedUpMult(m_sprint_speedup);
}
void Snake::onSprintStop()
{
    onSlowDownMult(m_sprint_speedup);
}
void Snake::onKeyRelease(SDL_Keycode key)
{
    if (key == SDLK_w || key == SDLK_UP)
    {
        onSprintStop();
        m_sprinting = false;
    }
    if (((key == SDLK_d || key == SDLK_RIGHT) && dir == -1) || ((key == SDLK_a || key == SDLK_LEFT) && dir == 1))
    {
        dir = 0;
        past_points.push_front({m_angle, m_pos, turn_radius, speed, time_x, dir});
    }
}
void Snake::onSpeedUpMult(float factor)
{
    speed *= factor;
    // time_factor *= factor;
    link_delay /= factor;
    for (auto &pp : past_points)
    {
        pp.time /= factor;
    }
    for (auto &pp : eat_times)
    {
        pp.poop_time /= factor;
        pp.time /= factor;
    }
    time_x /= factor;
    // past_points.push_front({m_angle, m_pos, turn_radius, speed, time_x, dir});
}

void Snake::onSpeedUpAdd(float speedup)
{
    float factor = (speed + speedup) / (speed);
    onSpeedUpMult(factor);
}
void Snake::onSlowDownAdd(float speed_down)
{
    float factor = (speed - speed_down) / (speed);
    onSlowDownMult(factor);
}
void Snake::onSlowDownMult(float factor)
{
    speed /= factor;
    link_delay *= factor;
    for (auto &pp : past_points)
    {
        pp.time *= factor;
    }
    for (auto &pp : eat_times)
    {
        pp.poop_time *= factor;
        pp.time *= factor;
    }

    time_x *= factor;
    // past_points.push_front({m_angle, m_pos, turn_radius, speed, time_x, dir});
}
void Snake::onShoot()
{
    Bullet::Spec spec;
    spec.pos = m_pos;
    spec.vel = 3 * speed * utils::angle2dir(m_angle);
    spec.size = {40};
    auto bullet = m_world->insertObject([this, spec](int id)
                                        { return std::make_shared<Bullet>(*m_world, spec, id); });
    bullet->m_collision_resolvers[ObjectType::TextBubble] = [this, bullet](auto &obj, auto &c_data)
    {
        auto &text = static_cast<TextBubble &>(obj);
        bullet->kill();
        bool is_correct = text.isCorrect();
        WordGuessedEvent wg;
        wg.was_correct = !is_correct;
        wg.entity_id = obj.getId();
        m_world->p_messenger->send(wg);
        if (is_correct)
        {
            // text.kill();
            m_effects_maker.create(EffectId::BlueExplosion, bullet->getPosition(), bullet->getSize() * 3.f, 0.69f);
        }
        else
        {
            m_effects_maker.create(EffectId::StarBurst, bullet->getPosition(), bullet->getSize() * 3.f, 0.69f);
        }
        TimedEventComponent t_comp;
        auto kill_obj = [&obj](float t, int c)
        { obj.kill(); };
        auto disappear_obj = [&obj](float t, int c)
        {
            auto &text = static_cast<TextBubble &>(obj);
            text.m_opacity = (3.f - t) / 3.f;
        };
        t_comp.addEvent({kill_obj, 0.f, 5.f});
        t_comp.addEvent({disappear_obj, TimedEventType::Infinite, 0.f, 2.f});
        m_world->m_systems.add(t_comp, obj.getId());
        m_world->m_systems.removeDelayed<CollisionComponent>(obj.getId());
        m_world->m_collision_system.removeObject(obj);
        text.setText(text.getCorrectForm());
        text.m_drawable.m_glow_color = is_correct ? ColorByte{255, 0, 0, 255} : ColorByte{0, 255, 0, 255};

        Transform final_trans = text.light_rect;
        final_trans.setScale(0, 0);
        transformAnimation(3.f, 2.f, text.light_rect, final_trans, text.light_rect, text.m_timers);
        text.light_rect.m_color = is_correct ? Color{5,0,0,1} : Color{0, 5, 0, 1};
    };
}

void Snake::synchronize(utils::Vector2f pos, float angle, double server_time)
{
    while (!past_points.empty() && past_points.front().time > server_time)
    {
        past_points.pop_front();
    }
    for (auto &pp : past_points)
    {
        std::cout << pp.time << " ";
    }
    time_x = server_time;
    std::cout << std::endl;
    PastPoint pp = {.angle = angle, .pos = pos, .radius = turn_radius, .speed = speed, .time = server_time, .dir = dir};
    past_points.push_front(pp);
}

void Snake::update(float dt)
{

    time_x += dt;
    if (eat_times.size() > 0 && time_x / time_factor >= eat_times.back().poop_time / time_factor)
    {
        tail.push_back({eat_times.back().angle, eat_times.back().pos});
        eat_times.pop_back();
    }

    float angle_vel = utils::degrees(speed / turn_radius);
    auto new_head = extrapolate(past_points.at(0), speed, time_factor, time_x);
    m_angle = new_head.angle;
    m_pos = new_head.pos;

    std::size_t past_point_id = 0;
    float link_time = time_x / time_factor;
    for (auto &link : tail)
    {
        link_time -= link_delay / time_factor;
        while (
            past_point_id + 1 != past_points.size() &&
            past_points.at(past_point_id).time / time_factor > link_time)
        {
            past_point_id++;
        }
        auto pp = extrapolate(past_points.at(past_point_id), speed, time_factor, link_time);
        link.angle = pp.angle;
        link.pos = pp.pos;
    }
    // //! remove past points beyond last link time because they will never be used
    for (; past_point_id < past_points.size(); ++past_point_id)
    {
        if (past_points.at(past_point_id).time / time_factor < link_time + link_delay / time_factor)
        {
            break;
        }
    }
    past_points.erase(past_points.begin() + past_point_id + 1, past_points.end());

    m_vel = utils::angle2dir(m_angle) * speed / time_factor;

    //! check if eating ass (lol)
    auto mouth_pos = m_pos + m_size.x / 2.f * utils::angle2dir(m_angle);
    for (std::size_t tail_id = 2; tail_id < tail.size(); ++tail_id)
    {
        if (utils::segmentsIntersect(mouth_pos, m_pos, tail.at(tail_id - 1).pos, tail.at(tail_id).pos))
        {
            loseTail(tail_id);
            break;
        }
    }
}

void Snake::onCreation()
{
    CollisionComponent c_comp;
    c_comp.type = ObjectType::Snake;
    c_comp.shape.convex_shapes.emplace_back(4);

    m_world->m_systems.addEntity(getId(), c_comp);
}
void Snake::draw(LayersHolder &target, Assets &assets)
{

    RectangleSimple light_rect;
    light_rect.setPosition(m_pos);
    light_rect.setScale(600, 600);
    light_rect.m_color = Color{1, 1, 1, 1.0};
    target.getCanvas("Light").drawRectangle(light_rect, "Light");

    auto &canvas = target.getCanvas("Unit");
    Sprite snake_part(*assets.textures.get("SnakeHead"));
    snake_part.setPosition(getPosition());
    snake_part.setRotation(utils::radians(m_angle - 90));
    snake_part.setScale(m_size);
    canvas.drawSprite(snake_part);

    if (tail.size() > 0)
    {
        snake_part.setTexture(*assets.textures.get("SnakeLink"));
        snake_part.setScale(m_size / 2.f);
        canvas.drawLineBatched(m_pos, tail.at(0).pos, 3.f, {0, 1, 1, 1});
    }

    std::size_t apple_id = eat_times.empty() ? -1 : 0;
    double link_time_diff = link_dist / speed;
    std::vector<std::size_t> apple_link_ids;
    for (const auto &apple : eat_times)
    {
        double d_apple_time = time_x - apple.time;
        int link_with_apple_id = std::floor(d_apple_time / link_time_diff); //! id of first link that eats apple
        apple_link_ids.push_back(link_with_apple_id);
    }

    float max_apple_scale = 0.5f;
    for (std::size_t link_id = 0; link_id < tail.size(); link_id++)
    {
        snake_part.setPosition(tail.at(link_id).pos);
        if (apple_id != -1)
        {
            double d_apple_time = time_x - eat_times.at(apple_id).time;
            int link_with_apple_id = std::floor(d_apple_time / link_time_diff); //! id of first link that eats apple
            float size_factor = (d_apple_time - link_with_apple_id * link_time_diff) / link_time_diff;

            if (link_id + 1 == link_with_apple_id)
            {
                snake_part.setScale(m_size / 2.f * (1. + max_apple_scale - max_apple_scale * size_factor));
            }
            else if (link_id == link_with_apple_id)
            {
                snake_part.setScale(m_size / 2.f * (1. + max_apple_scale * size_factor));
                apple_id++;
                if (apple_id >= eat_times.size())
                {
                    apple_id = -1;
                }
            }
            else
            {
                snake_part.setScale(m_size / 2.f);
            }
        }
        else
        {
            snake_part.setScale(m_size / 2.f);
        }
        canvas.drawSprite(snake_part);
    }
}

void Snake::loseTail(std::size_t first_lost_id)
{
    if (first_lost_id >= tail.size())
    {
        return;
    }
    std::size_t new_size = first_lost_id;

    auto createCutLink = [this](utils::Vector2f pos, utils::Vector2f link_dir)
    {
        float life_time = 5.f;

        VisualObject::Spec link_spec;
        link_spec.pos = pos;
        float start_speed = utils::randf(0, 50);
        utils::Vector2f start_dir = utils::angle2dir(utils::randf(0.f, 360.f));
        link_spec.vel = start_speed * start_dir; // Vec2{link_dir.y, -link_dir.x} / link_dist;

        link_spec.size = m_size / 2.f;
        link_spec.angle = utils::dir2angle(link_dir);
        link_spec.life_time = life_time;
        link_spec.obj_type = TypeId::VisualObject;
        link_spec.sprite.color = {255, 0, 255, 255};
        link_spec.sprite.texture_id = "SnakeLink";
        auto &shat_link = m_world->createObject(link_spec);
        auto linkSlower = [&shat_link, start_speed, start_dir, life_time](float t, int c)
        {
            float x = (life_time - t) / life_time;
            shat_link.m_vel = start_speed * (x * x * x) * start_dir;
        };
        TimedEvent slowEvent(linkSlower, TimedEventType::Infinite, 0.f, 0.f);
        m_world->m_systems.get<TimedEventComponent>(shat_link.getId()).addEvent(slowEvent);
    };

    utils::Vector2f link_dir = first_lost_id == 0 ? head_pos - tail.at(0).pos : tail.at(first_lost_id - 1).pos - tail.at(first_lost_id).pos;
    createCutLink(tail.at(first_lost_id).pos, link_dir);
    for (std::size_t link_id = first_lost_id + 1; link_id < tail.size(); ++link_id)
    {
        link_dir = tail.at(link_id).pos - tail.at(link_id - 1).pos;
        createCutLink(tail.at(link_id).pos, link_dir);
    }

    tail.resize(new_size);
}

void Snake::bounceOff(utils::Vector2f wall_norm)
{
    auto current_dir = utils::angle2dir(m_angle);
    current_dir -= 2.f * utils::dot(wall_norm, current_dir) * wall_norm;
    float new_angle = utils::dir2angle(current_dir);
    past_points.push_front(PastPoint{.angle = new_angle, .pos = m_pos, .radius = turn_radius, .speed = speed, .time = time_x});
    setAngle(new_angle);
}

void Snake::onCollisionWith(GameObject &obj, CollisionData &c_data)
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
        if (was_correct)
        {
            eat_times.push_front({time_x + (tail.size() + 1 + eat_times.size()) * link_dist / speed, time_x, m_pos, m_angle});
            onSpeedUpAdd(4.0);
            m_effects_maker.create(EffectId::StarBurst, obj.getPosition(), {1, 1}, 0.69f);
            obj.kill();
        }
        else
        {
            //! shorten the snake and slow him down
            m_effects_maker.create(EffectId::PurpleExplosion, obj.getPosition(), {150, 150}, 0.69f);
            onSlowDownMult(1.1);
            loseTail(tail.size() - 1);
            bounceOff(c_data.separation_axis);
            //! make the text slowly disappear;
            TimedEventComponent t_comp;
            auto kill_obj = [&obj](float t, int c)
            { obj.kill(); };
            auto disappear_obj = [&obj](float t, int c)
            {
                auto &text = static_cast<TextBubble &>(obj);
                text.m_opacity = (3.f - t) / 3.f;
            };
            t_comp.addEvent({kill_obj, 0.f, 5.f});
            t_comp.addEvent({disappear_obj, TimedEventType::Infinite, 0.f, 2.f});
            m_world->m_systems.add(t_comp, obj.getId());
            m_world->m_systems.removeDelayed<CollisionComponent>(obj.getId());
            m_world->m_collision_system.removeObject(obj);

            Transform final_trans = text.light_rect;
            final_trans.setScale(50, 50);
            transformAnimation(3.f, 2.f, text.light_rect, final_trans, text.light_rect, text.m_timers);
            text.light_rect.m_color = {1, 0.5, 0, 1};
        }
    }
    if (obj.getType() == ObjectType::Wall)
    {
        //! moving into wall
        auto head_dir = utils::angle2dir(m_angle);
        if (utils::dot(head_dir, -c_data.separation_axis) < 0.f)
        {
            loseTail(tail.size() - 2);
            bounceOff(-c_data.separation_axis);
            onSlowDownMult(1.1);
        }
    }
}

Snake::PastPoint extrapolate(const Snake::PastPoint &val, float speed, double time_factor, double t)
{
    Snake::PastPoint res = val;
    if (val.dir != 0)
    {
        float radius = val.radius; // speed / (utils::radians(angle_vel));
        float angle_vel = utils::degrees(speed / radius);
        res.angle = val.angle + (t - val.time / time_factor) * angle_vel * time_factor * val.dir;
        utils::Vector2f norm = utils::angle2dir(val.angle);
        norm = {norm.y, -norm.x};
        utils::Vector2f center = val.pos - val.dir * norm * radius;
        res.pos = center + val.dir * radius * utils::angle2dir(res.angle - 90);
    }
    else
    {
        res.pos = val.pos + (t - val.time / time_factor) * speed * time_factor * utils::angle2dir(val.angle);
    }
    return res;
}