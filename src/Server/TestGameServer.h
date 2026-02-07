#pragma once

#include <unordered_map>

#include "PostOffice.h"
#include "PostBox.h"
#include "TimedEventManager.h"
#include "Grid.h"

#include "Renderer.h"
#include "Utils/GridPosGenerator.h"

#include "nlohmann/json.hpp"

#include "GameWorld.h"

struct Assets;
class TextBubble;
class TextRect;

struct ClientEvent{
    std::string type;
};

struct GameThing{
    utils::Vector2f pos;
    utils::Vector2f vel;
};

class TestGameServer
{

public:
    TestGameServer(PostOffice &messenger);

    void update(std::uint64_t time_stamp);
    void handleSocketEvent(std::size_t client_id, nlohmann::json msg);
    void draw(LayersHolder &layers, Assets &assets, Renderer &window_canvas, View view);
    void broadCastEvent(ClientEvent event);

private:
    TextRect& generateWord2(bool is_correct, utils::Vector2f pos, utils::Vector2f size);
void registerSystems();

private:
    std::uint64_t m_old_time_stamp = 0;

    PostOffice &m_messenger;

    TimedEventManager m_timers;

    GameWorld m_world;

    std::vector<GameThing> m_things;
    std::unordered_map<std::size_t, int> m_player2thing_id;

    float m_player_speed = 50;

    int player_count = 0;

};