#pragma once

#include "Game.h"


struct Assets;

class CastleGame : public Game
{

public:
    CastleGame(Renderer &window, KeyBindings &bindings, Assets& asset);
    virtual ~CastleGame() override{}
    // virtual void updateImpl(const float dt, Renderer &win);
    // virtual void handleEventImpl(const SDL_Event &event);
    virtual void drawImpl(Renderer &window) final;

    std::shared_ptr<GameLevelA> levelFactory(int level_id, utils::Vector2f start_pos);

    void registerCollisions();
    void registerSystems();
    void restartGame();
    void initializeUI();

    std::unique_ptr<PostBox<LevelCompletedEvent>> m_level_listener;
    std::unique_ptr<PostBox<WordGuessedEvent>> m_guess_listener;
    std::size_t m_guessed_correct_count = 0;
    std::size_t m_guessed_wrong_count = 0;
    std::size_t m_correct_count = 0;
    std::size_t m_total_count = 0;
    
    float m_mistakes_scale = 1.f;
    float m_correct_scale = 1.f;
    float m_ui_header_height = 60.f;
    Text m_level_completed_text;
    
    std::deque<std::shared_ptr<GameLevelA>> m_levels;
    std::vector<CaveSpec2> m_cave_specs;
    
    utils::Vector2f m_checkpoint_pos;
};
 

