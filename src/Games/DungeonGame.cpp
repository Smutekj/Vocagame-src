#include "DungeonGame.h"

#include "Utils/RandomTools.h"
#include "Utils/IOUtils.h"

#include "Entities/Box.h"

#include "Systems/SpriteSystem.h"
#include "Systems/TimedEventSystem.h"
#include "JSBindings.h"
#include <algorithm>

DungeonGame::DungeonGame(Renderer &window, KeyBindings &bindings, Assets &assets)
    : Game(window, bindings, assets),
      m_pos_generator(messanger, {m_font->getLineHeight() * 2, m_font->getLineHeight() * 2.f}, {800.f, 800.f}),
      m_cdt({1200, 1200})
{

    Vec2 intro_text_pos = {m_window.getTargetSize().x / 4.f, m_window.getTargetSize().y * 19. / 20.};
    m_ui_definition.setFont(assets.fonts.at("Tahoma").get());
    m_ui_definition.setPageWidth(m_window.getTargetSize().y / 2.f);
    m_ui_definition.setPosition(intro_text_pos);

    m_box_touched_listener = std::make_unique<PostBox<PlayerTouchedBox>>(messanger, [this](const auto &events) {
        for(const auto& e : events)
        {
            m_ui_definition.setText(e.definition);
        }
    });

    std::string json_path = std::string{RESOURCES_DIR} + "TestWords.json";
    auto words_data = utils::loadJson(json_path.c_str());
    for (auto &words_data : words_data.at("words"))
    {
        DefinedWord new_word = {.word = words_data.at("text"), .definition = words_data.at("definition")};
        m_words.push_back(new_word);
    }

    Box::Spec bs;
    bs.pos = {500, 500};
    bs.size = {50, 50};
    bs.text = m_words.at(0).word;
    bs.definition = m_words.at(0).definition;
    auto& box = m_world->createObject(bs);


    VisualObject::Spec vs;
    vs.pos = {700, 400};
    vs.size = {100, 100};
    vs.sprite.texture_id = "Star";
    auto& star = m_world->createObject(vs);
    
    Trigger::Spec ts;
    ts.pos = {700, 400};
    ts.size = {50, 50};
    auto& trigger = m_world->createObject(ts);
    trigger.m_collision_resolvers[ObjectType::Box] = [&star, &box](GameObject& obj, CollisionData& c_data){
        if(box.getId() == obj.getId() && utils::dist(obj.getPosition(), box.getPosition()) < 10.f)
        {
            box.kill();
            star.kill();
        }
    };
    
    registerSystems();
    initUI();

    m_background_tex = m_textures.get("PaperVintage");
    m_background.setTexture(*m_background_tex);

    std::filesystem::path levels_file_path = std::string{RESOURCES_DIR} + "Levels/SnakeLevels.json";
    m_levels = readCaveSpecs3(levels_file_path);
    startLevel("Level1");

    //! add light layer
    TextureOptions tex_options;
    tex_options.min_param = TexMappingParam::Linear;
    int width = m_window.getTargetSize().x;
    int height = m_window.getTargetSize().y;
    auto &light_layer = m_layers.addLayer("Light", 10, tex_options, width, height);
    light_layer.m_canvas.addShader("Light", "basictex.vert", "Light.frag");
    light_layer.m_canvas.addShader("VisionLight", "basictex.vert", "visionLight.frag");
    light_layer.m_canvas.setShadersPath(std::string{RESOURCES_DIR} + "Shaders/");
    light_layer.m_canvas.m_blend_factors = {BlendFactor::One, BlendFactor::One, BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha};
    light_layer.addEffect(std::make_unique<LightCombine>(width, height));
    light_layer.setBackground({0, 0, 0, 1});
}

