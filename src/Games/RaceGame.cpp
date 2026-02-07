#include "RaceGame.h"

#include "../Utils/RandomTools.h"
#include "../Systems/SpriteSystem.h"
#include "../Systems/TimedEventSystem.h"
#include "../Animation.h"

#include "nlohmann/json.hpp"
#include <filesystem>



RaceGame::RaceGame(Renderer &window, KeyBindings &bindings, Assets &assets)
    : Game(window, bindings, assets), m_word_grid({200, 60})
{
    registerCollisions();
    registerSystems();

    m_levels.emplace_back(levelFactory(0, m_playerx->getPosition()));

    m_right_word = generateNewWord();
    m_right_word_ui_text.setFont(m_font.get());
    m_right_word_ui_text.setText(m_right_word.translation);
    m_right_sequence = m_right_word.correct_form.substr(0,m_guess_seq_len);
}

void RaceGame::initializeUI()
{
}

void RaceGame::restartGame()
{
}

void RaceGame::registerCollisions()
{
    auto &colllider = m_world->getCollisionSystem();

    colllider.registerResolver(ObjectType::Bullet, ObjectType::Player);
    colllider.registerResolver(ObjectType::CharSeq, ObjectType::Player);
    colllider.registerResolver(ObjectType::Wall, ObjectType::Player);
    colllider.registerResolver(ObjectType::BoomBox, ObjectType::Player);
    colllider.registerResolver(ObjectType::TextBubble, ObjectType::Player);
    colllider.registerResolver(ObjectType::TextBubble, ObjectType::Bullet);
    colllider.registerResolver(ObjectType::Trigger, ObjectType::Player);
    colllider.registerResolver(ObjectType::ElectroWall, ObjectType::Player);
}

void RaceGame::registerSystems()
{
    auto &systems = m_world->m_systems;

    systems.registerSystem(
        std::make_shared<TimedEventSystem>(systems.getComponents<TimedEventComponent>()));
    systems.registerSystem(std::make_shared<SpriteSystem>(systems.getComponents<SpriteComponent>(),
                                                          m_layers));
    std::filesystem::path animation_directory = {
        std::string{RESOURCES_DIR} + "Textures/Animations/"};
    //    animation_directory /= ;
    auto animation_system = std::make_shared<AnimationSystem>(
        systems.getComponents<AnimationComponent>(),
        animation_directory, animation_directory);
;
    systems.registerSystem(animation_system);
}

std::string RaceGame::generateNewSequence()
{
    const auto &right_string = m_right_word.correct_form;

    std::size_t seq_size = m_right_sequence.size();
    m_guessing_index += seq_size;
    std::size_t inds_left = right_string.size() - m_guessing_index;

    assert(inds_left > 0);
    std::size_t new_seq_size = std::min({seq_size, inds_left});

    return right_string.substr(m_guessing_index, new_seq_size);
}

WordRepresentation RaceGame::generateNewWord()
{
    auto random_word_repre = utils::randomValue(m_repres);
    if(auto space_ind = random_word_repre.correct_form.find(' '); space_ind != std::string::npos)
    {
        random_word_repre.correct_form = random_word_repre.correct_form.substr(space_ind+1);
    }
    return random_word_repre;
};

