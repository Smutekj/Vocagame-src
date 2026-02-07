#pragma once

#include <unordered_map>

#include "Game.h"
#include "Snake.h"

#include "Utils/GridPosGenerator.h"

class SnakeGame : public Game
{

public:
    SnakeGame(Renderer &window, KeyBindings &binds, Assets &assets);

    virtual void updateImpl(float dt) override;
    virtual void handleEventImpl(const SDL_Event &event) override;
    virtual void drawImpl(Renderer &window) override;

private:
    TextRect& generateWord2(bool is_correct, utils::Vector2f pos, utils::Vector2f size);
    void registerSystems();

private:
    std::shared_ptr<Snake> m_snake;
    
    std::unordered_map<int, TextBubble *> m_texts;

    std::size_t m_client_id = -1;
    utils::Grid m_text_grid;

    std::uint64_t m_time_stamp = 0;
    GridPosGenerator m_pos_generator;
   
    struct SnakeState
    {
        int ent_id;
        utils::Vector2f pos;
        float angle;
        int dir; 
    };

    struct WorldState{
        std::uint64_t server_time;
        std::vector<SnakeState> snakes;
    };

    std::uint64_t m_history_delay = 200000; //200 ms;
    std::uint64_t m_interpolation_delay =30000 ; // 30 ms
    std::deque<WorldState> m_history;
};