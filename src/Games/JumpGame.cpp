#include "JumpGame.h"

#include "../Utils/RandomTools.h"
#include "../Systems/SpriteSystem.h"
#include "../Systems/TimedEventSystem.h"
#include "../Systems/PathSystem.h"
#include "../Animation.h"

#include "Utils/IOUtils.h"
#include "TextureAtlas.h"

using namespace utils;

TextBubble &JumpGame::generateWord(bool is_correct, utils::Vector2f pos, utils::Vector2f size, ExerciseGenerator &generator)
{
    TextPlatform::Spec spec;
    auto random_word_repre = generator.generate(is_correct);

    spec.pos = pos;
    spec.size = size;
    spec.text = random_word_repre.shown_word;
    spec.meaning_id = random_word_repre.meaning;
    spec.translation = random_word_repre.translation;
    spec.correct_form = random_word_repre.correct_form;
    auto &word = static_cast<TextBubble &>(m_world->createObject(spec));

    std::string correct_no_space = spec.meaning_id;

    if (std::size_t pos = spec.meaning_id.find(' '); pos != std::string::npos)
    {
        std::string correct_no_space = spec.meaning_id.erase(spec.meaning_id.find(' '), 1);
    }

    std::cout << correct_no_space << std::endl;
    if (m_atlases.contains(correct_no_space))
    {
        word.setImage(m_atlases.getSprite(correct_no_space));
        word.setSize({size.x, size.y});
    }
    return word;
};

void JumpGame::loadDesigns(const std::filesystem::path &designs_file_path)
{
    const auto json_data = utils::loadJson(designs_file_path.c_str());

    for (auto &[id, design_data] : json_data.items())
    {
        std::vector<std::shared_ptr<GameObjectSpec>> specs;
        Design design;

        fromJson(design.bounds, "bounds", design_data);
        fromJson(design.origin, "origin", design_data);

        for (auto &spec_json : design_data.at("objects"))
        {
            std::string type_name = spec_json["obj_type"];
            try
            {
                auto type_id = stringToEnum<ObjectType>(type_name);
                auto new_spec = deserializeSpec(type_id, spec_json);
                if (new_spec)
                {
                    design.specs.push_back(new_spec);
                }
            }
            catch (std::exception &e)
            {
                std::cout << "Enum does not exist: " << e.what() << std::endl;
            }
        }

        m_designs[id] = design;
        m_design_keys.push_back(id);
    }
}

void JumpGame::randomWord(TextBubble::Spec &spec, bool is_correct)
{
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
    spec.translation = random_word_repre.translation;
    spec.correct_form = random_word_repre.correct_form;
    spec.glow_color = {0, 0, 0, 0};
}

