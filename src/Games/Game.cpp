#include "Game.h"

#include "Entities/Player.h"
#include "Utils/RandomTools.h"
#include "Utils/Time.h"

#include "Systems/TimedEventSystem.h"
#include "Systems/SpriteSystem.h"

#include <imgui_impl_sdl2.h>

#include "SoundSystem.h"

#include "Utils/IOUtils.h"

Camera *p_camera = nullptr;
Game *p_game = nullptr;
//! export to js
extern "C"
{
    void restartGame()
    {
        if (p_game)
        {
            std::cout << "RESTARTED GAME FROM BROWSER!" << std::endl;
            p_game->restartGame();
        }
    }

    void zoomCamera(float factor)
    {
        if (p_camera)
        {
            p_camera->zoom(factor);
        }
    }
}

inline std::vector<std::string> loadTopics()
{
    std::string topics = js::loadFromStorage("topics");
    if (topics.empty())
    {
        topics = "Werkzeug";
    }

    std::cout << "Topics: " << topics << std::endl;
    std::vector<std::string> split_topics;
    //! split string
    std::size_t start = 0;
    std::size_t end = 0;
    while (end != topics.npos)
    {
        end = topics.find_first_of(',', start);
        split_topics.push_back(topics.substr(start, end));
        start = end + 1;
    }
    return split_topics;
}

void Game::initializeLayersAndTextures()
{
    auto width = m_window.getTargetSize().x;
    auto height = m_window.getTargetSize().y;

    TextureOptions options;
    options.wrap_x = TexWrapParam::ClampEdge;
    options.wrap_y = TexWrapParam::ClampEdge;
    options.mag_param = TexMappingParam::Linear;
    options.min_param = TexMappingParam::Linear;

    TextureOptions text_options;
    text_options.data_type = TextureDataTypes::UByte;
    text_options.format = TextureFormat::RGBA;
    text_options.internal_format = TextureFormat::RGBA;
    text_options.mag_param = TexMappingParam::Linear;
    text_options.min_param = TexMappingParam::Linear;

    std::filesystem::path shaders_directory = std::string{RESOURCES_DIR} + "Shaders/";

    std::cout << "Unit layer dimensions: " << width << " : " << height << std::endl;
    auto &unit_layer = m_layers.addLayer("Unit", 3, text_options, width, height);
    unit_layer.m_canvas.setShadersPath(shaders_directory);
    unit_layer.m_canvas.addShader("ElectroWall", "basicinstanced.vert", "ElectroWall.frag");
    unit_layer.m_canvas.addShader("lightningBolt", "basicinstanced.vert", "lightningBolt.frag");
    unit_layer.m_canvas.addShader("flag", "basicinstanced.vert", "flag.frag");
    unit_layer.m_canvas.addShader("Meteor", "basictex.vert", "Meteor.frag");

    auto &bg_layer = m_layers.addLayer("Background", 1, text_options, width, height);
    bg_layer.m_canvas.setShadersPath(shaders_directory);
    auto &text_layer = m_layers.addLayer("Text", 7, text_options, width, height);
    text_layer.m_canvas.setShadersPath(shaders_directory);

    double dpr = 1.; // std::max(1., js::getDpr());
    std::cout << "DPR IS: " << dpr << std::endl;
    auto &bloom_layer = m_layers.addLayer("Bloom", 5, options, width / dpr, height / dpr, dpr);
    bloom_layer.m_canvas.setShadersPath(shaders_directory);
    bloom_layer.m_canvas.addShader("ElectroWall", "basicinstanced.vert", "ElectroWall.frag");
    bloom_layer.m_canvas.addShader("gradientX", "basictex.vert", "gradientX.frag");
    bloom_layer.m_canvas.addShader("gradientY", "basictex.vert", "gradientY.frag");
    bloom_layer.m_canvas.addShader("eye", "basictex.vert", "eye.frag");
    bloom_layer.addEffect(std::make_unique<BloomPhysical>(width / dpr, height / dpr, 2, 2, options));

    m_window.setShadersPath(shaders_directory);
    m_window.addShader("Shiny", "basicinstanced.vert", "shiny.frag");
    m_window.addShader("Arrow", "basicinstanced.vert", "texture.frag");
}

using namespace utils;

