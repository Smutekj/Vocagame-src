#include "CastleGame.h"

#include "../Utils/RandomTools.h"
#include "../Systems/SpriteSystem.h"
#include "../Systems/TimedEventSystem.h"
#include "../Entities/Trigger.h"
#include "../Animation.h"

#include "nlohmann/json.hpp"
#include <filesystem>

CastleGame::CastleGame(Renderer &window, KeyBindings &bindings, Assets &assets)
    : Game(window, bindings, assets)
{
    registerCollisions();
    registerSystems();

    //! PLAYER NEEDS TO BE FIRST BECAUSE OTHER OBJECTS MIGHT REFERENCE IT!
    auto p = m_world->insertObject([this](int id)
                                      { return std::make_shared<PlayeEntity>(m_world.get(), id); });
    m_playerx = std::static_pointer_cast<Player>(p).get();
    m_playerx->setPosition({300, 300});
    m_playerx->setSize({60, 60});
    m_playerx->setDestructionCallback(
        [this](int id, ObjectType type) {});
    m_world->m_player = m_playerx;

    m_camera.threshold_rect = {0.5f, 0.40f, 0.0f, 0.2f};
    m_camera.m_can_move_y = true;
    m_camera.setFolowee(m_playerx);
    
    std::filesystem::path cave_json_path = std::string{RESOURCES_DIR} + "Levels/castleLevels.json";
    m_cave_specs = readCaveSpecs2(cave_json_path);

    m_camera.setSize({800.f, window.getTarget().getAspect() * 800.f});
    Vec2 start_pos = {m_playerx->getPosition().x, m_playerx->getPosition().y - 50};
    m_checkpoint_pos = m_playerx->getPosition();
    m_levels.emplace_back(levelFactory(0, start_pos));
    m_levels.emplace_back(levelFactory(1, m_levels.back()->m_level_end_pos));

    m_guess_listener = std::make_unique<PostBox<WordGuessedEvent>>(messanger, [this](const auto &events)
                                                                   {
        for(const WordGuessedEvent& e : events)
        {
            m_guessed_correct_count += e.was_correct;
            m_guessed_wrong_count += !e.was_correct;
            scaleAnimation(0.5, 0., 0.15,e.was_correct ? m_correct_scale : m_mistakes_scale, m_timers);
            if(!e.was_correct)
            {
                m_playerx->health--;
                if(m_playerx->health == 0)
                {
                    js::startExam(3);
                    m_playerx->health = 3;
                }
            }

        } });

    initializeUI();
}

void CastleGame::initializeUI()
{
    m_level_completed_text.m_is_centered = true;
    m_level_completed_text.setFont(m_font.get());
    m_level_completed_text.setScale(1.5f, 1.5f);
    m_level_completed_text.setColor({0, 255, 0, 255});
    m_level_listener = std::make_unique<PostBox<LevelCompletedEvent>>(messanger, [this](const auto &events)
                                                                      {
        auto win_size = m_window.getTargetSize();
        for(const LevelCompletedEvent& e : events)
        {
            m_level_completed_text.setText("Level " + std::to_string(e.level) + " complete!");
            utils::Vector2f final_text_pos = {win_size.x/2.f, win_size.y  - m_ui_header_height*1.75};
            utils::Vector2f completed_text_pos = {m_window.getTargetSize().x / 2.f, m_window.getTargetSize().y * 12. / 10.};
            positionAnimation(1.f, 0.f, completed_text_pos, final_text_pos, m_level_completed_text, m_timers);
            positionAnimation(1.f, 5.f, final_text_pos, completed_text_pos, m_level_completed_text, m_timers);
        } });
}

void CastleGame::restartGame()
{
}

