#pragma once

#include "Menu/UIDocument.h"
#include "PostOffice.h"
#include "PostBox.h"
#include "ComponentSystem.h"
#include "Systems/TimedEventManager.h"
// #include "Systems/TimedEventManager.h

class Font;

class Player;

class UISystem
{
public:
    UISystem(Renderer &window, TextureHolder &textures,
             PostOffice &messenger, Player *player,
             Font &font, GameWorld &world);

    void draw(Renderer &window);

    void showMessage(const std::string& message_text, float duration = -1);
    void update(float dt);

    TextUIELement *getTextElement(std::string id);

    void addTimeBar();
    void removeTimeBar();
    void showLevelFinishedMessage(LevelCompletedEvent event);

private:
    UIDocument ui;

    GameWorld *p_world;
    Player *p_player;
    PostOffice *p_post_office;

    std::vector<std::unique_ptr<PostBoxI>> m_post_boxes;

    TextureHolder &m_textures;
    Renderer &window_canvas;

    TimedEventManager m_timers;

    Font &m_font;

    int boss_id = -1;
};