constexpr utils::Vector2f PLAYER_START_POS = {100, 20};
constexpr float START_VIEW_SIZE = 400.f;

// #include "piper.h"
// const char *ng_data_path = "/home/smutekj/projects/languagePlatformer/piper/libpiper/install/espeak-ng-data/";
// void write_wav(std::ofstream &file,
//                const float *samples,
//                size_t num_samples,
//                int sample_rate = 22050,
//                int channels = 1)
// {
//     int byte_rate = sample_rate * channels * 2; // 16-bit
//     int block_align = channels * 2;
//     int data_size = num_samples * 2;

//     // RIFF header
//     file.write("RIFF", 4);
//     int chunk_size = 36 + data_size;
//     file.write(reinterpret_cast<const char *>(&chunk_size), 4);
//     file.write("WAVE", 4);

//     // fmt subchunk
//     file.write("fmt ", 4);
//     int subchunk1_size = 16;
//     file.write(reinterpret_cast<const char *>(&subchunk1_size), 4);
//     short audio_format = 1; // PCM
//     file.write(reinterpret_cast<const char *>(&audio_format), 2);
//     file.write(reinterpret_cast<const char *>(&channels), 2);
//     file.write(reinterpret_cast<const char *>(&sample_rate), 4);
//     file.write(reinterpret_cast<const char *>(&byte_rate), 4);
//     file.write(reinterpret_cast<const char *>(&block_align), 2);
//     short bits_per_sample = 16;
//     file.write(reinterpret_cast<const char *>(&bits_per_sample), 2);

//     // data subchunk
//     file.write("data", 4);
//     file.write(reinterpret_cast<const char *>(&data_size), 4);

//     // Convert float samples [-1, 1] → int16 PCM
//     for (size_t i = 0; i < num_samples; i++)
//     {
//         float s = std::clamp(samples[i], -1.0f, 1.0f);
//         int16_t val = static_cast<int16_t>(s * 32767.0f);
//         file.write(reinterpret_cast<const char *>(&val), sizeof(val));
//     }
// }

std::vector<WordRepresentation> Game::generateRepre(std::vector<std::string> word_groups,
                                                    std::string excercise_type)
{
    std::vector<WordRepresentation> repres;
    repres.reserve(1000);

    using json = nlohmann::json;

    for (const auto &word_group : word_groups)
    {
        std::cout << "Loading topic: " << word_group << std::endl;

        std::filesystem::path word_file;
#if defined(__EMSCRIPTEN__)
        word_file = "/execDb/" + word_group + ".json";
#else
        word_file = std::string{RESOURCES_DIR} + "execDb/" + word_group + ".json";
#endif
        auto exec_data = utils::loadJson(word_file.c_str());

        std::size_t group_size = 0;
        for (auto [index, item] : exec_data.at(word_group).items())
        {
            group_size++;
            WordRepresentation repre;
            repre.group = word_group;
            repre.type = "Noun";
            repre.meaning_id = item.at("meaning_id");
            repre.translation = item.at("translation");
            repre.correct_form = item.at("correct_word");
            repres.push_back(std::move(repre));
            if (m_assets.atlases.contains(repre.meaning_id))
            {
                repre.image_id = repre.meaning_id;
            }
        }

        if (excercise_type == "translation")
        {
            int wrong_count = 1;
            for (auto &repre : repres)
            {
                for (int i = 0; i < wrong_count; ++i)
                {
                    std::string wrong_translation = randomValue(repres.end() - group_size, repres.end()).correct_form;
                    while ((wrong_translation == repre.correct_form))
                    {
                        wrong_translation = randomValue(repres.end() - group_size, repres.end()).correct_form;
                    }
                    repre.shown_forms.push_back(wrong_translation);
                }
            }
        }
        if (excercise_type == "article")
        {
            for (auto &repre : repres)
            {
                auto article_pos = repre.correct_form.find(' ') + 1;
                auto correct_article = repre.correct_form.substr(0, article_pos);
                auto word_len = repre.correct_form.length();
                std::string word_without_article = repre.correct_form.substr(article_pos, word_len - article_pos);
                for (std::string article : {"der ", "die ", "das "})
                {
                    if (article == correct_article)
                    {
                        continue;
                    }
                    repre.shown_forms.push_back(article + word_without_article);
                }
            }
        }
        m_group_repres[word_group] = {repres.end() - group_size, repres.end()};
        m_generators[word_group] = WordGenerator{};
        m_generators.at(word_group).setRepre(m_group_repres.at(word_group));
        std::cout << "Loaded: " << repres.size() << " words" << std::endl;
    }

    return repres;
}

