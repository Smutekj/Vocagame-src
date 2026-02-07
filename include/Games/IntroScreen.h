#pragma once

#include "Screen.h"
#include "DrawLayer.h"
#include "Assets.h"
#include "Menu/UIButtons.h"
#include "Systems/TimedEventManager.h"

#include "nlohmann/json.hpp"

class IntroScreen : public Screen
{
public:
    IntroScreen(Renderer &window, Assets &assets, const nlohmann::json &text_resources, std::function<void()> start_callback);
    virtual ~IntroScreen() override {}

    virtual void handleEvent(const SDL_Event &event) final;
    virtual void update(float dt) final;
    virtual void draw(Renderer &window) final;

private:
    Renderer &m_window;
    LayersHolder m_layers;
    TextureHolder &m_textures;
    nlohmann::json m_text_resources;
    std::shared_ptr<Font> m_font;
    utils::Vector2f m_intro_text_pos;
    MultiLineText m_intro_text;
    TextButton m_start_button;
    MultiLineText m_intro;
    MultiLineText m_controls;

    TimedEventManager m_timers;
};

class UIButtonI;
class MainMenu : public Screen
{
public:
    MainMenu(Renderer &window, Assets &assets, const nlohmann::json &text_resources,
            std::function<void(std::string)> start_callback);
    virtual ~MainMenu() override {}

    virtual void handleEvent(const SDL_Event &event) final;
    virtual void update(float dt) final;
    virtual void draw(Renderer &window) final;

private:
    Renderer &m_window;
    std::vector<std::unique_ptr<UIButtonI>> m_buttons;
    TimedEventManager m_timers;
};