std::shared_ptr<GameLevelA> CastleGame::levelFactory(int level_id, utils::Vector2f start_pos)
{
    auto level = std::make_shared<GameLevelA>(*m_world, level_id, messanger);

    auto gen_word_spec = [this](bool is_correct) -> WordSpec
    {
        WordSpec spec;
        auto random_word_repre = utils::randomValue(m_repres);
        if (is_correct)
        {
            spec.text = random_word_repre.correct_form;
        }
        else
        {
            int n_forms = random_word_repre.shown_forms.size();
            spec.text = random_word_repre.shown_forms.at(utils::randi(0, n_forms - 1));
        }
        spec.type = WordType::Dodger;
        spec.translation = random_word_repre.translation;
        spec.correct_form = random_word_repre.correct_form;
        return spec;
    };

    auto cave_specifier = [this, gen_word_spec](CaveSpec2 &spec, float t, int c)
    {
        spec = utils::randomValue(m_cave_specs);

        for (auto &ospec : spec.objects)
        {
            if (ospec->obj_type == ObjectType::TextBubble)
            {
                auto &wspec = static_cast<TextBubble::Spec &>(*ospec);
                auto word_spec = gen_word_spec(utils::randi(0, 2));
                wspec.text = word_spec.text;
                wspec.correct_form = word_spec.correct_form;
                wspec.translation = word_spec.translation;
            }
        }
        return spec;
    };

    float space = 300;

    CaveSpec2 spec{};
    cave_specifier(spec, 0.f, 0);
    float cave_width = spec.bounds.width;
    spec.pos = start_pos - (spec.start + spec.origin);

    for (auto &p_spec : spec.objects)
    {
        auto &obj = m_factories[p_spec->obj_type]->create(*p_spec);
        obj.setPosition(spec.pos + obj.getPosition());
    }

    Wall::Spec w_spec;
    w_spec.pos = start_pos;
    w_spec.size = {200, 20};
    m_factories.at(ObjectType::Wall)->create(w_spec);
    w_spec.pos = spec.end - (spec.start + spec.origin) + start_pos;
    int platform_id = m_factories.at(ObjectType::Wall)->create(w_spec).getId();

    auto end_pos = w_spec.pos;
    auto &safety_net = m_factories.at(TypeId::Trigger)->create(Trigger::Spec{});
    auto level_width = end_pos.x - start_pos.x;
    safety_net.setSize({level_width, 200.f});
    safety_net.setPosition({(start_pos.x + end_pos.x) / 2.f, (spec.bounds.pos_y + spec.origin.y) - 250.f});
    safety_net.m_collision_resolvers[ObjectType::Player] = [this](auto &player, auto &c_data)
    {
        player.setPosition(m_checkpoint_pos);
        player.m_vel = {0.f, 20.f};
    };

    level->m_level_end_pos = end_pos;

    VisualObject::Spec flag_spec;
    flag_spec.sprite.shader_id = "flag";
    flag_spec.sprite.texture_id = "Flag_" + m_studied_language;
    flag_spec.sprite.layer_id = "Unit";
    flag_spec.pos = {start_pos.x, start_pos.y + 80. / 2.f + 5.f};
    flag_spec.size = {80.f, 50.f};

    m_factories.at(ObjectType::VisualObject)->create(flag_spec);
    auto &checkpoint = m_world->addObject3(ObjectType::Trigger);
    checkpoint.setSize({w_spec.size.x, w_spec.size.y});
    checkpoint.setPosition({w_spec.pos.x, w_spec.pos.y + checkpoint.getSize().y / 2.f + spec.size.y / 2.f});
    checkpoint.m_collision_resolvers[ObjectType::Player] = [this, &checkpoint, level, w_spec](auto &player,
                                                                                              auto &c_data)
    {
        level->m_on_stage_end(*level);

        const auto &last_lvl = m_levels.back();
        m_factories.at(ObjectType::Wall)->create(w_spec);
        //! there are always two levels, and the last one is the one that follows
        m_levels.back()->m_on_stage_start(*m_levels.back());
        m_levels.push_back(levelFactory(last_lvl->m_id + 1, last_lvl->m_level_end_pos)); //! create next lvl

        if (m_levels.size() >= 3)
        {
            m_levels.pop_front(); //! remove current lvl
        }
        checkpoint.kill();
    };

    CollisionComponent c_comp;
    c_comp.shape.convex_shapes.emplace_back(4);
    c_comp.type = ObjectType::Trigger;
    m_world->m_systems.addEntityDelayed(checkpoint.getId(), c_comp);

    level->m_on_stage_start = [this, start_pos](auto &l)
    {
        m_effects_factory->create(EffectId::StarBurst, m_playerx->getPosition(), Vec2{1, 1}, 2.f);

        m_checkpoint_pos = {start_pos.x, start_pos.y + 20.f};
        m_correct_count = 0;
        m_guessed_correct_count = 0;
        for (auto [id, p_obj] : l.m_entities)
        {
            if (p_obj->getType() == TypeId::TextBubble)
            {
                m_correct_count += static_cast<TextBubble &>(*p_obj).isCorrect();
            }
        };
    };
    level->m_on_stage_end = [this, level, platform_id](auto &l)
    {
        messanger.send(LevelCompletedEvent{.stage = GameStage::Dodge,
                                           .level = level->m_id,
                                           .time = level->getDuration()});
        //! make them move away;
        for (auto [id, p_obj] : l.m_entities)
        {
            if (id == platform_id)
            {
                continue;
            }
            TransformComponent t_comp;
            t_comp.duration = 1.5f;
            t_comp.target_pos = p_obj->getPosition() + Vec2{-600.f, 0.f};
            m_world->m_systems.add(t_comp, p_obj->getId());
        }
        TimedEvent kill_entities = {[level](float t, int c)
                                    {
                                        level->killEntities();
                                    },
                                    1.5f};
        m_timers.addTimedEvent(kill_entities);
    };

    messanger.distributeMessages();
    level->m_on_entity_spawn = nullptr;

    return level;
}