void Game::restartGame()
{
    auto new_topics = loadTopics();
    if (new_topics != m_topics)
    {
        m_topics = new_topics;
        auto tic = std::chrono::high_resolution_clock::now();
        m_repres = generateRepre(m_topics, "translation");
        auto toc = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(toc - tic).count() << std::endl;
    }
    //! set languages
    try
    {
        auto languages_json = utils::loadJson("/execDb/SelectedLanguages.json");
        m_studied_language = languages_json.value("studied", "de");
    }
    catch (std::exception &e)
    {
        std::cout << "ERROR loading selected languages! " << e.what() << std::endl;
        m_studied_language = "de";
    }

    for (auto &lvl : m_levels)
    {
        lvl->killEntities();
    }
    m_levels.clear();
}

// bool touch_cb(int event_type, const EmscriptenTouchEvent* touch_event, void* data)
// {
//     Player* p_player = static_cast<Player*>(data);
//     p_player->onJump();
//     std::cout << "Registered touch event" << std::endl;
// }

void Game::repositionUI()
{
    auto window_size = m_window.getTargetSize();
    float button_size = window_size.y / 8.f;
    float button_offset = window_size.x / 10.f;
    float button_space = button_size / 1.5f;
    float l_pos_x = button_offset;
    float l_pos_y = window_size.y * 1.f / 8.f;
    float r_pos_x = l_pos_x + button_size + button_space;
    float r_pos_y = l_pos_y;
    utils::Vector2f exit_pos = {window_size.x * 7.f / 8.f, window_size.y * 7. / 8.};
    auto left_button = m_buttons.at("leftButton");
    auto right_button = m_buttons.at("rightButton");
    auto exit_button = m_buttons.at("exitButton");
    left_button->setScale(utils::Vector2f{button_size / 2.f});
    right_button->setScale(utils::Vector2f{button_size / 2.f});
    exit_button->setScale(utils::Vector2f{button_size / 6.f});
    left_button->setRotation(utils::radians(180.f));
    left_button->setPosition(l_pos_x, l_pos_y);
    right_button->setPosition(r_pos_x, r_pos_y);
    exit_button->setPosition(exit_pos);
}
void Game::initializeUI()
{
    m_buttons.clear();
    //! init UI
    auto window_size = m_window.getTargetSize();
    float button_size = window_size.y / 8.f;
    float button_offset = window_size.x / 10.f;
    float button_space = button_size / 1.5f;
    float l_pos_x = button_offset;
    float l_pos_y = window_size.y * 1.f / 8.f;
    float r_pos_x = l_pos_x + button_size + button_space;
    float r_pos_y = l_pos_y;
    utils::Vector2f exit_pos = {window_size.x * 7.f / 8.f, window_size.y * 7. / 8.};
    auto left_button = std::make_shared<IconButton>(*m_textures.get("Arrow"));
    auto right_button = std::make_shared<IconButton>(*m_textures.get("Arrow"));
    auto exit_button = std::make_shared<IconButton>(*m_textures.get("UIClose"));
    left_button->setScale(utils::Vector2f{button_size / 2.f});
    right_button->setScale(utils::Vector2f{button_size / 2.f});
    exit_button->setScale(utils::Vector2f{button_size / 6.f});
    left_button->setRotation(utils::radians(180.f));
    left_button->setPosition(l_pos_x, l_pos_y);
    right_button->setPosition(r_pos_x, r_pos_y);
    exit_button->setPosition(exit_pos);
    left_button->setOnClick([this]()
                            { m_playerx->onRunStart(-1); });
    left_button->setOnRelease([this]()
                              { m_playerx->onRunEnd(-1); });
    right_button->setOnClick([this]()
                             { m_playerx->onRunStart(1); });
    right_button->setOnRelease([this]()
                               { m_playerx->onRunEnd(1); });
    exit_button->setOnClick([]() {});
    exit_button->setOnRelease([this]()
                              { js::pauseGame("PAUSED"); });
    m_buttons["leftButton"] = left_button;
    m_buttons["rightButton"] = right_button;
    m_buttons["exitButton"] = exit_button;
}