std::shared_ptr<GameLevelA> RaceGame::levelFactory(int level_id, utils::Vector2f start_pos)
{
    auto level = std::make_shared<GameLevelA>(*m_world, level_id, messanger);

    m_on_char_guess = std::make_unique<PostBox<CharacterGuessEvent>>(messanger, [this](const auto &events)
                                                                          {
        for(const CharacterGuessEvent& e : events)
        {
            if(e.sequence == m_right_sequence)            
            {
               const auto& right_string = m_right_word.correct_form; 
                bool is_last_sequence = m_guessing_index + e.sequence.size() == right_string.size();  
                if(is_last_sequence)
                {
                    m_right_word = utils::randomValue(m_repres);
                    m_right_word_ui_text.setText(m_right_word.translation);
                    m_right_sequence = m_right_word.correct_form.substr(0,m_guess_seq_len);
                    m_found_sequence = "";
                    SoundSystem::play("WordCorrect1");
                }else{
                    m_found_sequence += m_right_sequence;
                    m_right_sequence = generateNewSequence();
                }
            }else{

            }
        } });

    auto spawn_char_event = [this](float t, int c)
    {
        bool is_correct = utils::randi(0,1);
        CharSeq::Spec spec;
        
        auto pos = m_playerx->getPosition() + Vec2(utils::randf(100, 400), utils::randf(0, 100));
        spec.pos = m_word_grid.getCellCenter(pos);
        if(m_word_grid.containsEntity(pos))
        {
            return;
        }
        if(is_correct)
        {
            spec.sequence = m_right_sequence;
        }else{
            const auto& rand_word = utils::randomValue(m_repres).correct_form;
            int start = utils::randi(0, rand_word.size() - m_guess_seq_len);
            spec.sequence = rand_word.substr(start, m_guess_seq_len);
        }
        spec.p_font = m_font.get();
        auto& word_ent = m_factories.at(TypeId::CharSeq)->create(spec);
        m_word_grid.insert(word_ent.getId(), pos);
    };

    m_timers.addInfiniteEvent(spawn_char_event, 1.f, 0.f);

    return level;
}

void RaceGame::drawImpl(Renderer &win)
{
    auto old_view = win.m_view;
    win.m_view = win.getDefaultView();
    utils::Vector2f win_size = win.getTargetSize();

    Sprite ui_background(*m_textures.get("UIHeaderFrame"));
    ui_background.setPosition(win_size.x / 2.f, win_size.y * 7.f / 8.f);
    auto bb = m_right_word_ui_text.getBoundingBox();
    ui_background.setScale(bb.width / 2.f + 30, m_ui_header_height / 2. * 1.5);
    win.drawSprite(ui_background);
    win.drawAll();

    Text m_right_word_ui_text;
    m_right_word_ui_text.setText(m_right_word.translation);
    m_right_word_ui_text.scale(m_right_word_scale, m_right_word_scale);
    m_right_word_ui_text.setFont(m_font.get());
    bb = m_right_word_ui_text.getBoundingBox();
    m_right_word_ui_text.centerAround({win_size.x * 1.f / 2.f, win_size.y * 7.f/8.f});
    m_right_word_ui_text.setColor({255, 0, 50, 255});
    m_window.drawText(m_right_word_ui_text);
    
    float text_y_pos = win_size.y*7./8.;
    float text_height = bb.height;
    m_right_word_ui_text.setText(m_found_sequence);
    bb = m_right_word_ui_text.getBoundingBox();
    m_right_word_ui_text.setColor({25,255,25,255});
    m_right_word_ui_text.centerAround({win_size.x * 1.f / 2.f, text_y_pos - text_height - 10});
    m_window.drawText(m_right_word_ui_text);
    win.drawAll();
    /*
        Text word_counts = {"Correct: " + std::to_string(m_guessed_correct_count) + " / " + std::to_string(m_correct_count)};
        word_counts.setFont(m_font.get());
        word_counts.setScale(m_correct_scale, m_correct_scale);
        word_counts.setColor({25, 255, 25, 255});
        bb = word_counts.getBoundingBox();
        word_counts.centerAround({win_size.x - win_size.x / 4., win_size.y - m_ui_header_height / 2.f});
        win.drawText(word_counts);

        word_counts.setScale(m_mistakes_scale, m_mistakes_scale);
        word_counts.setText("Mistakes: " + std::to_string(m_guessed_wrong_count));
        bb = word_counts.getBoundingBox();
        word_counts.setPosition(win_size.x * 8. / 10. - bb.width / 2.f, win_size.y * 9. / 10.f - bb.height / 2. - 50);
        word_counts.setColor({255, 20, 20, 255});
        // win.drawText(word_counts);

        m_level_completed_text.centerAround(m_level_completed_text_pos);
        win.drawText(m_level_completed_text) */
    ;
    // Text timer;

    win.m_view = old_view;
};
