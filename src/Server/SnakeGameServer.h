#pragma once

#include <unordered_map>

#include "PostOffice.h"
#include "PostBox.h"
#include "GameWorld.h"
#include "TimedEventManager.h"
#include "Grid.h"
#include "Snake.h"

#include "Utils/GridPosGenerator.h"

struct Assets;
class TextBubble;
class TextRect;

class SnakeGameServer
{

public:
    SnakeGameServer(PostOffice &messenger);

    void update(std::uint64_t time_stamp);
    void handleSocketEvent(std::size_t client_id, SnakeClientEvent event);
    void draw(LayersHolder &layers, Assets &assets, Renderer &window_canvas, View view);
    void broadCastEvent(SnakeClientEvent event);

private:
    TextRect& generateWord2(bool is_correct, utils::Vector2f pos, utils::Vector2f size);
void registerSystems();

private:
    std::uint64_t m_old_time_stamp = 0;

    PostOffice &m_messenger;

    TimedEventManager m_timers;
    GameWorld m_world;

    std::unordered_map<std::size_t, std::shared_ptr<Snake>> m_snakes;
    std::shared_ptr<Snake> m_snake;

    utils::Grid m_text_grid;


    GridPosGenerator m_pos_generator;

};