void JumpGame::create(utils::Vector2f pos, const Design &design)
{

    utils::Vector2f origin = {design.origin};

    for (auto spec : design.specs)
    {
        if (spec->obj_type == TypeId::TextBubble || spec->obj_type == TypeId::TextPlatform)
        {
            auto &wspec = static_cast<TextBubble::Spec &>(*spec);
            randomWord(wspec, randi(0, 1));
        }
        auto spec_pos = spec->pos;
        spec->pos += pos - origin;
        auto new_obj = m_factories.at(spec->obj_type)->create(*spec);
        spec->pos = spec_pos;
    }
}
JumpGame::JumpGame(Renderer &window, KeyBindings &bindings, Assets &assets)
    : Game(window, bindings, assets)
{

    std::vector<TranslationExercise> exers;
    TranslationExerciseLoader loader;
    for (auto &word_group : m_topics)
    {

#if defined(__EMSCRIPTEN__)
        std::filesystem::path word_file = "/execDb/" + word_group + ".json";
#else
        std::filesystem::path word_file = std::string{RESOURCES_DIR} + "execDb/" + word_group + ".json";
#endif

        auto new_exers = loader.loadExercises(word_group, word_file);
        exers.insert(exers.end(), new_exers.begin(), new_exers.end());
    }
    m_exergenerator = std::make_unique<ExerciseGenerator>(exers);

    registerCollisions();
    registerSystems();

    //! PLAYER NEEDS TO BE FIRST BECAUSE OTHER OBJECTS MIGHT REFERENCE IT!
    auto p = m_world->insertObject([this](int id)
                                   { return std::make_shared<PlayeEntity>(m_world.get(), id); });
    m_playerx = std::static_pointer_cast<Player>(p).get();
    m_playerx->setPosition({300, 300});
    m_playerx->setSize({50, 50});

    m_camera.threshold_rect = {0.5f, 0.40f, 0.0f, 0.2f};
    m_camera.m_can_move_y = true;
    m_camera.setFolowee(m_playerx);

    m_background_tex = m_assets.textures.get("SkyNight2");
    m_background.setTexture(*m_background_tex);

    std::filesystem::path designs_path = std::string(RESOURCES_DIR) + "Levels/climbWalls.json";
    loadDesigns(designs_path);

    float aspect = window.getTarget().getAspect();
    m_camera.setSize({1000.f / aspect, 1000.f});
    m_levels.emplace_back(climbLevelFactory(0, {m_playerx->getPosition().x, m_playerx->getPosition().y - 20},
                                            {0, 1}));

    m_levels.front()->m_on_stage_start(*m_levels.front());

    m_checkpoint_pos = m_playerx->getPosition();
    //! add fall trigger
    Trigger::Spec ts;
    ts.pos = {m_checkpoint_pos.x, m_checkpoint_pos.y - 1000.f};
    ts.size = {10000.f, 200.f};
    auto &fall_trigger = m_world->createObject(ts);
    fall_trigger.m_collision_resolvers[ObjectType::Player] = [this](auto &player, auto &c_data)
    {
        m_playerx->m_vel *= 0.0f;
        m_playerx->setPosition(m_checkpoint_pos);
    };

    m_guess_listener = std::make_unique<PostBox<WordGuessedEvent>>(messanger, [this](const auto &events)
                                                                   {
        for(const WordGuessedEvent& e : events)
        {
            m_guessed_correct_count += e.was_correct;
            m_guessed_wrong_count += !e.was_correct;
            scaleAnimation(0.5, 0., 0.15, e.was_correct ? m_correct_scale : m_mistakes_scale, m_timers);

        } });

    m_level_completed_text.m_is_centered = true;
    m_level_completed_text.setFont(m_font.get());
    m_level_completed_text.setScale(1.5f, 1.5f);
    m_level_completed_text.setColor({0, 255, 0, 255});
    m_level_listener = std::make_unique<PostBox<LevelCompletedEvent>>(messanger, [this](const auto &events)
                                                                      {
        auto win_size = m_window.getTargetSize();
        for(const LevelCompletedEvent& e : events)
        {
            m_level_completed_text_pos.x = win_size.x/2.f;
            m_level_completed_text.setText("Level " + std::to_string(e.level) + " complete!");
            utils::Vector2f final_text_pos = {m_level_completed_text_pos.x, win_size.y  - m_ui_header_height*1.75};
            utils::Vector2f outside_text_pos = {m_window.getTargetSize().x / 2.f, m_window.getTargetSize().y * 12. / 10.};

            positionAnimation(1.f, 0.f, outside_text_pos, final_text_pos, m_level_completed_text, m_timers);
            positionAnimation(1.f, 5.f, final_text_pos, outside_text_pos, m_level_completed_text, m_timers);
            m_timers.runDelayed([this](){
                m_level_completed_text.setPosition(-500, -500);
            }, 5.f);
        } });
}
void JumpGame::updateImpl(const float dt)
{
    for (auto ent_p : m_world->getEntities().data())
    {
    }

    //! move background with camera;
    utils::Vector2f bg_tex_pos = m_camera.getView().getCenter() / 2.f;
    auto bg_size = m_background_tex->getSize();
    m_background.m_tex_rect = {(int)bg_tex_pos.x, 0, (int)(bg_size.x / 1.2), (int)bg_size.y};

    if (m_playerx->health <= 0)
    {
        js::startExam(3);
        m_exam_count++;
        m_playerx->health = 3;
    }
}

