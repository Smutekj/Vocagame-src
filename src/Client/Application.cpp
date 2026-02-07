#include "Application.h"

#include "PostEffects.h"
#include "DrawLayer.h"

#include "Games/IntroScreen.h"
#include "Utils/Time.h"
#include "Utils/RandomTools.h"
#include "ImageExtractor.h"

#include <Shader.h>

#include "Utils/IOUtils.h"

#include "Games/JumpGame.h"
#include "Games/CastleGame.h"
#include "Games/DodgeGame.h"
#include "Games/IntroScreen.h"
#include "Games/RaceGame.h"
#include "Games/SnakeGame.h"
#include "Games/SpaceGame.h"
#include "Games/SnakeGameSP.h"
#include "Games/DungeonGame.h"
#include "Games/VocabularyExam.h"

//! editor only locally!
#ifndef __EMSCRIPTEN__
#include "serializers.h"
#include "Editor.h"
#endif

#if defined(__EMSCRIPTEN__)

bool s_assets_loaded = false;
Application *p_my_app = nullptr;
extern "C"
{
    bool assetsLoaded()
    {
        return s_assets_loaded && p_my_app;
    }

    void startGame(int game_id)
    {
        int games_count = static_cast<int>(Application::GameId::Count);
        if (game_id < 0 || game_id >= games_count)
        {
            std::cout << "GAME ID: " << game_id << " DOES NOT EXIST!";
        }
        else
        {
            if (p_my_app)
            {
                auto id = static_cast<Application::GameId>(game_id);
                p_my_app->startGame(id);
            }
        }
    }

    void setCanvasSize(int width, int height)
    {
        if (p_my_app)
        {
            double dpr = js::getDpr();
            std::cout << "Changing canvas size from Browser: " << width << " " << height << std::endl;
            p_my_app->m_window.setSize(dpr * width, dpr * height);
        }
    }

    void toggleFullscreen(int w, int h)
    {
        if (p_my_app)
        {
            SDL_SetWindowFullscreen(p_my_app->m_window.getHandle(), SDL_WINDOW_FULLSCREEN);
        }
    }
    void disableInput(bool disable)
    {
        std::cout << "CALLED disableInput with: " << disable << std::endl;
        if (disable)
        {
            SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);
            SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_IGNORE);
            SDL_EventState(SDL_MOUSEBUTTONUP, SDL_IGNORE);
            SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);
            SDL_EventState(SDL_KEYUP, SDL_IGNORE);
        }
        else
        {
            SDL_EventState(SDL_MOUSEMOTION, SDL_ENABLE);
            SDL_EventState(SDL_MOUSEBUTTONDOWN, SDL_ENABLE);
            SDL_EventState(SDL_MOUSEBUTTONUP, SDL_ENABLE);
            SDL_EventState(SDL_KEYDOWN, SDL_ENABLE);
            SDL_EventState(SDL_KEYUP, SDL_ENABLE);
        }
    }
}

EM_JS(bool, setAssetsLoaded, (), {return Module.setAssetsLoaded()});
EM_JS(void, updateLoadingProgress, (int progress, int total), {return Module.updateLoadingProgress(progress, total)});
#endif

void Application::run()
{
#if defined(__EMSCRIPTEN__)
    p_my_app = this;
    int fps = 0; // Use browser's requestAnimationFrame
    emscripten_set_main_loop_arg(loadAssetsLoop, (void *)this, fps, true);
    // emscripten_set_main_loop_timing(EM_TIMING_RAF, 2);
#else
    createMenu();
    while (!m_window.shouldClose())
        gameLoop((void *)this);
#endif
}

