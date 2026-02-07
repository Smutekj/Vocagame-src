#include "UISystem.h"
#include "Entities/Player.h"
#include "GameWorld.h"

#include <Renderer.h>
#include <Shader.h>

UISystem::UISystem(Renderer &window, TextureHolder &textures,
                   PostOffice &messenger, Player *player,
                   Font &font, GameWorld &world)
    : ui(window), p_post_office(&messenger),
      p_player(player), window_canvas(window),
      m_textures(textures), p_world(&world), m_font(font)
{
    // lorem_ipsum.setFont(&font);
    // lorem_ipsum.setText("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla id malesuada justo. Nunc aliquam accumsan lorem in facilisis. Maecenas a porttitor sem, in consectetur lacus. Donec eleifend a eros non mattis. Vivamus massa arcu, porttitor non massa sit amet, auctor imperdiet augue. Vestibulum commodo magna at turpis suscipit porta. Vestibulum a tellus vel diam luctus auctor. Integer tristique facilisis risus, a blandit ex lacinia ut.");


    auto m_timer_postbox = std::make_unique<PostBox<StartedTimerEvent>>(messenger, [this](const auto &events)
                                                                        {
        for(const auto&  e : events)
        {
            addTimeBar();
        } });

    auto m_died_postbox = std::make_unique<PostBox<EntityDiedEvent>>(messenger, [this](const auto &events)
                                                                     {
        for (const auto &e : events)
        {
            if(e.id == boss_id)
            {
                boss_id = -1;
            }
        } });
    auto m_level_completed = std::make_unique<PostBox<LevelCompletedEvent>>(messenger, [this](const auto &events)
                                                                            {
        for (const auto &e : events)
        {
            showLevelFinishedMessage(e);
        } });

    m_post_boxes.push_back(std::move(m_died_postbox));
    m_post_boxes.push_back(std::move(m_level_completed));

    auto top_bar = std::make_shared<UIElement>();

    auto bottom_bar = std::make_shared<UIElement>();
    auto fuel_bars = std::make_shared<UIElement>();
    auto health_bars = std::make_shared<UIElement>();

    auto health_bar = std::make_shared<SpriteUIELement>("healthBar");
    health_bar->dimensions = {Percentage{1.f}, Percentage{0.4f}};
    health_bar->id = "HealthBar";
    health_bar->margin.y = 10;

    health_bars->layout = Layout::Y;
    health_bars->align_x = Alignement::Left;
    health_bars->content_align_x = Alignement::Center;
    health_bars->content_align_y = Alignement::Center;
    health_bars->dimensions = {Percentage{0.2f}, Percentage{1.f}};
    health_bars->margin.x = {20};
    health_bars->id = "HB";

    health_bars->addChildren(health_bar);

    auto text_bars = std::make_shared<UIElement>();
    text_bars->id = "TextBars";
    text_bars->dimensions = {Percentage{1.f}, Percentage{1.f}};

    top_bar->dimensions = {Percentage{0.8f}, Percentage{0.2f}};
    top_bar->id = "TopBar";
    top_bar->layout = Layout::Y;
    top_bar->align_y = Alignement::Top;
    top_bar->align_x = Alignement::Center;
    top_bar->content_align_y = Alignement::Center;
    top_bar->addChildren(text_bars, health_bars);

    ui.root->dimensions = {Pixels{(float)window.getTargetSize().x}, Pixels{(float)window.getTargetSize().y}};
    ui.root->addChildren(top_bar, bottom_bar);
    ui.root->content_align_y = Alignement::Bottom;
    ui.root->content_align_x = Alignement::CenterX;
    ui.root->layout = Layout::Y;
}

TextUIELement *UISystem::getTextElement(std::string id)
{
    auto *item = ui.getElementById(id);
    return dynamic_cast<TextUIELement *>(item);
}

void UISystem::draw(Renderer &window)
{
    auto old_view = window.m_view;
    window.m_view = window.getDefaultView();
    window.m_view.setSize(window.m_view.getSize().x, -window.m_view.getSize().y);

    // #if DEBUG
    ////    ui.drawBoundingBoxes(); //! TODO: WHY THE FUCK IS IT FLIPPED WHEN I PUT IT AFTER drawUI???
    // #endif

    ui.drawUI();

    window.drawAll();

    window.m_view = old_view;
}

void UISystem::addTimeBar()
{
    auto time_bar = std::make_shared<TextUIELement>(m_font, "00:00");
    time_bar->id = "TimerBar";
    time_bar->dimensions = {Percentage{0.2f}, Percentage{0.5f}};
    time_bar->layout = Layout::Y;
    time_bar->align_x = Alignement::Left;
    time_bar->align_y = Alignement::CenterY;

    if (auto el = ui.getElementById("TextBars"))
    {
        el->addChild(time_bar);
    }
}

void UISystem::removeTimeBar()
{
    ui.removeElementById("TimerBar");
}

void UISystem::showLevelFinishedMessage(LevelCompletedEvent event)
{
    auto boss_bar = std::make_shared<TextUIELement>(m_font,
                                                    "Level " + std::to_string(event.level) + " Finished!");
    boss_bar->dimensions = {Percentage{1.f}, Percentage{0.2f}};
    boss_bar->id = "LevelMessage";
    boss_bar->align_y = Alignement::Center;

    if (auto el = ui.getElementById("TopBar"))
    {
        el->addChild(boss_bar);
    }

    m_timers.addTimedEvent([this](float t, int c)
                           { ui.removeElementById("LevelMessage"); }, 3.f);
}

void UISystem::update(float dt)
{

    m_timers.update(dt);

    float health_ratio = std::min({1.f, p_player->getHpRatio()});
    window_canvas.getShader("healthBar").setUniform("u_health_ratio", health_ratio);

    //!
}

void UISystem::showMessage(const std::string &message_text, float duration)
{
    auto boss_bar = std::make_shared<TextUIELement>(m_font, message_text);
    boss_bar->dimensions = {Percentage{1.f}, Percentage{0.2f}};
    boss_bar->id = "Message";
    boss_bar->align_y = Alignement::Center;

    if (auto el = ui.getElementById("TopBar"))
    {
        el->addChild(boss_bar);
    }

    if (duration > 0.f)
    {
        m_timers.addTimedEvent([this](float t, int c)
                               { ui.removeElementById("Message"); }, duration);
    }
}