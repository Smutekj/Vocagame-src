#include "Meteor.h"

#include "GameWorld.h"

#include "DrawLayer.h"
#include "Assets.h"
#include "Utils/RandomTools.h"

Meteor::Meteor(GameWorld &world, const Meteor::Spec &spec, int ent_id)
    : GameObject(&world, spec, ent_id, TypeId::Meteor)
{
    // m_rigid_body = std::make_unique<RigidBody>();
    m_max_vel = 200.f;
    initializeRandomMeteor(spec.size.x);
}

void Meteor::update(float dt)
{
    using namespace utils;

    truncate(m_vel, m_max_vel);
    truncate(m_impulse_vel, m_max_impulse_vel);
    m_pos += (m_vel + m_impulse_vel) * dt;
    setAngle(m_angle + m_angle_vel * dt);

    m_impulse_vel -= m_impulse_vel * m_impulse_decay * dt;

    // auto player_pos = p_player->getPosition();
    // auto player_vel = p_player->m_vel;
    // auto obj_moves_away = utils::dot(player_vel, m_vel) < 0;
    // auto player_obj_dist = utils::dist(m_pos, player_pos);
    // if (obj_moves_away && player_obj_dist > max_dist_from_player)
    // {
    //     auto rand_radius = randf(max_dist_from_player * 0.6f, max_dist_from_player * 0.9f);
    //     auto rand_angle = randf(0, 360);
    //     auto new_obj_pos = player_pos + rand_radius * utils::angle2dir(rand_angle);
    //     setPosition(new_obj_pos);
    // }
}
void Meteor::onCreation()
{
}
void Meteor::onDestruction()
{
}

void Meteor::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    GameObject::onCollisionWith(obj, c_data);
}

void Meteor::draw(LayersHolder &layers, Assets &assets)
{

    //! find center of mass in the base coordinates
    auto &cp = m_world->m_systems.get<CollisionComponent>(getId());
    auto collision_shape = cp.shape.convex_shapes[0];
    auto points_orig = collision_shape.points;

    auto n_points = points_orig.size();

    auto points = collision_shape.getPointsInWorld();
    auto &target = layers.getCanvas("Unit");

    m_verts.resize(3 * points.size());

    Color c = {1, 0, 0, 1};
    auto center = collision_shape.getCenter();
    for (int i = 0; i < n_points; ++i)
    {
        m_verts[3 * i + 0] = {points[i], c, points_orig[i] * 0.1 + m_center_offset};
        m_verts[3 * i + 1] = {points[(i + 1) % n_points], c, points_orig[(i + 1) % n_points] * 0.1 + m_center_offset};
        m_verts[3 * i + 2] = {center, c, m_center_tex * 0.1 + m_center_offset};
    }

    target.drawVertices(m_verts, "Meteor", assets.textures.get("Meteor"));
}

//! Not my code! Taken from here: https://cglab.ca/~sander/misc/ConvexGeneration/convex.html
Polygon generateRandomConvexPolygon(int n) 
{

    // Generate two lists of random X and Y coordinates
    std::vector<float> xPool(0);
    std::vector<float> yPool(0);

    for (int i = 0; i < n; i++)
    {
        xPool.push_back(utils::randf(-1, 1));
        yPool.push_back(utils::randf(-1, 1));
    }

    // Sort them
    std::sort(xPool.begin(), xPool.end());
    std::sort(yPool.begin(), yPool.end());

    // Isolate the extreme points
    auto minX = xPool.at(0);
    auto maxX = xPool.at(n - 1);
    auto minY = yPool.at(0);
    auto maxY = yPool.at(n - 1);

    // Divide the interior points into two chains & Extract the vector components
    std::vector<float> xVec(0);
    std::vector<float> yVec(0);

    float lastTop = minX, lastBot = minX;

    for (int i = 1; i < n - 1; i++)
    {
        auto x = xPool.at(i);

        if (rand() % 2)
        {
            xVec.push_back(x - lastTop);
            lastTop = x;
        }
        else
        {
            xVec.push_back(lastBot - x);
            lastBot = x;
        }
    }

    xVec.push_back(maxX - lastTop);
    xVec.push_back(lastBot - maxX);

    float lastLeft = minY, lastRight = minY;

    for (int i = 1; i < n - 1; i++)
    {
        auto y = yPool.at(i);

        if (rand() % 2)
        {
            yVec.push_back(y - lastLeft);
            lastLeft = y;
        }
        else
        {
            yVec.push_back(lastRight - y);
            lastRight = y;
        }
    }

    yVec.push_back(maxY - lastLeft);
    yVec.push_back(lastRight - maxY);

    std::random_device rd;
    std::mt19937 g(rd());

    // Randomly pair up the X- and Y-components
    std::shuffle(yVec.begin(), yVec.end(), g);

    // Combine the paired up components into vectors
    std::vector<utils::Vector2f> vec;

    for (int i = 0; i < n; i++)
    {
        vec.emplace_back(xVec.at(i), yVec.at(i));
    }

    // Sort the vectors by angle
    std::sort(vec.begin(), vec.end(), [](const auto &p1, const auto &p2)
              { return std::atan2(p1.y, p1.x) < std::atan2(p2.y, p2.x); });

    // Lay them end-to-end
    float x = 0, y = 0;
    float minPolygonX = 0;
    float minPolygonY = 0;
    std::vector<utils::Vector2f> points;

    for (int i = 0; i < n; i++)
    {
        points.push_back({x, y});

        x += vec.at(i).x;
        y += vec.at(i).y;

        minPolygonX = std::min(minPolygonX, x);
        minPolygonY = std::min(minPolygonY, y);
    }

    // Move the polygon to the original min and max coordinates
    auto xShift = minX - minPolygonX;
    auto yShift = minY - minPolygonY;

    for (int i = 0; i < n; i++)
    {
        auto p = points.at(i);
        points.at(i) += utils::Vector2f{xShift, yShift};
    }
    Polygon p;
    p.points = points;

    return p;
}

void Meteor::initializeRandomMeteor(float radius)
{
    auto polygon = generateRandomConvexPolygon(12 + rand() % 3);
    m_size = {radius * 2};
    polygon.setScale(radius, radius);
    auto rand_pos = randomPosInBox(utils::Vector2f{0, 0}, utils::Vector2f{500, 500});
    polygon.setPosition(rand_pos.x, rand_pos.y);

    m_pos = polygon.getPosition();
    m_angle = polygon.getRotation();

    m_vel = {utils::randf(-6, 6), utils::randf(-6, 6)};

    m_angle_vel = utils::randf(-10., 10.);
    m_ass = radius * radius;
    m_inertia = 0.1 * radius * radius * m_ass;

    CollisionComponent c_comp;
    c_comp.type = TypeId::Meteor;

    //! texture coordinate
    auto &points_orig = polygon.points;
    m_center_tex = std::accumulate(points_orig.begin(), points_orig.end(), utils::Vector2f{0, 0},
                                   [](utils::Vector2f prev_sum, const utils::Vector2f &val)
                                   {
                                       return prev_sum + val;
                                   });
    m_center_tex /= points_orig.size();

    m_center_offset = {utils::randf(-0.85, 0.85), utils::randf(-0.85, 0.85)};
    
    c_comp = {std::vector<Polygon>{polygon}, ObjectType::Meteor};
    m_world->m_systems.addDelayed(c_comp, getId());
}