void Application::iterate()
{

    if (m_screen_stack.size() > 0)
    {
        m_screen_stack.back()->update(m_dt);
    }

    //! poll and events let state stack handle them
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {

        if (m_screen_stack.size() > 0)
        {
            m_screen_stack.back()->handleEvent(event);
        }

#if defined(__EMSCRIPTEN__)
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
        {
            js::pauseGame("PAUSED");
        }
#else
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
        {
            m_stack_actions.push_back([this](auto &stack)
                                      {
                stack.clear();
                stack.push_back(std::make_unique<Editor>(m_window_canvas,
                                                 m_bindings,
                                                m_assets)); });
        }
        if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_i)
        {
            m_stack_actions.push_back([this](auto &stack)
                                      {
                                          // stack.clear();
                                          // stack.push_back(std::make_unique<Editing::ImageExtractor>(m_window_canvas,
                                          //                                  m_bindings,
                                          //                                  m_assets));
                                      });
        }
#endif

        if (event.type == SDL_QUIT)
        {
            m_window.close();
        }
        if (event.type == SDL_WINDOWEVENT)
        {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED)
            {
                std::cout << "WINDOW SIZE RESIZED EVENT: " << std::endl;
                std::cout << "W: " << event.window.data1 << " H: " << event.window.data2 << std::endl;
                double dpr = js::getDpr();
                auto width = event.window.data1;
                auto height = event.window.data2;
                if (width != m_window.getSize().x || height != m_window.getSize().y)
                {
                    m_window.setSize(dpr * width, dpr * height);
                }
            }
            else if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                std::cout << "WINDOW SIZE CHANGED EVENT: " << std::endl;
                std::cout << "W: " << event.window.data1 << " H: " << event.window.data2 << std::endl;

                auto width = event.window.data1;
                auto height = event.window.data2;
                m_window_canvas.m_view.setCenter(m_window_canvas.getTargetSize() / 2.f);
                m_window_canvas.m_view.setSize(m_window_canvas.getTargetSize());
            }
        }
    }
    m_window_canvas.clear({0, 0, 0, 1});

    for (auto &screen : m_screen_stack)
    {
        screen->draw(m_window_canvas);
    }

    while (!m_stack_actions.empty())
    {
        m_stack_actions.front()(m_screen_stack);
        m_stack_actions.pop_front();
    }

    Shader::m_time += m_dt;
}

static TimeType tic_sim;
static TimeType tic_frame;
static std::size_t s_frame_count = 0;

void inline gameLoop(void *mainLoopArg)
{

    Application *p_app = (Application *)mainLoopArg;

    auto now = timeNow();
    auto dt = getDt(now, tic_frame);
    tic_frame = now;
    tic_sim = now;

    p_app->iterate();

    now = timeNow();
    double dt_simulation = getDt(now, tic_sim);

    SDL_GL_SwapWindow(p_app->m_window.getHandle()); // Swap front/back framebuffers
    double dt_swap_window = getDt(timeNow(), now);

    p_app->m_avg_frame_time.addNumber(dt);
    s_frame_count++;
    if (s_frame_count > 250)
    {
        p_app->m_avg_frame_time.averaging_interval = 250;
        std::cout << "simulation frame time " << dt_simulation << " ms" << std::endl;
        std::cout << "avg frame time: " << p_app->m_avg_frame_time.getAverage() << " ms" << std::endl;
        std::cout << "max game frame time: " << p_app->m_avg_frame_time.getMax() << " ms" << std::endl;
        std::cout << "gl swap time:  " << dt_swap_window << " ms" << std::endl;
        std::cout << "actual frame time: " << dt << " ms" << std::endl;
        p_app->m_avg_frame_time.reset();
        s_frame_count = 0;
    }

    p_app->m_dt = std::min(0.05, dt / 1000.); //! limit the timestep for debugging
}

#ifndef RESOURCES_DIR
static_assert(false, "RESOURCES_DIR MACRO NEEDS TO BE DEFINED!")
#endif

    Application::Application(int width, int height)
    : m_window(width, height), m_window_canvas(m_window)
{
#if defined(__EMSCRIPTEN__)
    std::string resource_dir = {RESOURCES_DIR};
    m_assets.textures.setBaseDirectory(resource_dir + "Textures/");
#endif

    registerSerializers(); //! it needs to be run exactly once but i don't know where to put this :(
}

bool Application::loadAssets()
{
    m_text_resources = utils::loadJson((std::string(RESOURCES_DIR) + "TextResources.json").c_str());

    //     //! set view and add it to renderers
    m_window_canvas.m_view.setSize(m_window_canvas.getTargetSize());
    m_window_canvas.m_view.setCenter(m_window_canvas.getTargetSize() / 2);

    m_window_canvas.setShadersPath(std::string(RESOURCES_DIR) + "Shaders/");
    m_window_canvas.addShader("Text", "basicinstanced.vert", "textBorder.frag");

    //! this is so retarded, omg...
    bool fetch_locally = true;
#if defined(__EMSCRIPTEN__)
    fetch_locally = false;
#endif
    std::string resource_dir = {RESOURCES_DIR};

    nlohmann::json resources_data = {};
    try
    {
        resources_data = utils::loadJson((resource_dir + "Resources.json").c_str());
    }
    catch (std::exception &e)
    {
        std::cout << "Failed to load Resource Registry json!" << e.what() << std::endl;
    }

    for (auto &[resource_type, resources] : resources_data.items())
    {
        for (auto &[id, resource_data] : resources.items())
        {
            resource_data["id"] = id;
            auto fetcher = makeFetcher(resource_type, resource_data, m_assets, fetch_locally);
            m_to_fetch.insert({std::string(id), fetcher});
        }
    }
    for (auto &[id, fetcher] : m_to_fetch)
    {
        if (fetcher)
        {
            fetcher->doFetch();
        }
    }
    m_assets.textures.setBaseDirectory(resource_dir + "Textures/");

    return true;
}

