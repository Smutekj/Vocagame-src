#include "DodgeGame.h"

#include "../Utils/RandomTools.h"
#include "../Systems/SpriteSystem.h"
#include "../Systems/PathSystem.h"
#include "../Systems/TimedEventSystem.h"
#include "../Animation.h"

#include <filesystem>

DodgeGame::DodgeGame(Renderer &window, KeyBindings &bindings, Assets &assets)
    : Game(window, bindings, assets)
{
    registerCollisions();
    registerSystems();
    
    //! PLAYER NEEDS TO BE FIRST BECAUSE OTHER OBJECTS MIGHT REFERENCE IT!
    auto p = m_world->insertObject([this](int id)
                                      { return std::make_shared<PlayeEntity>(m_world.get(), id); });
    m_playerx = std::static_pointer_cast<Player>(p).get();
    m_playerx->setPosition({300, 300});
    m_playerx->setDestructionCallback(
        [this](int id, ObjectType type) {});

    m_camera.threshold_rect = {0.5f, 0.40f, 0.0f, 0.2f};
    m_camera.m_can_move_y = true;
    m_camera.setFolowee(m_playerx);

    m_levels.emplace_back(levelFactory(0, m_playerx->getPosition()));

    m_guess_listener = std::make_unique<PostBox<WordGuessedEvent>>(messanger, [this](const auto &events)
                                                                   {
        for(const WordGuessedEvent& e : events)
        {
            m_guessed_correct_count += e.was_correct;
            // m_guessed_wrong_count += !e.was_correct;
            if(e.was_correct)
            {
                scaleAnimation(0.5, 0., 0.15, m_correct_scale, m_timers);
            }else{
                scaleAnimation(0.5, 0., 0.15, m_lives_scale, m_timers);
            }
        } });

    m_on_text_create = std::make_unique<PostBox<NewEntity<TextBubble>>>(messanger, [this](const auto &events)
                                                                        {
        for(const NewEntity<TextBubble>& e : events)
        {
            auto random_word_repre = m_generators.at(m_topics.at(0)).getNext();
            bool is_correct = utils::randi(0,1);
            if (is_correct)
            {
                e.p_entity->setText(random_word_repre.correct_form);
                m_correct_count++;
            }
            else
            {
                int n_forms = m_group_repres.at(random_word_repre.group).size();
                auto text = utils::randomValue(m_group_repres.at(random_word_repre.group)).correct_form;
                e.p_entity->setText(text);
                e.p_entity->setCorrectForm(random_word_repre.correct_form);
            }
            e.p_entity->setTranslation(random_word_repre.translation);
        } });
}

void DodgeGame::initializeUI()
{
}

void DodgeGame::restartGame()
{
}

void DodgeGame::registerCollisions()
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

void DodgeGame::registerSystems()
{
    auto &systems = m_world->m_systems;

    systems.registerSystem(std::make_shared<TimedEventSystem>(systems.getComponents<TimedEventComponent>()));
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

std::shared_ptr<GameLevelA> DodgeGame::levelFactory(int level_id, utils::Vector2f start_pos)
{
    auto level = std::make_shared<GameLevelA>(*m_world, level_id, messanger);

    std::filesystem::path cave_json_path = std::string{RESOURCES_DIR} + "Levels/wallShapes.json";
    std::unordered_map<std::string, CaveSpec2> caves = readCaveSpecs3(cave_json_path);
    const auto &level_data = caves.at("Level1");
    float level_width = level_data.level_size.x;
    float level_height = level_data.level_size.y;
    ;
    auto spawn_cave = [this, level_width, level_height](const CaveSpec2 &cave)
    {
        for (auto spec : cave.objects)
        {
            m_factories.at(spec->obj_type)->create(*spec);
        }
    };

    // m_timers.addInfiniteEvent(spawn_event, 5.f, 0.f);
    spawn_cave(level_data);

    m_camera.setPostition({m_playerx->getPosition().x, level_height / 2.f});
    float window_aspect = m_window.getTarget().getAspect();
    float camera_y_size = level_height + 20.f;
    m_camera.setSize({camera_y_size / window_aspect, camera_y_size});
    m_camera.m_can_move_y = false;

    float wall_speed = 0.f;
    //! left wall
    ElectroWall::Spec e_spec;
    e_spec.vel = {0.f, 0};
    e_spec.off_duration = 0.f;
    e_spec.angle = 90;
    e_spec.size = {level_height, 30.f};
    e_spec.pos = {-10.f, level_height / 2.f};
    m_factories.at(ObjectType::ElectroWall)->create(e_spec);
    //! right wall
    e_spec.pos.x += level_width;
    m_factories.at(ObjectType::ElectroWall)->create(e_spec);

    Wall::Spec w_spec;

    float platform_width = 100.f;

    w_spec.pos = {level_width / 2.f, level_height + 10.f};
    w_spec.size = {level_width, 20.f};
    m_factories.at(ObjectType::Wall)->create(w_spec);
    w_spec.pos = {level_width / 2.f, -10.f};
    w_spec.size = {level_width, 20.f};
    m_factories.at(ObjectType::Wall)->create(w_spec);

    m_camera.setSize({level_height / m_window.getTarget().getAspect(), level_height + 40.});
    std::cout << "Camera view: " << m_camera.getView().getSize().y << std::endl;
    m_camera.setPostition({300, level_height / 2.f});

    std::vector<std::string> cave_keys; //! for randomly generating caves;
    //! normalize posiitons of specs
    for (auto &[key, cave] : caves)
    {
        cave_keys.push_back(key);
        // auto bb = cave.bounds;
        for (auto spec : cave.objects)
        {
            // spec->pos -= Vec2{bb.pos_x, bb.pos_y};
            // spec->pos.x += 0 * level_width;
        }
    }

    auto &stuff_killer = m_world->addObject3(ObjectType::Trigger);
    stuff_killer.setSize({10.f, level_height});
    stuff_killer.m_vel = {wall_speed, 0.f};
    stuff_killer.setPosition({-5.f, level_height / 2.f});
    stuff_killer.m_collision_resolvers[ObjectType::TextBubble] = [this](auto &word, auto &c_data)
    {
        word.kill();
    };
    stuff_killer.m_collision_resolvers[ObjectType::Wall] = [this](auto &word, auto &c_data)
    {
        word.kill();
    };
    CollisionComponent c_comp;
    c_comp.shape.convex_shapes.emplace_back(4);
    c_comp.type = ObjectType::Trigger;
    m_world->m_systems.addEntityDelayed(stuff_killer.getId(), c_comp);

    return level;
}

void DodgeGame::drawImpl(Renderer &win)
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
    // ui_background.setPosition(m_level_completed_text_pos);
    // auto bb = m_level_completed_text.getBoundingBox();
    // ui_background.setScale(bb.width / 2.f + 30, m_ui_header_height / 2. * 1.5);
    // win.drawSprite(ui_background);
    win.drawAll();

    Text player_hp;
    player_hp.setText("Lives: " + std::to_string((int)m_playerx->health));
    player_hp.scale(m_lives_scale, m_lives_scale);
    player_hp.setFont(m_font.get());
    auto bb = player_hp.getBoundingBox();
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

    // m_level_completed_text.centerAround(m_level_completed_text_pos);
    // win.drawText(m_level_completed_text);
    // Text timer;
    win.drawAll();
    word_counts.setScale(1, 1);

    win.m_view = old_view;
}