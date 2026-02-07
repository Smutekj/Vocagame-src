#include "Camera.h"

#include "GameObject.h"
#include "PostOffice.h"

Camera::Camera(utils::Vector2f center, utils::Vector2f size, PostOffice &messanger)
    : m_messanger(messanger)
{
    m_view.setCenter(center);
    m_view.setSize(size);
    m_view_target_size = m_view.getSize();
    m_default_view = m_view;
}

void Camera::setSpeed(float speed)
{
    m_max_view_speed = speed;
}

void Camera::startFollowingPath(std::deque<utils::Vector2f> path, float duration, std::function<void(Camera &)> callback)
{
    m_path = path;
    m_on_reaching_target_callback = callback;

    m_move_state = MoveState::FollowingPath;
    m_move_view_duration = duration;
    m_move_view_time = 0.f;
    m_view_target = path.at(0);
    path.pop_front();
}
void Camera::startMovingTo(utils::Vector2f target, float duration, std::function<void(Camera &)> callback)
{
    m_on_reaching_target_callback = callback;

    m_move_state = MoveState::MovingToPosition;
    m_move_view_duration = duration;
    m_move_view_time = 0.f;
    m_view_target = target;
}
void Camera::startChangingSize(utils::Vector2f target, float duration, std::function<void(Camera &)> callback)
{
    m_on_reaching_target_callback = callback;

    m_view_size_state = SizeState::Resizing;
    m_resize_view_duration = duration;
    m_resize_view_time = 0.f;
    m_view_target_size = target;
}

void Camera::resizeToTarget(float dt)
{
    m_move_view_time += dt;

    utils::Vector2f dr_to_target = m_view_target_size - m_view.getSize();
    float dist_to_target = std::max(utils::norm(dr_to_target), 0.001f);
    float view_speed = 100.;

    if (dist_to_target < 10)
    {
        m_view_size_state = SizeState::FollowingPlayer; //! by default start Following Player if neede can be overriden in callback
        m_on_reaching_target_callback(*this);
        view_speed = 0.;
        m_view_velocity = {0};
    }

    m_view_velocity = dr_to_target / dist_to_target * view_speed;
    m_view.setSize(m_view.getSize() + m_view_velocity * dt);
}

bool Camera::moveToTarget(float dt)
{
    m_move_view_time += dt;

    utils::Vector2f dr_to_target = m_view_target - m_view.getCenter();
    float dist_to_target = utils::norm(dr_to_target);
    float speed = m_max_view_speed;
    if (dist_to_target < 20)
    {
        m_view_velocity = {0};
        return true;
    }

    m_view_velocity = dr_to_target / dist_to_target * (m_max_view_speed + m_player_near_edge * m_max_view_speed);
    // utils::truncate(m_view_velocity, m_max_view_speed);
    m_view.setCenter(m_view.getCenter() + m_view_velocity * dt);
    return false;
}

void Camera::update(float dt)
{
    if (m_followee && m_move_state == MoveState::FollowingPlayer && m_view_size_state == SizeState::FollowingPlayer)
    {

        followPlayer(dt);
        resizeToTarget(dt);
    }
    else if (m_move_state == MoveState::MovingToPosition)
    {
        if (moveToTarget(dt))
        {
            m_move_state = MoveState::FollowingPlayer; //! by default start Following Player if neede can be overriden in callback
            m_on_reaching_target_callback(*this);
        }
    }
    else if (m_followee && m_move_state == MoveState::FollowingPath)
    {
        auto threshold = m_view.getSize() / 2.f - m_view.getSize() / 3.f;
        auto dx = m_followee->getPosition().x - m_view.getCenter().x;
        auto dy = m_followee->getPosition().y - m_view.getCenter().y;
        auto center = m_view.getCenter();
        m_player_near_edge = false;
        if (dx > threshold.x)
        {
            m_view.setCenter(center.x + dx - threshold.x, center.y);
        }
        else if (dx < -threshold.x)
        {
            m_view.setCenter(center.x + dx + threshold.x, center.y);
        }
        if (dy > threshold.y)
        {
            m_view.setCenter(center.x, center.y + dy - threshold.y);
        }
        else if (dy < -threshold.y)
        {
            m_view.setCenter(center.x, center.y + dy + threshold.y);
        }

        followPath(dt);
    }
    if (m_view_size_state == SizeState::Resizing)
    {
        resizeToTarget(dt);
    }

    if (m_followee && !m_view.contains(m_followee->getPosition()))
    {
        m_messanger.send(EntityLeftViewEvent{m_followee->getId(), m_view});
    }
}
View Camera::getView() const
{
    return m_view;
}

void Camera::followPath(float dt)
{
    bool reached_next_spot = moveToTarget(dt);
    if (reached_next_spot) //! reached target spot and there is more
    {
        if (m_path.empty())
        {
            m_move_state = MoveState::FollowingPlayer;
            m_on_reaching_target_callback(*this);
            return;
        }
        m_view_target = m_path.front();
        m_path.pop_front();
    }
}

void Camera::followPlayer(float dt)
{
    const auto &view_size = m_view.getSize();
    const auto &view_pos = m_view.getCenter();
    //! look from higher distance when boosting
    auto view_ll = view_pos - view_size / 2.f;
    auto view_ur = view_pos + view_size / 2.f;
    float threshold_left = view_ll.x + view_size.x * (threshold_rect.pos_x);
    float threshold_right = view_ll.x + view_size.x * (threshold_rect.pos_x + threshold_rect.width);
    float threshold_down = view_ll.y + view_size.y * (threshold_rect.pos_y);
    float threshold_up = view_ll.y + view_size.y * (threshold_rect.pos_y + threshold_rect.height);

    auto p_pos = m_followee->getPosition();

    //! for checking if approaching world boundaries
    float world_left = m_world_bounds.pos_x;
    float world_right = m_world_bounds.pos_x + m_world_bounds.width;
    float world_down = m_world_bounds.pos_y;
    float world_up = m_world_bounds.pos_y + m_world_bounds.height;

    m_view_velocity = {0};
    utils::Vector2f m_view_acc = {0};
    //! move view when approaching sides
    //! if bounded by world, we also check world boundaries
    if (m_can_move_x)
    {
        if (!m_bounded || view_ur.x < world_right)
        {
            m_view_acc.x += std::max(p_pos.x - threshold_right, 0.f);
        }
        if (!m_bounded || view_ll.x > world_left)
        {
            m_view_acc.x += std::min(p_pos.x - threshold_left, 0.f);
        }
    }

    if (m_can_move_y)
    {
        if(!m_bounded || view_ur.y < world_up)
        {
            m_view_acc.y += std::max(p_pos.y - threshold_up, 0.f);
        }
        if(!m_bounded || view_ll.y > world_down)
        {
            m_view_acc.y += std::min(p_pos.y - threshold_down, 0.f);
        }
    }
    // m_view.setCenter(m_player->getPosition());
    m_view.setCenter(m_view.getCenter() + m_view_acc);
}

void Camera::setSize(const utils::Vector2f &size)
{
    m_view.setSize(size);
    m_view_target_size = size;
}

void Camera::zoom(float factor)
{
    m_view.zoom(factor);
    m_view_target_size = m_view.getSize();
}
void Camera::setFolowee(GameObject *followee)
{
    m_followee = followee;
}