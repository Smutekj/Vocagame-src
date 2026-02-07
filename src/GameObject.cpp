#include "GameObject.h"

#include "Polygon.h"

GameObject::GameObject(GameWorld *world, const GameObjectSpec &spec, int id, ObjectType type)
    : m_pos(spec.pos),
      m_vel(spec.vel),
      m_size(spec.size),
      m_angle(spec.angle),
      m_world(world), m_id(id), m_type(type)
{
}

GameObject::GameObject(GameWorld *world, int id, ObjectType type)
    : m_world(world), m_id(id), m_type(type)
{
}

void GameObject::update(float dt)
{
    m_pos += m_vel * dt;
};

void GameObject::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (m_collision_resolvers.contains(obj.getType()))
    {
        m_collision_resolvers.at(obj.getType())(obj, c_data);
    }
};

void GameObject::onCreation() {}
void GameObject::onDestruction() { m_on_destruction_callback(getId(), m_type); }
void GameObject::draw(LayersHolder &target, Assets &assets) {}

void GameObject::updateAll(float dt)
{

    if (m_parent)
    {
        // m_pos = m_parent->getPosition();
        // m_angle = m_parent->getAngle();
        m_vel = m_parent->m_vel;
    }

    update(dt);
}

bool GameObject::isRoot() const
{
    return m_parent == nullptr;
}

const utils::Vector2f GameObject::getPosition() const
{
    utils::Vector2f origin = {m_pivot.x * m_size.x, m_pivot.y * m_size.y};
    if (m_parent)
    {
        auto r = utils::rotate(-origin, m_angle) + m_pos;
        auto pr = m_parent->getPosition();
        return pr + utils::rotate(r, m_parent->getAngle());
    }
    return m_pos - utils::rotate(origin, m_angle);
}

float GameObject::getAngle() const
{
    if (m_parent)
    {
        return m_angle + m_parent->getAngle();
    }
    return m_angle;
}

int GameObject::getId() const
{
    return m_id;
}

ObjectType GameObject::getType() const
{
    return m_type;
}

void GameObject::setDestructionCallback(std::function<void(int, ObjectType)> callback)
{
    m_on_destruction_callback = callback;
}

void GameObject::addChild(GameObject *child)
{
    m_children.push_back(child);
    child->m_parent = this;
}

void GameObject::removeChild(GameObject *child)
{
    m_children.erase(std::remove(m_children.begin(), m_children.end(), child), m_children.end());
}

bool GameObject::isParentOf(GameObject *child) const
{
    //! walk through queried object parents, if we find ourselves we are a parent of the child
    GameObject *curr = child->m_parent;
    while (curr)
    {
        if (curr == this)
        {
            return true;
        }
        curr = curr->m_parent;
    }
    return false;
}

void GameObject::kill()
{
    m_is_dead = true;
}

bool GameObject::isDead() const
{
    return m_is_dead;
}

void GameObject::setPosition(utils::Vector2f new_position)
{
    m_pos = new_position;
}

void GameObject::setSize(utils::Vector2f size)
{
    m_size = size;
}

const utils::Vector2f &GameObject::getSize() const
{
    return m_size;
}

void GameObject::setAngle(float angle)
{
    m_angle = angle;
}

void GameObject::move(utils::Vector2f by)
{
    setPosition(getPosition() + by);
}