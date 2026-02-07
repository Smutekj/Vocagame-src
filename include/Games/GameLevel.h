#include "../PostOffice.h"
#include "../Systems/TimedEventManager.h"
#include "../Entities/Spawners.h"

class GameWorld;

class GameLevel
{

public:
    GameLevel(GameWorld &world, int level_id, PostOffice &messanger);
    virtual ~GameLevel() = default;

    void killEntities();

    void update(float dt);

    void deactivate();

    float getDuration() const;

private:
    virtual void updateImpl(float dt);

public:
    int m_id = -1;
    std::vector<std::shared_ptr<SpawnerI>> m_spawners2;
    std::function<void(GameLevel &)> m_on_stage_end = [](auto &) {};
    std::function<void(GameLevel& )> m_on_stage_start = [](auto&) {};

    std::unordered_set<int> m_entity_ids;
    std::unordered_map<int, std::shared_ptr<GameObject>> m_entities;

    std::unique_ptr<PostBox<NewEntityEvent>> m_on_entity_spawn;
    std::unique_ptr<PostBox<EntityDiedEvent>> m_on_entity_died;
protected:
    GameWorld &m_world;

private:
    float m_duration = 0.f;
};

class GameLevelA : public GameLevel
{
public:
    GameLevelA(GameWorld &world, int level_id, PostOffice &messanger);
    virtual ~GameLevelA() = default;
    
    
    private:
    virtual void updateImpl(float dt) override;
    
    TimedEventManager m_timers;
    
    struct WordStats
    {
        std::string shown_form;
        std::string translation;
        std::string correct_form;
        int mistakes_count = 0;
    };
    
public:
    utils::Vector2f m_level_end_pos;

    std::unique_ptr<PostBox<WordGuessedEvent>> m_on_guessing;
    std::unique_ptr<PostBox<NewEntityEvent>> m_on_creation;
    std::unordered_map<int, WordStats> m_word2stats;



    int m_level_score = 0;
    int m_level_max_score = 0;
    std::size_t m_total_word_count = 0;
    std::size_t m_correct_word_count= 0;
};