void CastleGame::registerCollisions()
{
    auto &colllider = m_world->getCollisionSystem();

    colllider.registerResolver(ObjectType::Bullet, ObjectType::Player);
    colllider.registerResolver(ObjectType::Wall, ObjectType::Player);
    colllider.registerResolver(ObjectType::BoomBox, ObjectType::Player);
    colllider.registerResolver(ObjectType::Wall, ObjectType::TextBubble);
    colllider.registerResolver(ObjectType::TextBubble, ObjectType::Player);
    colllider.registerResolver(ObjectType::TextBubble, ObjectType::Bullet);
    colllider.registerResolver(ObjectType::Wall, ObjectType::Bullet);
    colllider.registerResolver(ObjectType::PathingWall, ObjectType::Player);
    colllider.registerResolver(ObjectType::PathingWall, ObjectType::Bullet);
    colllider.registerResolver(ObjectType::Trigger, ObjectType::Player);
    colllider.registerResolver(ObjectType::Trigger, ObjectType::Wall);
    colllider.registerResolver(ObjectType::Trigger, ObjectType::TextBubble);
    colllider.registerResolver(ObjectType::ElectroWall, ObjectType::Player);
}

void CastleGame::registerSystems()
{
    auto &systems = m_world->m_systems;

    systems.registerSystem(std::make_shared<TransformSystem>(systems.getComponents<TransformComponent>()));
    systems.registerSystem(std::make_shared<TimedEventSystem>(systems.getComponents<TimedEventComponent>()));
    systems.registerSystem(std::make_shared<SpriteSystem>(systems.getComponents<SpriteComponent>(), m_layers));

    std::filesystem::path animation_directory = {
        std::string{RESOURCES_DIR} + "Textures/Animations/"};
    //    animation_directory /= ;
    auto animation_system = std::make_shared<AnimationSystem>(
        systems.getComponents<AnimationComponent>(),
        animation_directory, animation_directory);

    animation_system->registerAnimation(m_atlases.get("PurpleExplosion"), "PurpleExplosion");
    animation_system->registerAnimation(m_atlases.get("BlueExplosion"), "BlueExplosion");

    systems.registerSystem(animation_system);
}

void CastleGame::drawImpl(Renderer &win)
{

    auto old_view = win.m_view;
    win.m_view = win.getDefaultView();
    utils::Vector2f win_size = win.getTargetSize();

    Sprite ui_background(*m_textures.get("UIHeaderFrame"));
    ui_background.setPosition(win_size.x / 4.f, win_size.y - m_ui_header_height / 2.f);
    ui_background.setScale(100, m_ui_header_height / 2.f);
    win.drawSprite(ui_background);
    ui_background.setPosition(win_size.x - win_size.x / 4.f, win_size.y - m_ui_header_height / 2.f);
    win.drawSprite(ui_background);
    ui_background.setPosition(m_level_completed_text.getPosition());
    auto bb = m_level_completed_text.getBoundingBox();
    ui_background.setScale(bb.width / 2.f + 30, m_ui_header_height / 2. * 1.5);
    win.drawSprite(ui_background);
    win.drawAll();

    Text player_hp;
    player_hp.setText("Lives: " + std::to_string((int)m_playerx->health));
    player_hp.scale(m_mistakes_scale, m_mistakes_scale);
    player_hp.setFont(m_font.get());
    bb = player_hp.getBoundingBox();
    player_hp.centerAround({win_size.x * 1.f / 4.f, win_size.y - m_ui_header_height / 2.f});
    player_hp.setColor({255, 0, 50, 255});
    m_window.drawText(player_hp);

    Text word_counts = Text{"Correct: " + std::to_string(m_guessed_correct_count) + " / " + std::to_string(m_correct_count)};
    word_counts.setFont(m_font.get());
    word_counts.setScale(m_correct_scale, m_correct_scale);
    word_counts.setColor({25, 255, 25, 255});
    bb = word_counts.getBoundingBox();
    word_counts.centerAround({win_size.x - win_size.x / 4., win_size.y - m_ui_header_height / 2.f});
    win.drawText(word_counts);

    win.drawText(m_level_completed_text);
    // Text timer;
    win.drawAll();
    word_counts.setScale(1, 1);

    win.m_view = old_view;
}