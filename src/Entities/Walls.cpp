#include "Walls.h"

#include "DrawLayer.h"
#include "Assets.h"

#include "GameWorld.h"
#include "Player.h"
#include "SoundSystem.h"


Wall::Wall(GameWorld &world, const Spec &spec, int ent_id, ObjectType type)
    : GameObject(&world, ent_id, type)
{
    m_sprite_spec = spec.s_sprite;
}

void Wall::onCreation()
{
    CollisionComponent c_comp;
    c_comp.shape.convex_shapes.emplace_back(4);
    c_comp.type = ObjectType::Wall;
    m_world->m_systems.addEntity(getId(), c_comp);
}

void Wall::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
}

void Wall::draw(LayersHolder &layers, Assets& assets)
{
    auto &canvas = layers.getCanvas("Unit");

    auto &texture = *assets.textures.get(m_sprite_spec.texture_id);
    auto tex_rect = m_sprite_spec.tex_rect;
    auto tex_size = texture.getSize();
    Sprite wall_sprite(texture);
    wall_sprite.m_tex_rect = {0, 0, (int)(tex_size.x * m_size.x / 50.f), (int)(tex_size.y * m_size.y / 50.f)};
    wall_sprite.setPosition(m_pos);
    wall_sprite.setScale(m_size / 2.f);
    wall_sprite.setRotation(utils::radians(m_angle));
    canvas.drawSprite(wall_sprite);

    auto &b_canvas = layers.getCanvas("Bloom");
    float line_w = 4.0f;
    utils::Vector2f l_size = {m_size.x, m_size.y};
    /*     RectangleSimple line_rect;
        line_rect.setColor(m_line_color);
        line_rect.setScale(line_w * 2, m_size.y + line_w);
        line_rect.setPosition(Vec2{m_pos.x - m_size.x / 2.f - line_w, m_pos.y});
        b_canvas.drawRectangle(line_rect, m_line_color, "gradientX");
        line_rect.setRotation(180.f);
        line_rect.setPosition(Vec2{m_pos.x + m_size.x / 2.f + line_w, m_pos.y});
        b_canvas.drawRectangle(line_rect, m_line_color, "gradientX");
        line_rect.setRotation(0.f);
        line_rect.setScale(m_size.x + line_w, line_w * 2);
        line_rect.setPosition(Vec2{m_pos.x, m_pos.y - m_size.y / 2.f - line_w});
        b_canvas.drawRectangle(line_rect, m_line_color, "gradientY");
        line_rect.setRotation(180.f);
        line_rect.setPosition(Vec2{m_pos.x, m_pos.y + m_size.y / 2.f + line_w});
        b_canvas.drawRectangle(line_rect, m_line_color, "gradientY") */
    ;
    // b_canvas.drawLineBatched({m_pos.x + m_size.x / 2.f, m_pos.y + l_size.y / 2.f + line_w / 2.f}, {m_pos.x + l_size.x / 2.f, m_pos.y - l_size.y / 2.f - line_w / 2.f}, line_w, m_line_color);
    // b_canvas.drawLineBatched({m_pos.x + l_size.x / 2.f + line_w / 2.f, m_pos.y - l_size.y / 2.f}, {m_pos.x - l_size.x / 2.f - line_w / 2.f, m_pos.y - l_size.y / 2.f}, line_w, m_line_color);
    // b_canvas.drawLineBatched({m_pos.x - l_size.x / 2.f, m_pos.y - l_size.y / 2.f - line_w / 2.f}, {m_pos.x - l_size.x / 2.f, m_pos.y + l_size.y / 2.f + line_w / 2.f}, line_w, m_line_color);
    // b_canvas.drawLineBatched({m_pos.x - l_size.x / 2.f - line_w / 2.f, m_pos.y + l_size.y / 2.f}, {m_pos.x + l_size.x / 2.f + line_w / 2.f, m_pos.y + l_size.y / 2.f}, line_w, m_line_color);
}

void PathingWall::onCreation()
{
    CollisionComponent c_comp;
    c_comp.shape.convex_shapes.emplace_back(4);
    c_comp.type = ObjectType::PathingWall;
    m_world->m_systems.addEntity(getId(), c_comp);

    if (m_path.steps.size() > 0)
    {
        setPath(m_path, m_speed);
    }
}

/* void PathingWall::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
}
 */
PathingWall::PathingWall(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, ObjectType::PathingWall)
{
    m_sprite_spec = spec.s_sprite;

    m_path = spec.path;
}

void PathingWall::update(float dt)
{
    m_pos += m_vel * dt;

    m_reached_next_point.update(dt);
}

