#pragma once

#include "Game.h"

struct Assets;

class DodgeGame : public Game
{

public:
    DodgeGame(Renderer &window, KeyBindings &bindings, Assets &asset);
    virtual ~DodgeGame() override {}

    // virtual void updateImpl(const float dt, Renderer &win);
    // virtual void handleEventImpl(const SDL_Event &event);
    virtual void drawImpl(Renderer &window) override;

    std::shared_ptr<GameLevelA> levelFactory(int level_id, utils::Vector2f start_pos);

    void registerCollisions();
    void registerSystems();
    void restartGame();
    void initializeUI();
    
    std::unique_ptr<PostBox<NewEntity<TextBubble>>> m_on_text_create;
    std::unique_ptr<PostBox<WordGuessedEvent>> m_guess_listener;
    std::deque<std::shared_ptr<GameLevelA>> m_levels;
    
  
    std::size_t m_correct_count = 0;
    std::size_t m_guessed_correct_count = 0;
    float m_lives_scale = 1.f;
    float m_correct_scale = 1.f;
    float m_ui_header_height = 40.f;
};