Game::Game(Renderer &window, KeyBindings &bindings, Assets &assets)
    : m_window(window), m_key_binding(bindings),
      m_assets(assets),
      m_textures(assets.textures),
      m_font(assets.fonts.at("Tahoma")),
      m_atlases(assets.atlases),
      m_camera({PLAYER_START_POS.x, 0.f},
               {START_VIEW_SIZE, START_VIEW_SIZE * window.getTargetSize().y / window.getTargetSize().x},
               messanger)
{

    m_textures.get("SkyNight2")->setWrapX(TexWrapParam::Repeat);
    m_textures.get("BrickWall")->setWrapX(TexWrapParam::Repeat);
    m_textures.get("BrickWall")->setWrapY(TexWrapParam::Repeat);

    std::cout << "Creating game!" << std::endl;
#if defined(__EMSCRIPTEN__)
    p_camera = &m_camera;
    p_game = this;
    js::setupDeviceOrientation();
#endif
    //! seed
    srand(time(NULL));

    auto tic = std::chrono::high_resolution_clock::now();
    initializeLayersAndTextures();
    auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tic);
    std::cout << dt.count() << " shaders" << std::endl;

    tic = std::chrono::high_resolution_clock::now();
    m_world = std::make_unique<GameWorld>(messanger);
    dt = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - tic);
    std::cout << dt.count() << "world" << std::endl;

    // emscripten_set_touchstart_callback("#canvas", (void*)m_playerx, false, touch_cb);

    m_ui_system = std::make_unique<UISystem>(window, m_textures, messanger, m_playerx, *m_font,
                                             *m_world);

    createFactories<TYPE_LIST>();

    m_effects_factory = std::make_unique<EffectsFactory>(*m_world);

    m_objective_system = std::make_unique<ObjectiveSystem>(messanger);

    initializeUI();
    restartGame();
}