void JumpGame::initializeUI()
{
}

void JumpGame::restartGame()
{
}

std::shared_ptr<GameLevelA> JumpGame::climbLevelFactory(int level_id, utils::Vector2f start_pos, utils::Vector2f direction)
{
    auto level = std::make_shared<GameLevelA>(*m_world, level_id, messanger);

    m_create_listener = std::make_unique<PostBox<NewEntityEvent>>(messanger, [this](const auto &events)
                                                                  {
        for(const NewEntityEvent& e : events)
        {
           if(e.obj->getType() == TypeId::TextBubble || e.obj->getType() == TypeId::TextPlatform)
           {
               m_total_count++;
               auto& obj = static_cast<TextBubble&>(*e.obj);
               m_correct_count += obj.isCorrect(); 
           } 
        } });

    float cell_width = 160.f;
    float level_height = 200.f;
    auto gen_platform_spec = [&](int level, int platform_cell_id) -> Wall::Spec
    {
        Wall::Spec spec;

        float width = cell_width / 3.f + randf(0, cell_width / 4.f);
        spec.size = {width, 20.f};

        float height = level * level_height + randf(0.f, level_height / 10.f);
        float center = (platform_cell_id + 0.5f) * cell_width + randf(-cell_width / 6.f, cell_width / 6.f);
        spec.pos = {center, height};
        spec.pos = start_pos + utils::Vector2f{spec.pos.x, spec.pos.y};
        return spec;
    };

    auto gen_word = [&](bool is_correct, int cell_id, int height_id, float y_disp, float x_disp = 0.f)
    {
        utils::Vector2f pos = {(cell_id + 0.5f) * cell_width + x_disp, height_id * level_height + y_disp};
        utils::Vector2f size = {cell_width * 0.85, 35.f};
        auto &word = generateWord(is_correct, pos, size, *m_exergenerator);
    };

    auto make_word_line = [this, cell_width](utils::Vector2f center, int word_count, int correct_count)
    {
        bool kek = randi(0, 1);
        int words_on_line = randi(2, 5);

        utils::Vector2f word_size = {cell_width * 0.45, cell_width * 0.45};
        if (words_on_line <= 4)
        {
            auto &right_word = generateWord(kek, {center.x - cell_width, center.y}, word_size, *m_exergenerator);
            auto &wrong_word = generateWord(!kek, {center.x + cell_width, center.y}, word_size, *m_exergenerator);
        }
        else
        {
            std::vector<int> corrects = {1, randi(0, 1), 0};
            utils::permute(corrects);
            int n = corrects.size();
            for (int x_id = 0; x_id < n; ++x_id)
            {
                float x = center.x - (x_id - n / 2) * cell_width;
                auto &word = generateWord(corrects.at(x_id), {x, center.y + randf(0.f, 20.f)}, word_size, *m_exergenerator);
            }
        }
    };

    utils::Vector2f center = {start_pos.x, start_pos.y + level_height};

    Wall::Spec spec;
    spec.pos = {start_pos.x, start_pos.y};
    spec.size = {cell_width, 20};
    auto &platform_start = m_factories.at(ObjectType::Wall)->create(spec);

    for (int height_id = 0; height_id < level_id + 1; ++height_id)
    {

        int levels_to_next_platform = randi(4, 6);
        bool vertical = true;
        for (int y_id = 0; y_id < levels_to_next_platform; ++y_id)
        {
            int word_count = randi(2, 3);
            int correct_count = randi(1, word_count);
            make_word_line(center, word_count, correct_count);
            center.y += level_height;
            center.x += randf(-50.f, 50.f);
        }

        auto &design = m_designs.at(randomValue(m_design_keys));
        create({center.x, center.y}, design);
        center.y += design.bounds.height + level_height;

        spec.size = {cell_width, 20.f};
        spec.pos = {center.x, center.y};
        auto &platform_left = m_factories.at(ObjectType::Wall)->create(spec);
        center.y += level_height;
    }
    m_current_lvl_height = spec.pos.y;
    createFlag(start_pos, {50, 50});
    createCheckpoint(spec.pos, {50, 50});
    //! level ends on last platform
    level->m_level_end_pos = spec.pos;

    Trigger::Spec ts;
    ts.pos = {spec.pos.x, spec.pos.y + 25};
    ts.size = {50, 50};
    auto &trigger = m_world->createObject(ts);
    trigger.m_collision_resolvers[ObjectType::Player] = [this, &trigger, level, direction, spec](auto &player,
                                                                                                 auto &c_data)
    {
        level->m_on_stage_end(*level);

        messanger.send(LevelCompletedEvent{.stage = GameStage::Dodge,
                                           .level = level->m_id,
                                           .time = level->getDuration()});

        const auto &last_lvl = m_levels.back();
        m_factories.at(ObjectType::Wall)->create(spec);
        m_levels.push_back(climbLevelFactory(last_lvl->m_id + 1, last_lvl->m_level_end_pos, {0, 1})); //! create next lvl

        m_levels.back()->m_on_stage_start(*m_levels.back());
        // if (m_levels.size() >= 3)
        {
            // m_levels.front()->killEntities();
            m_levels.pop_front(); //! remove current lvl
        }
        trigger.kill();
    };

    level->m_on_creation = nullptr; //! stop registering new entities
    level->m_on_stage_end = [this, level](auto &l)
    {
        for (auto [id, p_obj] : l.m_entities)
        {
            if (p_obj->getType() == TypeId::TextBubble)
            {
                //! make text arrive from top
                TransformComponent t_comp;
                t_comp.duration = 1.f;
                t_comp.target_pos = p_obj->getPosition() - Vec2{0.f, 400.f};
                m_world->m_systems.add(t_comp, p_obj->getId());
            }
            else
            {
                p_obj->kill();
            }
        }
        TimedEvent kill_entities = {[level](float t, int c)
                                    {
                                        level->killEntities();
                                    },
                                    1.5f};
        m_timers.addTimedEvent(kill_entities);
    };
    level->m_on_stage_start = [this](auto &l)
    {
        m_correct_count = 0;
        m_guessed_correct_count = 0;
        for (auto [id, p_obj] : l.m_entities)
        {
            //! make text arrive from top
            if (p_obj->getType() == TypeId::TextBubble || p_obj->getType() == TypeId::TextPlatform)
            {
                m_correct_count += static_cast<TextBubble &>(*p_obj).isCorrect();
                TransformComponent t_comp;
                t_comp.duration = 1.f;
                t_comp.target_pos = p_obj->getPosition();
                p_obj->setPosition(p_obj->getPosition() + Vec2{0, 400.f});
                m_world->m_systems.addDelayed(t_comp, p_obj->getId());
            }
        }
    };

    messanger.distributeMessages();
    level->m_on_entity_spawn = nullptr;
    return level;
}