void DungeonGame::initUI()
{
    m_player_hp_text.setFont(m_font.get());
    m_player_hp_text.setColor({0, 0, 0, 255});
    m_player_hp_text.m_edge_color = {255, 0, 0, 255};
    m_player_hp_text.m_glow_color = {255, 0, 0, 255};

    m_word_counts_text.setFont(m_font.get());
    m_word_counts_text.setColor({0, 0, 0, 255});
    m_word_counts_text.m_edge_color = {0, 255, 0, 255};
    m_word_counts_text.m_glow_color = {0, 255, 0, 255};
    m_guess_listener = std::make_unique<PostBox<WordGuessedEvent>>(messanger, [this](const auto &events)
                                                                   {
        for(const WordGuessedEvent& e : events)
        {
            m_guessed_correct_count += e.was_correct;
            m_guessed_wrong_count += !e.was_correct;
            impulseAnimation(0.5, 0.15, e.was_correct ? m_word_counts_text : m_player_hp_text, m_timers);
            if(!e.was_correct)
            {
                m_lives_count--;
                if(m_lives_count == 0)
                {
                    m_lives_count = 3;
                    js::startExam(m_exam_count);
                    m_exam_count++;
                }
            }
        } });

    m_level_completed_text.m_is_centered = true;
    m_level_completed_text.setFont(m_font.get());
    m_level_completed_text.setScale(1.5f, 1.5f);
    m_level_completed_text.setPosition(-500.f, -500.f);
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
        } });

    m_buttons.at("leftButton")->setOnClick([this]()
                                           { m_snake->onKeyPress(SDLK_a); });
    m_buttons.at("leftButton")->setOnRelease([this]()
                                             { m_snake->onKeyRelease(SDLK_a); });
    m_buttons.at("rightButton")->setOnClick([this]()
                                            { m_snake->onKeyPress(SDLK_d); });
    m_buttons.at("rightButton")->setOnRelease([this]()
                                              { m_snake->onKeyRelease(SDLK_d); });
}
void DungeonGame::addVertsToTriangulation(utils::Vector2f pos, utils::Vector2f size)
{
    //! it is a rectangle
    cdt::Vector2i ll = Vec2{pos.x - size.x / 2.f, pos.y - size.y / 2.f};
    cdt::Vector2i lr = Vec2{pos.x + size.x / 2.f, pos.y - size.y / 2.f};
    cdt::Vector2i ur = Vec2{pos.x + size.x / 2.f, pos.y + size.y / 2.f};
    cdt::Vector2i ul = Vec2{pos.x - size.x / 2.f, pos.y + size.y / 2.f};

    int n_verts = m_cdt.m_vertices.size();
    auto vll = m_cdt.insertVertexAndGetData(ll, false);
    auto ill = vll.overlapping_vertex == -1 ? n_verts : vll.overlapping_vertex;

    n_verts = m_cdt.m_vertices.size();
    auto vlr = m_cdt.insertVertexAndGetData(lr, true);
    auto ilr = vlr.overlapping_vertex == -1 ? n_verts : vlr.overlapping_vertex;

    n_verts = m_cdt.m_vertices.size();
    auto vur = m_cdt.insertVertexAndGetData(ur, true);
    auto iur = vur.overlapping_vertex == -1 ? n_verts : vur.overlapping_vertex;

    n_verts = m_cdt.m_vertices.size();
    auto vul = m_cdt.insertVertexAndGetData(ul, true);
    auto iul = vul.overlapping_vertex == -1 ? n_verts : vul.overlapping_vertex;

    m_cdt.insertConstraint(cdt::EdgeVInd{ill, ilr});
    m_cdt.insertConstraint(cdt::EdgeVInd{ilr, iur}); 
    m_cdt.insertConstraint(cdt::EdgeVInd{iur, iul});
    m_cdt.insertConstraint(cdt::EdgeVInd{iul, ill});
}

void DungeonGame::startLevel(std::string id)
{
    if (!m_levels.contains(id))
    {
        return;
    }

    auto &lvl = m_levels.at(id);
    Vec2 ll_bound = {lvl.origin};
    level_size = lvl.level_size;
    m_pos_generator.setBoxSize(level_size);

    //! THE GREATEST WALL IN THE WORLD!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    Wall::Spec wall_s;
    wall_s.obj_type = TypeId::Wall;
    float wall_thickness = 60.;
    wall_s.size = {level_size.x + wall_thickness * 2, wall_thickness};
    wall_s.pos = ll_bound + Vec2{level_size.x / 2.f, -wall_thickness / 2.f};
    m_world->createObject(wall_s);
    wall_s.pos.y = ll_bound.y + level_size.y + wall_thickness / 2.f;
    m_world->createObject(wall_s);
    wall_s.size = ll_bound + Vec2{wall_thickness, level_size.y + wall_thickness * 2.f};
    wall_s.pos = {-wall_thickness / 2.f, level_size.y / 2.f};
    m_world->createObject(wall_s);
    wall_s.pos.x = ll_bound.x + level_size.x + wall_thickness / 2.f;
    m_world->createObject(wall_s);

    for (auto &obj : lvl.objects)
    {
        auto &new_obj = m_world->createObject(*obj);
        m_pos_generator.removeCoveredCells(obj->pos, new_obj.getSize());
        addVertsToTriangulation(new_obj.getPosition(), new_obj.getSize());
    }

    m_snake = std::static_pointer_cast<DungeonPlayer>(m_world->insertObject([&, this](int ent_id)
                                                                            {
        DungeonPlayer::Spec spec;
        spec.pos = lvl.start;
        spec.size= {40, 40};
        spec.cdt = &m_cdt;
        return std::make_shared<DungeonPlayer>(*m_world, spec, ent_id); }));

    m_camera.setPostition({level_size.x / 2.f, level_size.x / 2.f});
    float screen_aspect = m_window.getTarget().getAspect();
    if (screen_aspect < 1.)
    {
        m_camera.setSize({m_camera_size / screen_aspect, m_camera_size});
    }
    else
    {
        m_camera.setSize({m_camera_size, m_camera_size * screen_aspect});
    }
    m_camera.setFolowee(nullptr);
    m_camera.m_bounded = true;
    m_camera.m_world_bounds = {-wall_thickness, -wall_thickness, level_size.x + 2 * wall_thickness, level_size.y + 2 * wall_thickness};
    m_camera.m_can_move_x = true;
    m_camera.m_can_move_y = true;
    m_camera.setFolowee(m_snake.get());
    m_camera.threshold_rect = {0.35, 0.35, 0.3, 0.3};
}

