
#pragma once

#include <unordered_map>

#include "Game.h"
#include "Grid.h"
#include "PlayerShip.h"

#include "Utils/GridPosGenerator.h"

// struct CollisionInfo 
// {
//     GameObject* o1;
//     GameObject* o2;
//     CollisionData c_data;
// };


// class CollisionEventObserver
// {

//     std::unordered_map<std::pair<int, int>, CollisionInfo> m_entitty_collisions;
//     std::unordered_map<std::pair<TypeId, int>, CollisionInfo> m_;
//     std::unordered_map<std::pair<TypeId, TypeId>, CollisionInfo> m_pair_collisions;
// };

class SpaceGame : public Game
{

public:
    SpaceGame(Renderer &window, KeyBindings &binds, Assets &assets);

    virtual void updateImpl(float dt) override;
    virtual void handleEventImpl(const SDL_Event &event) override;
    virtual void drawImpl(Renderer &window) override;

private:
    TextBubble& generateWord2(bool is_correct, utils::Vector2f pos, utils::Vector2f size);
    void registerSystems();
    void initUI();

    void startLevel(std::string level_id);

private:
    
    int m_lives_count = 3;
    std::size_t m_max_length = 3;
    std::shared_ptr<PlayerShip> m_player;
    std::unordered_map<int, TextBubble *> m_texts;

    std::unordered_map<std::string, CaveSpec2> m_levels; 

    utils::Grid m_text_grid;

    GridPosGenerator m_pos_generator;

    float m_camera_size = 700;
    utils::Vector2f level_size = {5000, 5000};

    //! UI Stuff
    float m_ui_header_height = 60;
    
    utils::Vector2f m_level_completed_text_pos;
    Text m_level_completed_text;
    Text m_player_hp_text;
    Text m_word_counts_text;

    std::unique_ptr<PostBox<EntityCreatedEvent>> m_spawn_listener;
    std::unique_ptr<PostBox<NewEntityEvent>> m_create_listener;
    std::unique_ptr<PostBox<EntityDiedEvent>> m_death_listener;

    std::unique_ptr<PostBox<LevelCompletedEvent>> m_level_listener;
    std::unique_ptr<PostBox<WordGuessedEvent>> m_guess_listener;
    std::size_t m_guessed_correct_count = 0;
    std::size_t m_guessed_wrong_count = 0;
    std::size_t m_correct_count = 0;
    std::size_t m_total_count = 0;

    int m_exam_count = 3;
};