void PathingWall::setPath(const Path &path, float speed)
{
    m_speed = speed;
    if (path.steps.size() == 0)
    {
        return;
    }
    m_path = path;

    //! set vel to move to first path point
    m_current_path_id = 0;
    auto offset = m_path.steps.at(m_current_path_id).target;
    float distance = utils::norm(offset);
    if (distance > 0.f)
    {
        m_vel = m_speed * offset / distance;
    }
    auto on_reaching_first_point = [this]()
    {
        if (m_path.stepsCount() > 0.f)
        {
            moveToNextPoint();
        }
        else
        {
            m_vel *= 0.f;
        }
    };
    m_reached_next_point.runDelayed(on_reaching_first_point, distance / m_speed);
}

void PathingWall::moveToNextPoint()
{
    auto prev_point = m_path.steps.at(m_current_path_id).target;
    auto next_point = m_path.steps.at(m_path.getNextStepId(m_current_path_id)).target;
    auto offset = next_point - prev_point;

    auto target_pos = getPosition() + offset;
    float point_distance = utils::dist(getPosition(), target_pos);
    float time_to_reach_point = point_distance / m_speed;
    if (time_to_reach_point > 0.f)
    {
        m_vel = (offset) / time_to_reach_point;
    }
    m_current_path_id = m_path.getNextStepId(m_current_path_id);

    //! when reaching the next point move to another point
    auto on_reaching_point = [this]()
    {
        // m_current_path_id = m_path.getNextStepId(m_current_path_id);
        if (m_path.stepsCount() > 1)
        {
            moveToNextPoint();
        }
        else
        {
            m_vel *= 0.f;
        }
    };
    m_reached_next_point.runDelayed(on_reaching_point, time_to_reach_point);
}

void PathingWall::draw(LayersHolder &layers, Assets& assets)
{
    auto &canvas = layers.getCanvas("Unit");

    auto &texture = *assets.textures.get(m_sprite_spec.texture_id);
    auto tex_rect = m_sprite_spec.tex_rect;
    auto tex_size = texture.getSize();
    Sprite wall_sprite(texture);
    if (m_sprite_spec.scalable)
    {
        float aspect = m_size.y / m_size.x;
        wall_sprite.m_tex_rect = {0, 0, (int)(tex_rect.width * m_size.x), (int)(tex_rect.height * m_size.y)};
    }
    else
    {
        wall_sprite.m_tex_rect = m_sprite_spec.tex_rect;
    }
    wall_sprite.setPosition(m_pos);
    wall_sprite.setScale(m_size / 2.f);
    wall_sprite.setRotation(utils::radians(m_angle));
    canvas.drawSprite(wall_sprite);

    auto &b_canvas = layers.getCanvas("Bloom");
    float line_w = 4.0f;
    utils::Vector2f l_size = {m_size.x, m_size.y};
}


ElectroWall::ElectroWall(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, ent_id, ObjectType::ElectroWall)
{
    m_dmg = spec.dmg;
    m_on_duration = spec.on_duration;
    m_off_duration = spec.off_duration;
    m_color = spec.color;
}
void ElectroWall::onCreation()
{
    CollisionComponent c_comp;
    c_comp.shape.convex_shapes.emplace_back(4);
    c_comp.type = ObjectType::ElectroWall;

    TimedEventComponent t_comp;
    if (m_off_duration > 0.f)
    {
        float on_off_cycle_period = m_on_duration + m_off_duration;
        t_comp.addEvent({[this](float t, int c)
                         {
                             m_is_on = true;
                         },
                         TimedEventType::Infinite, on_off_cycle_period, m_off_duration});
        t_comp.addEvent({[this](float t, int c)
                         {
                             m_is_on = false;
                         },
                         TimedEventType::Infinite, on_off_cycle_period, 0.f});
    }

    m_world->m_systems.addEntity(getId(), c_comp, t_comp);
}

void ElectroWall::draw(LayersHolder &layers, Assets& assets)
{
    auto &target = layers.getCanvas("Unit");

    Sprite holder(*assets.textures.get("Station"));
    auto dir = utils::angle2dir(m_angle);
    holder.setPosition(m_pos + dir * (m_size.x / 2.f + m_size.y / 2.f));
    holder.setScale(m_size.y, m_size.y);
    holder.setRotation(utils::radians(m_angle));
    target.drawSprite(holder);

    holder.setPosition(m_pos - dir * (m_size.x / 2.f + m_size.y / 2.f));
    target.drawSprite(holder);

    if (m_is_on)
    {
        holder.setPosition(m_pos);
        holder.setScale(m_size / 2.f);
        holder.setColor(m_color);
        target.drawSprite(holder, "ElectroWall");
    }
}

void ElectroWall::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (obj.getType() == ObjectType::Player && m_is_on)
    {
        auto dr = obj.getPosition() - m_pos;
        auto dr_dir = dr / utils::norm(dr);
        auto dr_norm = utils::dot(dr_dir, c_data.separation_axis) * c_data.separation_axis;
        obj.m_vel -= utils::dot(obj.m_vel, c_data.separation_axis) * c_data.separation_axis;
        static_cast<Player &>(obj).m_impulse_vel += dr_norm * 400.f;
        SoundSystem::play("ElectroShock1");
    }
}
