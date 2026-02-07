#include "TestGameServer.h"

#include "Utils/RandomTools.h"

#include "Systems/SpriteSystem.h"
#include "Systems/TimedEventSystem.h"

#include "Assets.h"
#include "Texts.h"

void sendState(PostOffice &messenger, std::unordered_map<std::size_t, int> player_ids, std::vector<GameThing> &things)
{
    nlohmann::json msg = {};
    msg["type"] = "state";
    msg["objects"] = {};
    for (const auto [pos, vel] : things)
    {
        nlohmann::json obj_msg;
        obj_msg = {};
        obj_msg["pos"] = {pos.x, pos.y};
        obj_msg["vel"] = {vel.x, vel.y};
        msg["objects"].push_back(obj_msg);
    }
    msg["players"] = {};
    for (const auto [player_id, obj_id] : player_ids)
    {
        msg["players"].push_back({player_id, obj_id});
    }
    messenger.send(msg);
}

TestGameServer::TestGameServer(PostOffice &messenger)
    : m_messenger(messenger),
      m_world(messenger)
{
    m_timers.addInfiniteEvent([this](float t, int c) {}, 3.5f, 0.f);
    registerSystems();
    m_things.reserve(50);
}

void TestGameServer::registerSystems()
{
    // auto &colllider = m_world.getCollisionSystem();
    // colllider.registerResolver(ObjectType::Bullet, ObjectType::TextBubble);
    // colllider.registerResolver(ObjectType::Snake, ObjectType::TextBubble);
    // colllider.registerResolver(ObjectType::Snake, ObjectType::Wall);

    // auto &systems = m_world.m_systems;
    // systems.registerSystem(std::make_shared<TransformSystem>(systems.getComponents<TransformComponent>()));
    // systems.registerSystem(std::make_shared<TimedEventSystem>(systems.getComponents<TimedEventComponent>()));
}

void TestGameServer::update(std::uint64_t time_stamp)
{
    double dt = (time_stamp - m_old_time_stamp) * 1e-6;
    m_old_time_stamp = time_stamp;
    // m_world.update(dt);
    m_timers.update(dt);
    for (auto &[pos, vel] : m_things)
    {
        pos += vel * dt;
    }

    sendState(m_messenger, m_player2thing_id, m_things);
}
void TestGameServer::draw(LayersHolder &layers, Assets &assets, Renderer &window_canvas, View view)
{
    for (auto &[pos, vel] : m_things)
    {
        window_canvas.drawCricleBatched(pos, 36, {1, 0, 0, 1});
    }
}

void TestGameServer::broadCastEvent(ClientEvent event)
{
}
void TestGameServer::handleSocketEvent(std::size_t client_id, nlohmann::json event)
{
    if (event["type"] == "client-started")
    {
        sendState(m_messenger, m_player2thing_id, m_things);
    }
    if (event.at("type") == "client-ready")
    {
        utils::Vector2f pos = {utils::randf(0, 800), utils::randf(0, 800)};
        m_things.emplace_back(pos, Vec2{0, 0});
        m_player2thing_id[player_count] = m_things.size();
        player_count++;

        nlohmann::json msg;
        msg["type"] = "game-started";
        m_messenger.send(msg);
    }
    if (event.at("type") == "client-connected")
    {
        return;
    }

    if (event.at("type") == "control")
    {
        int player_id = client_id;
        if (!m_player2thing_id.count(player_id))
        {
            return;
        }

        auto &obj = m_things.at(m_player2thing_id.at(player_id));
        if (event.at("action") == "pushed")
        {
            if (event.at("key") == "w")
            {
                obj.vel.y = m_player_speed;
            }
            if (event.at("key") == "s")
            {
                obj.vel.y = -m_player_speed;
            }
            if (event.at("key") == "d")
            {
                obj.vel.x = m_player_speed;
            }
            if (event.at("key") == "a")
            {
                obj.vel.x = -m_player_speed;
            }
        }
        if (event.at("action") == "released")
        {
            if (event.at("key") == "w")
            {
                obj.vel.y = 0;
            }
            if (event.at("key") == "s")
            {
                obj.vel.y = 0;
            }
            if (event.at("key") == "d")
            {
                obj.vel.x = 0;
            }
            if (event.at("key") == "a")
            {
                obj.vel.x = 0;
            }
        }
    }
}