void DungeonGame::registerSystems()
{
    auto &colllider = m_world->getCollisionSystem();
    colllider.registerResolver(ObjectType::Bullet, ObjectType::TextBubble);
    colllider.registerResolver(ObjectType::Snake, ObjectType::TextBubble);
    colllider.registerResolver(ObjectType::Snake, ObjectType::Wall);
    colllider.registerResolver(ObjectType::Wall, ObjectType::Player);
    colllider.registerResolver(ObjectType::Trigger, ObjectType::Box);
    colllider.registerResolver(ObjectType::Box, ObjectType::Player);
    colllider.registerResolver(ObjectType::Box, ObjectType::Wall);

    auto &systems = m_world->m_systems;
    systems.registerSystem(std::make_shared<TransformSystem>(systems.getComponents<TransformComponent>()));
    systems.registerSystem(std::make_shared<TimedEventSystem>(systems.getComponents<TimedEventComponent>()));
    systems.registerSystem(std::make_shared<SpriteSystem>(systems.getComponents<SpriteComponent>(), m_layers));

    std::filesystem::path animation_directory = {
        std::string{RESOURCES_DIR} + "Textures/Animations/"};
    auto animation_system = std::make_shared<AnimationSystem>(
        systems.getComponents<AnimationComponent>(),
        animation_directory, animation_directory);

    animation_system->registerAnimation(m_atlases.get("PurpleExplosion"), "PurpleExplosion");
    animation_system->registerAnimation(m_atlases.get("BlueExplosion"), "BlueExplosion");

    systems.registerSystem(animation_system);
}

void DungeonGame::updateImpl([[maybe_unused]] float dt)
{
    auto tex_size = m_background_tex->getSize();
    auto cam_size = m_camera.getView().getSize();
    auto cam_center = m_camera.getView().getCenter();
    auto cam_ll = cam_center - cam_size / 2.f;
    Recti background_tex_rect = {(int)(cam_ll.x / level_size.x * tex_size.x),
                                 (int)(cam_ll.y / level_size.y * tex_size.y),
                                 (int)(cam_size.x / level_size.x * tex_size.x),
                                 (int)(cam_size.y / level_size.y * tex_size.y)};
    // background_tex_rect.pos_y = tex_size.y - background_tex_rect.pos_y;
    m_background.m_tex_rect = background_tex_rect;
}

void DungeonGame::handleEventImpl(const SDL_Event &event)
{
    if (event.type == SDL_KEYDOWN)
    {
        auto key = event.key.keysym.sym;
        m_snake->onKeyPress(key);
    }
    else if (event.type == SDL_KEYUP)
    {
        auto key = event.key.keysym.sym;
        m_snake->onKeyRelease(key);
    }

    if (event.type == SDL_FINGERDOWN)
    {
        auto window_size = m_window.getTargetSize();
        auto touch_pos = utils::Vector2f{event.tfinger.x, 1. - event.tfinger.y};
        touch_pos = {touch_pos.x * window_size.x, touch_pos.y * window_size.y};

        bool no_button_pushed = std::none_of(m_buttons.begin(), m_buttons.end(), [touch_pos](auto &id_button)
                                             { return id_button.second->getBoundingBox().contains(touch_pos); });
        if (no_button_pushed)
        {
        }
    }
    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
    {
        float screen_aspect = m_window.getTarget().getAspect();
        if (screen_aspect < 1.)
        {
            m_camera.setSize({m_camera_size / screen_aspect, m_camera_size});
        }
        else
        {
            m_camera.setSize({m_camera_size, m_camera_size * screen_aspect});
        }
    }
}

void DungeonGame::drawImpl(Renderer &win)
{
    auto old_view = win.m_view;
    win.m_view = win.getDefaultView();
    utils::Vector2f win_size = win.getTargetSize();

    //! draw Definition text
    m_ui_definition.setPageWidth(win_size.x / 2.f);
    utils::Vector2f page_size = {m_ui_definition.getPageWidth(), m_ui_definition.getPageHeight()};
    // utils::Vector2f page_center = m_ui_definition.getPosition() + Vec2{page_size.x, -page_size.y} / 2.f;
    m_ui_definition.drawInto(win);
    // intro.setScale(0.8f);

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

    m_player_hp_text.setText("Lives: " + std::to_string(m_lives_count));
    bb = m_player_hp_text.getBoundingBox();
    m_player_hp_text.centerAround({win_size.x * 1.f / 4.f, win_size.y - m_ui_header_height / 2.f});
    m_window.drawText2(m_player_hp_text);

    m_word_counts_text.setText("Max Length: " + std::to_string(m_max_length));
    bb = m_word_counts_text.getBoundingBox();
    m_word_counts_text.centerAround({win_size.x - win_size.x / 4., win_size.y - m_ui_header_height / 2.f});
    win.drawText2(m_word_counts_text);

    win.drawText(m_level_completed_text);
    // Text timer;
    win.drawAll();
    m_word_counts_text.setScale(1, 1);

    win.m_view = old_view;
}