TextBubble &Game::generateWord(bool is_correct, utils::Vector2f pos, utils::Vector2f size)
{
    TextPlatform::Spec spec;
    auto random_word_repre = m_generators.at(m_topics.at(0)).getNext();
    if (is_correct)
    {
        spec.text = random_word_repre.correct_form;
    }
    else
    {
        spec.text = randomValue(m_group_repres.at(random_word_repre.group)).correct_form;
    }

    spec.pos = pos;
    spec.size = size;
    spec.meaning_id = random_word_repre.meaning_id;
    spec.translation = random_word_repre.translation;
    spec.correct_form = random_word_repre.correct_form;
    auto &word = static_cast<TextBubble &>(m_factories.at(ObjectType::TextPlatform)->create(spec));

    std::string correct_no_space = spec.meaning_id;
    ;
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

TextBubble &Game::generateWordGroup(bool is_correct, std::string word_group, utils::Vector2f pos, utils::Vector2f size)
{
    WordSpec spec;
    auto random_word_repre = randomValue(m_group_repres.at(word_group));
    if (is_correct)
    {
        spec.text = random_word_repre.correct_form;
    }
    else
    {
        int n_forms = random_word_repre.shown_forms.size();
        spec.text = random_word_repre.shown_forms.at(randi(0, n_forms - 1));
    }
    spec.type = {WordType::Platform};
    spec.pos = pos;
    spec.size = size;
    spec.translation = random_word_repre.translation;
    spec.correct_form = random_word_repre.correct_form;
    auto &word = static_cast<TextBubble &>(m_factories.at(ObjectType::TextBubble)->create(spec));

    word.m_collision_resolvers[ObjectType::Player] = [this, &word](GameObject &obj, CollisionData &c_data)
    {
        float fall_speed = 20.f;
        auto &t_word = static_cast<TextBubble &>(word);
        auto &hit_text = word.getText();

        auto surface_norm = c_data.separation_axis;
        auto rel_vel = obj.m_vel - word.m_vel;
        float v_in_norm = utils::dot(surface_norm, rel_vel);
        if (surface_norm.y > 0.f && v_in_norm <= 0.f)
        {
            obj.move(surface_norm * c_data.minimum_translation);
            obj.m_vel -= v_in_norm * surface_norm;
            if (word.firstTouch())
            {
                messanger.send<WordGuessedEvent>(
                    {.entity_id = word.getId(),
                     .translation = word.getTranslation(),
                     .shown_form = word.getText(),
                     .correct_form = word.getCorrectForm()});

                if (word.isCorrect()) //! if this is first time player touched the thing
                {
                    // auto &stars = m_world->addObject2<StarEmitter>();
                    // stars.setPosition(word.getPosition());
                }
                else
                {
                    SoundSystem::play("WrongWord");
                }
            }
            word.first_touch = false;
            word.m_player_is_standing = true;
            word.m_vel.y = word.isCorrect() ? 0.f : -fall_speed;
            obj.m_vel.y = word.isCorrect() ? obj.m_vel.y : word.m_vel.y;
        }
    };
    return word;
};
/* std::shared_ptr<GameLevelA> Game::shooterLevelFactory(int level_id, utils::Vector2f start_pos)
{
    auto level = std::make_shared<GameLevelA>(*m_world, level_id, messanger);

    WallSpec w_spec;
    w_spec.type = WallType::Static;

    float platform_width = 100.f;

    float level_height = 500.f;
    float level_width = 600.f;
    w_spec.pos = {-10.f, level_height / 2.f};
    w_spec.size = {20.f, level_height};
    m_factories.at(ObjectType::Wall)->create(w_spec);
    w_spec.pos = {level_width + 10.f, level_height / 2.f};
    w_spec.size = {20.f, level_height};
    m_factories.at(ObjectType::Wall)->create(w_spec);

    w_spec.pos = {200.f, 60.f};
    w_spec.size = {platform_width, 20.f};
    m_factories.at(ObjectType::Wall)->create(w_spec);

    w_spec.pos = {400.f, 60.f};
    m_factories.at(ObjectType::Wall)->create(w_spec);

    w_spec.pos = {300, 140.f};
    w_spec.size = {200.f, 20.f};
    m_factories.at(ObjectType::Wall)->create(w_spec);

    w_spec.pos = {300, 350.f};
    w_spec.size = {200.f, 20.f};
    m_factories.at(ObjectType::Wall)->create(w_spec);

    w_spec.pos = {150, 200.f};
    w_spec.size = {80.f, 20.f};
    w_spec.type = WallType::Moving;
    w_spec.path = {w_spec.pos, w_spec.pos + utils::Vector2f{0.f, 150.f}};
    m_factories.at(ObjectType::Wall)->create(w_spec);
    w_spec.pos = {550.f, 440.f};
    w_spec.path = {w_spec.pos, w_spec.pos - utils::Vector2f{0.f, 150.f}};
    m_factories.at(ObjectType::Wall)->create(w_spec);

    auto word_specifier = [this, level_width, level_height](WordSpec &spec, float t, int c)
    {
        int top = randi(0, 1);
        int left_right = randi(0, 1);
        if (top)
        {
            spec.pos = {randf(0, level_width), level_height};
            spec.vel = {randf(-20.f, 20.f), -20.f};
        }
        else
        {
            spec.pos.y = randf(60.f, level_height - 100.f);
            spec.pos.x = left_right * level_width;
            spec.vel = {(1 - 2 * left_right) * 20.f, randf(-20.f, 20.f)};
        }

        auto random_word_repre = randomValue(m_repres);
        if (randi(0, 1))
        {
            spec.text = random_word_repre.correct_form;
        }
        else
        {
            int n_forms = random_word_repre.shown_forms.size();
            spec.text = random_word_repre.shown_forms.at(randi(0, n_forms - 1));
        }
        spec.translation = random_word_repre.translation;
        spec.correct_form = random_word_repre.correct_form;
    };

    WordSpec spec;
    spec.type = WordType::Correct;
    level->m_spawners2.emplace_back(std::make_shared<SpawnerT<WordFactory>>(*m_world, m_textures, spec, word_specifier, 1.f));
    level->m_spawners2.back()->setMaxCount(10);

    return level;
}
 */

void Game::handleEvent(const SDL_Event &event)
{
    auto mouse_position = m_window.getMouseInWorld();

    if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
    {
        int new_width = event.window.data1;
        int new_height = event.window.data2;
        // m_layers.resize(new_width, new_height);
        std::cout << "Changed window size in game window is: " << m_window.getTargetSize().x << " " << m_window.getTargetSize().y << std::endl;
        std::cout << "Changed window size in game event size: " << new_width << " " << new_height << std::endl;
        m_camera.setSize({800.f * (float)(new_width) / new_height, 800.f});

        m_layers.resize(new_width, new_height);
        repositionUI();
    }
    if (event.type == SDL_MOUSEWHEEL)
    {
        if (event.wheel.y > 0)
        {
            m_camera.zoom(0.9f);
        }
        else if (event.wheel.y < 0)
        {
            m_camera.zoom(1. / 0.9f);
        }
    }
    if (event.type == SDL_MOUSEBUTTONDOWN)
    {
        for (auto &[id, button] : m_buttons)
        {
            button->handlePushEvent(Vec2{m_window.getMouseInScreen().x, m_window.getTargetSize().y - m_window.getMouseInScreen().y}, 0);
        }
    }
    if (event.type == SDL_MOUSEBUTTONUP)
    {
        for (auto &[id, button] : m_buttons)
        {
            button->handleReleaseEvent(Vec2{m_window.getMouseInScreen().x, m_window.getTargetSize().y - m_window.getMouseInScreen().y});
        }
    }

    utils::Vector2f window_size = {m_window.getTargetSize().x, m_window.getTargetSize().y};
    if (event.type == SDL_FINGERMOTION)
    {
        auto touch_pos = utils::Vector2f{event.tfinger.x, 1. - event.tfinger.y};
        touch_pos = {touch_pos.x * window_size.x, touch_pos.y * window_size.y};
        auto dr = utils::Vector2f{event.tfinger.dx * window_size.x, -event.tfinger.dy * window_size.y};
        for (auto &[id, button] : m_buttons)
        {
            button->handleMotionEvent(touch_pos, touch_pos + dr, event.tfinger.fingerId);
        }
    }
    if (event.type == SDL_FINGERDOWN)
    {
        auto touch_pos = utils::Vector2f{event.tfinger.x, 1. - event.tfinger.y};
        touch_pos = {touch_pos.x * window_size.x, touch_pos.y * window_size.y};
        for (auto &[id, button] : m_buttons)
        {
            button->handlePushEvent(touch_pos, event.tfinger.fingerId);
        }
        if (touch_pos.x > window_size.x / 2.f &&
            m_buttons.at("rightButton")->getFingerId() != event.tfinger.fingerId)
        {
            m_playerx->onJump();
        }
    }
    if (event.type == SDL_FINGERUP)
    {
        auto touch_pos = utils::Vector2f{event.tfinger.x, 1. - event.tfinger.y};
        touch_pos = {touch_pos.x * window_size.x, touch_pos.y * window_size.y};
        for (auto &[id, button] : m_buttons)
        {
            button->handleReleaseEvent(touch_pos);
        }
    }

    handleEventImpl(event);
}

void Game::handleEventImpl(const SDL_Event &event)
{

#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#endif

    if (event.type == SDL_KEYDOWN)
    {
        auto key_sym = event.key.keysym.sym;
        if (key_sym == SDLK_d || key_sym == SDLK_RIGHT)
        {
            m_playerx->onRunStart(1);
        }
        if (key_sym == SDLK_a || key_sym == SDLK_LEFT)
        {
            m_playerx->onRunStart(-1);
        }
        if (key_sym == SDLK_SPACE || key_sym == SDLK_w || key_sym == SDLK_UP)
        {
            m_playerx->onJump();
        }
        if (event.key.keysym.sym == SDLK_LCTRL)
        {
            Bullet::Spec spec;
            spec.pos = {m_playerx->getPosition().x, m_playerx->getPosition().y + m_playerx->getSize().y + 5.f};
            spec.vel = {0.f, 200.f};
            spec.size = {20.f, 20.f};
            m_factories.at(ObjectType::Bullet)->create(spec);
        }
    }
    if (event.type == SDL_KEYUP)
    {
        auto key_sym = event.key.keysym.sym;
        if (key_sym == SDLK_d || key_sym == SDLK_RIGHT)
        {
            m_playerx->onRunEnd(1);
        }
        if (key_sym == SDLK_a || key_sym == SDLK_LEFT)
        {
            m_playerx->onRunEnd(-1);
        }
    }
}

//! \brief parse events and normal input
void Game::parseInput(float dt)
{
    m_playerx->m_accelerating = isKeyPressed(m_key_binding[PlayerControl::MOVE_FORWARD]);
    m_playerx->m_deccelerating = isKeyPressed(m_key_binding[PlayerControl::MOVE_BACK]);
}

void Game::update(float dt)
{

#if defined(__EMSCRIPTEN__)
    if (m_move_by_tilting)
    {
        m_playerx->m_moving_left = js::getDeviceGamma() < -20.f;
        m_playerx->m_moving_right = js::getDeviceGamma() > 20.f;
    }
#endif

    m_world->update(dt);
    if (!m_levels.empty())
    {
        m_levels.front()->update(dt);
    }
    m_camera.update(dt);

    m_timers.update(dt);
    messanger.distributeMessages();
    m_objective_system->update(dt);

    this->updateImpl(dt);

    // frame_count++;
    // if (frame_count % 250 == 0)
    // {
    //     std::cout << "update took: " << getDt(timeNow(), tic) << std::endl;
    //     frame_count = 0;
    // }
};

void Game::draw(Renderer &window)
{
    auto tic = timeNow();

    // Sprite background_rect;
    auto old_view = window.m_view;
    m_window.m_view = m_window.getDefaultView();
    if (m_background_tex)
    {
        auto &bg_texture = *m_background_tex;
        auto bg_size = bg_texture.getSize();

        m_background.setPosition(Vec2(m_window.getTargetSize()) / 2.f);
        m_background.setScale(Vec2(m_window.getTargetSize()) / 2.f);
        m_background.setScale(m_background.getScale().x, -m_background.getScale().y);
        m_window.drawSprite(m_background);
        m_window.drawAll();
    }
    m_window.m_view = m_camera.getView();


    m_layers.getLayer("Unit")->setBackground({0.f, 0.f, 0.f, 0.f});
    m_layers.getLayer("Bloom")->setBackground({0.f, 0.f, 0.f, 0.f});
    m_layers.clearAllLayers();
    Sprite ss(*m_textures.get("Arrow"));
    Vec2 buffer_size = m_layers.getCanvas("Unit").getTargetSize();
    ss.setPosition(buffer_size / 2.f);
    ss.setScale(buffer_size / 2.f);
    m_window.m_blend_factors = {BlendFactor::One, BlendFactor::OneMinusSrcAlpha};
    m_world->draw(m_layers, m_assets, m_window, m_camera.getView());
    m_layers.setView(m_window.m_view);

    // RectangleSimple mouse_rect;
    // mouse_rect.setPosition(m_window.getMouseInWorld());
    // mouse_rect.setScale(400, 400);
    // mouse_rect.m_color = {50,0,10,1};
    // m_layers.getCanvas("Light").drawRectangle(mouse_rect, "Light");
    
    m_layers.drawInto(m_window);

    //! draw the scene into the window
    m_window.m_view = m_window.getDefaultView();
    if (js::isMobile())
    {
        for (auto &[id, button] : m_buttons)
        {
            button->draw(m_window);
        }
    }
    auto old_factors = m_window.m_blend_factors;

    m_window.drawAll();

    drawImpl(window);

    m_window.m_view = old_view;
    m_window.m_blend_factors = old_factors;

    /*     if (frame_count % 250 == 1)
        {
            std::cout << "Draw took: " << getDt(timeNow(), tic) << " ms" << std::endl;
     } */
    // m_objective_system->draw(window, m_textures);
}

void WordGenerator::setRepre(std::vector<WordRepresentation> repre)
{
    repres = repre;
    permute(repres);
}

WordRepresentation WordGenerator::getNext()
{
    auto repre = repres.at(current_index);
    current_index++;
    if (current_index >= repres.size())
    {
        current_index = 0;
        permute(repres);
    }
    return repre;
}

template <class... TypeList>
void Game::createFactories()
{
    ((m_factories[getTypeId<TypeList>()] = std::make_unique<EntityFactory2<TypeList>>(*m_world)), ...);
    registerSerializers();
}