Application *p_app = nullptr;

#if defined(EMSCRIPTEN)
void inline loadAssetsLoop(void *mainLoopArg)
{
    Application &app = *static_cast<Application *>(mainLoopArg);
    p_app = &app;
    auto &to_fetch = app.m_to_fetch;
    int loaded_count = std::accumulate(to_fetch.begin(), to_fetch.end(), 0, [](int sum, auto &x)
                                       { return sum + x.second->isLoaded(); });
    updateLoadingProgress(loaded_count, to_fetch.size());
    bool all_loaded = loaded_count == to_fetch.size();
    if (all_loaded)
    { // switch to game loop when all loaded
        std::cout << "ALL LOADED " << std::endl;
        setAssetsLoaded();
        s_assets_loaded = true;

        // p_app->createGame();
        emscripten_cancel_main_loop();
        emscripten_set_main_loop_arg(gameLoop, (void *)p_app, 0, true);
    }
}
#endif

template <class GameType>
void Application::pushGameStart(GameId game_id)
{
    std::string native_lang = "en";
    try
    {
        std::string lang_path = std::string{RESOURCES_DIR} + "execDb/SelectedLanguages.json";
        auto lang_json = utils::loadJson(lang_path.c_str());
        native_lang = lang_json.at("native");
    }
    catch (std::exception &e)
    {
        std::cout << "StudiedLanguages.json does not exist or smthign!" << std::endl;
    }

    m_stack_actions.push_back([this, game_id, native_lang](auto &stack)
                              {
        while (stack.size() > 1)
        {
            stack.pop_back();
        }

        m_screen_stack.push_back(std::make_unique<GameType>(m_window_canvas,
                                                             m_bindings,
                                                             m_assets));


        std::string game_id_text = enumToString(game_id);
        if (m_text_resources.at(native_lang).contains(game_id_text))
        {
            m_screen_stack.push_back(std::make_unique<IntroScreen>(m_window_canvas,
                                                                   m_assets,
                                                                   m_text_resources.at(native_lang).at(game_id_text),
                                                                   [this]()
                                                                   {
                                                                       m_stack_actions.push_back([](ScreenStack &screens)
                                                                                                 { screens.pop_back(); });
                                                                   }));
        } });
}

void Application::startGame(GameId game_id)
{
    if (game_id == GameId::Dodge)
    {
        pushGameStart<DodgeGame>(game_id);
    }
    else if (game_id == GameId::Castle)
    {
        pushGameStart<CastleGame>(game_id);
    }
    else if (game_id == GameId::Jump)
    {
        pushGameStart<JumpGame>(game_id);
    }
    else if (game_id == GameId::Snake)
    {
        pushGameStart<SnakeGameSP>(game_id);
    }
    else if (game_id == GameId::Dungeon)
    {
        pushGameStart<DungeonGame>(game_id);
    }
}

void Application::createMenu()
{
    std::string native_lang = "en";
    try
    {
        std::string lang_path = std::string{RESOURCES_DIR} + "execDb/SelectedLanguages.json";
        auto lang_json = utils::loadJson(lang_path.c_str());
        native_lang = lang_json.at("native");
    }
    catch (std::exception &e)
    {
        std::cout << "StudiedLanguages.json does not exist or smthign!" << std::endl;
    }
    auto on_game_start = [this](std::string game_id)
    {
        try
        {
            GameId id = stringToEnum<GameId>(game_id);
            startGame(id);
        }
        catch (std::exception &e)
        {
            std::cout << "Enum does not exist: " << e.what() << std::endl;
        }
    };
    // _screen_stack.push_back(std::make_unique<Editing::ImageExtractor>(m_window_canvas, m_bindings, m_assets));

#ifndef __EMSCRIPTEN__
    m_screen_stack.push_back(std::make_unique<MainMenu>(m_window_canvas, m_assets, m_text_resources.at(native_lang), on_game_start));
#endif
    // m_screen_stack.push_back(std::make_unique<JumpGame>(m_window_canvas, m_bindings, m_assets));
}