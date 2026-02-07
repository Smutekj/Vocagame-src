#pragma once

#include <chrono>
#include <vector>
#include <deque>
#include <numeric>
#include <unordered_map>
#include <memory>

#include <Window.h>
#include <DrawLayer.h>

#include "Utils/Statistics.h"
#include "Commands.h"
#include "ToolBoxUI.h"

#include "Screen.h"

#include "SoundSystem.h"

#include "ResourceFetcher.h"


void gameLoop(void *mainLoopArg);
void loadAssetsLoop(void *mainLoopArg);

class Application
{

    using ScreenStack = std::vector<std::unique_ptr<Screen>>;
    using StackAction = std::function<void(ScreenStack &)>;

public:
    Application(int widht, int height);

    bool loadAssets();
    void run();
    void iterate();

    void createMenu();

    enum class GameId
    {
        Snake = 0,
        Jump = 1,
        Castle = 2,
        Dodge = 3,
        Dungeon= 4,
        Count
    };
    BOOST_DESCRIBE_NESTED_ENUM(GameId, Snake, Dodge, Jump, Castle, Dungeon);

    void startGame(GameId game_id);

    friend void gameLoop(void *);
    friend void loadAssetsLoop(void *);

private:
    template <class GameType>
    void pushGameStart(GameId id);

public:
    Window m_window;
    Renderer m_window_canvas;
    Assets m_assets;

    std::unordered_map<std::string, std::shared_ptr<ResourceFetcher>> m_to_fetch;

private:
    ScreenStack m_screen_stack;
    std::deque<StackAction> m_stack_actions;

    KeyBindings m_bindings; //! defines key->command bindings

    std::unordered_map<GameId, std::string> m_intro_texts;
    nlohmann::json m_text_resources;

    float m_dt = 0.f; //! time step
    Statistics m_avg_frame_time;
};