void JumpGame::registerCollisions()
{
    auto &colllider = m_world->getCollisionSystem();

    colllider.registerResolver(ObjectType::Bullet, ObjectType::Player);
    colllider.registerResolver(ObjectType::Wall, ObjectType::Player);
    colllider.registerResolver(ObjectType::BoomBox, ObjectType::Player);
    colllider.registerResolver(ObjectType::Wall, ObjectType::TextBubble);
    colllider.registerResolver(ObjectType::TextBubble, ObjectType::Player);
    colllider.registerResolver(ObjectType::TextPlatform, ObjectType::Player);
    colllider.registerResolver(ObjectType::TextBubble, ObjectType::Bullet);
    colllider.registerResolver(ObjectType::Wall, ObjectType::Bullet);
    colllider.registerResolver(ObjectType::PathingWall, ObjectType::Player);
    colllider.registerResolver(ObjectType::PathingWall, ObjectType::Bullet);
    colllider.registerResolver(ObjectType::Trigger, ObjectType::Player);
    colllider.registerResolver(ObjectType::Trigger, ObjectType::Wall);
    colllider.registerResolver(ObjectType::Trigger, ObjectType::TextBubble);
    colllider.registerResolver(ObjectType::ElectroWall, ObjectType::Player);
}

void JumpGame::registerSystems()
{
    auto &systems = m_world->m_systems;

    systems.registerSystem(std::make_shared<TimedEventSystem>(systems.getComponents<TimedEventComponent>()));
    systems.registerSystem(std::make_shared<TransformSystem>(systems.getComponents<TransformComponent>()));
    systems.registerSystem(std::make_shared<SpriteSystem>(systems.getComponents<SpriteComponent>(), m_layers));
    systems.registerSystem(std::make_shared<PathSystem>(systems.getComponents<PathComponent>(), m_layers));

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

void JumpGame::createFlag(utils::Vector2f pos, utils::Vector2f size)
{

    VisualObject::Spec flag_spec;
    flag_spec.sprite.shader_id = "flag";
    flag_spec.sprite.texture_id = "Flag_" + m_studied_language;
    flag_spec.sprite.layer_id = "Unit";
    flag_spec.pos = {pos.x, pos.y + size.y / 2.f + 5.f};
    flag_spec.size = {size.x * 17. / 24., size.y};
    m_factories.at(ObjectType::VisualObject)->create(flag_spec);
}

void JumpGame::createCheckpoint(utils::Vector2f pos, utils::Vector2f size)
{
    createFlag(pos, size);

    Trigger::Spec ts;
    ts.size = size;
    ts.pos = {pos.x, pos.y + size.y / 2.f + 5.f};
    auto &checkpoint_trigger = m_world->createObject(ts);
    checkpoint_trigger.m_collision_resolvers[ObjectType::Player] = [=, this, &checkpoint_trigger](auto &player, auto &c_data)
    {
        m_effects_factory->create(EffectId::Fireworks1, pos, {1, 1}, 1.f);
        m_effects_factory->create(EffectId::StarBurst, pos, {1, 1}, 1.f);

        SoundSystem::play("LevelCompleted1");

        m_checkpoint_pos = checkpoint_trigger.getPosition();
        Trigger::Spec ts;
        ts.pos = {m_checkpoint_pos.x, m_checkpoint_pos.y - 1000.f};
        ts.size = {10000.f, 200};
        auto &fall_trigger = m_world->createObject(ts);
        fall_trigger.m_collision_resolvers[ObjectType::Player] = [this](auto &player, auto &c_data)
        {
            m_playerx->m_vel *= 0.0f;
            m_playerx->setPosition(m_checkpoint_pos);
        };

        checkpoint_trigger.kill();

        js::startExam(m_exam_count); //! start practice exam
        m_exam_count++;
    };
}

void JumpGame::drawImpl(Renderer &win)
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

std::shared_ptr<GameLevelA> JumpGame::levelFactory(int level_id, utils::Vector2f start_pos, int direction)
{
    auto level = std::make_shared<GameLevelA>(*m_world, level_id, messanger);

    float level_height = 150.f;
    float fall_speed = 10.f;

    int platform_cells_count = 6;
    float total_width = 900.f;
    float cell_width = 80.f; // total_width / platform_cells_count;

    auto gen_platform_spec = [&](int level, int platform_cell_id) -> Wall::Spec
    {
        Wall::Spec spec;

        float width = cell_width / 1.5f + randf(0, cell_width / 4.f);
        spec.size = {width, 20.f};

        float height = level * level_height + randf(0.f, level_height / 10.f);
        float center = (platform_cell_id + 0.5f) * cell_width + randf(-cell_width / 6.f, cell_width / 6.f);
        spec.pos = {center, 0.f};
        spec.pos = start_pos + utils::Vector2f{direction * spec.pos.x, spec.pos.y};
        return spec;
    };

    auto gen_word = [&](bool is_correct, int cell_id, int height_id, float y_disp, float x_disp = 0.f)
    {
        utils::Vector2f pos = {start_pos.x + direction * (cell_id + 0.5f) * cell_width, start_pos.y + y_disp};
        utils::Vector2f size = {cell_width, 35.f};
        auto &word = generateWord(is_correct, pos, size, *m_exergenerator);
    };

    int level_cell_count = 10;
    Wall::Spec s;

    int id = 0;
    while (id < level_cell_count)
    {
        auto &platform = m_factories.at(ObjectType::Wall)->create(gen_platform_spec(2 * level_id - 1, id));
        ;

        id++;
        int x = 0;
        int words_between = randi(10, 15);
        float prev_height = (2 * level_id - 1) * level_height;
        while (x < words_between)
        {
            gen_word(true, id + x, 2 * level_id - 1, 0);
            x += 1;
            int dice = randi(0, 3);
            if (dice > 1)
            {
                for (int i = 0; i < dice; ++i)
                {
                    float y_disp = randf(50., 80.f);
                    bool kek = randi(0, 1) == 0;
                    gen_word(kek, id + x + i, 2 * level_id - 1, y_disp);
                    gen_word(!kek, id + x + i, 2 * level_id - 1, randf(-50.f, -70.f));
                }
                x += dice;
            }
            else if (dice == 2)
            {
                bool kek = randi(0, 1) == 0;
                bool kek2 = randi(0, 1) == 0;
                gen_word(kek, id + x, 2 * level_id - 1, randf(100.f, 150.f));
                // gen_word(kek2, id + x + 1, 2 * level-1, randf(-20.f, 20.f));
                gen_word(!kek, id + x, 2 * level_id - 1, randf(-90.f, -100.f));
                x += 2;
            }
            else
            {
                int wrong_count = randi(0, 2);
                for (int i = 0; i < wrong_count; ++i)
                {
                    gen_word(false, id + x + i, 2 * level_id - 1, 0.f);
                }
                x += wrong_count;
            }
        }

        id += x;
    }

    utils::Vector2f last_platform_pos = {id * cell_width, (2 * level_id - 1) * level_height};
    auto &platform = m_factories.at(ObjectType::Wall)->create(gen_platform_spec(2 * level_id - 1, id));
    Wall::Spec spec;
    m_factories.at(ObjectType::Wall)->create(spec);
    spec.pos = {last_platform_pos.x + 100.f, last_platform_pos.y + level_height};
    spec.size = {20.f, 2 * level_height};
    m_factories.at(ObjectType::Wall)->create(spec);

    createCheckpoint({platform.getPosition().x, platform.getPosition().y + 50.f}, {50, 50});

    auto &trigger = m_world->addObject3(ObjectType::Trigger);
    trigger.setSize({platform.getSize().x, 500.f});
    trigger.setPosition({platform.getPosition().x, platform.getPosition().y + trigger.getSize().y / 2.f});
    trigger.m_collision_resolvers[ObjectType::Player] = [this, &trigger, &platform, direction](auto &player,
                                                                                               auto &c_data)
    {
        auto &current_lvl = m_levels.front();
        current_lvl->m_on_stage_end(*current_lvl);

        messanger.send(LevelCompletedEvent{.stage = GameStage::Dodge,
                                           .level = current_lvl->m_id,
                                           .time = current_lvl->getDuration()});

        const auto &last_lvl = m_levels.back();

        m_levels.push_back(levelFactory(last_lvl->m_id + 1, {trigger.getPosition().x, trigger.getPosition().y}, 1)); //! create next lvl
        // m_levels.push_back(levelFactory(last_lvl.m_id + 1, {trigger.getPosition().x, trigger.getPosition().y}, -direction)); //! create next lvl
        trigger.kill();
    };

    CollisionComponent c_comp;
    c_comp.shape.convex_shapes.emplace_back(4);
    c_comp.type = ObjectType::Trigger;
    m_world->m_systems.addEntityDelayed(trigger.getId(), c_comp);

    level->m_on_stage_end = [this](auto &l)
    {
        l.killEntities();
    };
    return level;
}