#include "GameLevel.h"

GameLevel::GameLevel(GameWorld &world, int level_id, PostOffice &messanger) : m_world(world), m_id(level_id)
{
    m_on_entity_spawn = std::make_unique<PostBox<NewEntityEvent>>(messanger, [this](const auto &messages)
                                                                  {
            for (const auto &msg : messages)
            {
                m_entity_ids.insert(msg.id);
                m_entities.insert({msg.id, msg.obj}); 
            } });
    m_on_entity_died = std::make_unique<PostBox<EntityDiedEvent>>(messanger, [this](const auto &messages)
                                                                  {
            for (const auto &msg : messages)
            {
                m_entity_ids.erase(msg.id);
                m_entities.erase(msg.id);
            } });
}
void GameLevel::killEntities()
{
    for (auto ent_id : m_entity_ids)
    {
        m_world.get(ent_id)->kill();
    }
    m_entity_ids.clear();
}

void GameLevel::update(float dt)
{
    updateImpl(dt);

    for (auto &spawner : m_spawners2)
    {
        spawner->update(dt);
    }
    m_duration += dt;
}

float GameLevel::getDuration() const
{
    return m_duration;
}

void GameLevel::updateImpl(float dt) {};

GameLevelA::GameLevelA(GameWorld &world, int level_id, PostOffice &messanger)
    : GameLevel(world, level_id, messanger)
{
    m_on_guessing = std::make_unique<PostBox<WordGuessedEvent>>(messanger, [this](const auto &messages)
                                                                {
            for(const auto& msg : messages)
            {
                if(msg.was_correct)
                {
                    m_level_score++;
                }else{
                    m_word2stats[msg.entity_id] = {};
                    m_word2stats[msg.entity_id].mistakes_count++;
                    m_word2stats[msg.entity_id].correct_form = msg.correct_form;
                    m_word2stats[msg.entity_id].shown_form = msg.shown_form;
                    m_word2stats[msg.entity_id].translation = msg.translation;
                }
            } });

    m_on_creation = std::make_unique<PostBox<NewEntityEvent>>(messanger, [this](const auto &messages)
                                                              {
        for (const auto& e: messages)
        {
            if (e.obj->getType() == TypeId::TextBubble)
            {
                m_total_word_count++;
                auto &obj = static_cast<TextBubble &>(*e.obj);
                m_correct_word_count += obj.isCorrect();
            }
        } });
}

void GameLevelA::updateImpl(float dt)
{
}
