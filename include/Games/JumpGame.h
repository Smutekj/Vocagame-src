#pragma once

#include "Game.h"
#include "ExerciseLoader.h"

class JumpGame : public Game
{

public:
    JumpGame(Renderer &window, KeyBindings &bindings, Assets &asset);
    virtual ~JumpGame() override {}

    virtual void updateImpl(const float dt) override;
    virtual void drawImpl(Renderer &window) override;
    // virtual void handleEventImpl(const SDL_Event &event);

    std::shared_ptr<GameLevelA> climbLevelFactory(int level_id, utils::Vector2f start_pos, utils::Vector2f dir);
    std::shared_ptr<GameLevelA> levelFactory(int level_id, utils::Vector2f start_pos, int direction);

    utils::Vector2f m_checkpoint_pos = {0.f, 0.f};
    void createCheckpoint(utils::Vector2f pos, utils::Vector2f size);
    void createFlag(utils::Vector2f pos, utils::Vector2f size);

    void registerCollisions();
    void registerSystems();
    void restartGame();
    void initializeUI();

private:
    TextBubble &generateWord(bool is_correct, utils::Vector2f pos, utils::Vector2f size, ExerciseGenerator &generator);

    RectangleSimple r = RectangleSimple({10, 11.1, 0.5, 1});
    TimedEventId text_animation_id = -1;
    float m_correct_scale = 1.f;
    float m_mistakes_scale = 1.f;

    float m_ui_header_height = 60;

    utils::Vector2f m_level_completed_text_pos;
    Text m_level_completed_text;

    std::unique_ptr<PostBox<EntityCreatedEvent>> m_spawn_listener;
    std::unique_ptr<PostBox<NewEntityEvent>> m_create_listener;
    std::unique_ptr<PostBox<EntityDiedEvent>> m_death_listener;

    std::unique_ptr<PostBox<LevelCompletedEvent>> m_level_listener;
    std::unique_ptr<PostBox<WordGuessedEvent>> m_guess_listener;
    std::size_t m_guessed_correct_count = 0;
    std::size_t m_guessed_wrong_count = 0;
    std::size_t m_correct_count = 0;
    std::size_t m_total_count = 0;

    float m_current_lvl_height = 800.f;

    // std::unordered_map<int, std::unordrered_set<int>> m_level2entities;

    void loadDesigns(const std::filesystem::path &designs_file);
    void randomWord(TextBubble::Spec &spec, bool is_correct);

    struct Design
    {
        bool isRightOriented() const
        {
            return origin.x < (bounds.pos_x + bounds.width / 2.f);
        }
        utils::Vector2f origin;
        Rectf bounds;
        std::vector<std::shared_ptr<GameObjectSpec>> specs;
    };
    std::vector<std::string> m_design_keys; //! for easy random selection of designs
    std::unordered_map<std::string, Design> m_designs;

    void create(utils::Vector2f pos, const Design &design);

    std::unique_ptr<ExerciseGenerator> m_exergenerator;
    std::size_t m_exam_count = 2;
};
