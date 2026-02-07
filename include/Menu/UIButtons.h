#pragma once

#include <Transform.h>
#include <Renderer.h>
#include <Text.h>

#include <functional>
#include <any>

#include <SDL2/SDL_events.h>

class UIButtonI : public Transform
{
public:
    UIButtonI();
    virtual ~UIButtonI() = default;


    bool isPushed() const;
    int getFingerId() const;

    void setOnClick(std::function<void()> on_click);
    void setOnRelease(std::function<void()> on_release);

    Rectf getBoundingBox() const;

    void handleEvent(const SDL_Event &event, utils::Vector2f cursor_pos);
    void handleMotionEvent(utils::Vector2f init_pos, utils::Vector2f final_pos, int finger_id = 0);
    void handleReleaseEvent(utils::Vector2f touch_pos);
    void handlePushEvent(utils::Vector2f touch_pos, int finger_id = 0);
    virtual void draw(Renderer &canvas) = 0;

protected:
    bool m_is_hovered = false;
    bool m_is_pushed = false;
    int m_finger_id = -1;

private:
    std::function<void()> m_on_click = []() {};
    std::function<void()> m_on_release = []() {};
};

class LayersHolder;

class TextButton : public UIButtonI
{

public:
    TextButton(const std::string &text, Font &font);

    virtual void draw(Renderer &canvas) override;
    void draw(LayersHolder &layers, Renderer& window);
    void setWhenPushed(std::function<void(Text &)> pushed);
    void setOnHover(std::function<void(TextButton &)> hover);

    Color m_background_color = {0, 0, 0, 1};
    Color m_border_color = {50., 5., 0., 1.};
    float m_border_width = 0.f;
    Text m_text;
    utils::Vector2f m_bounding_rect_size;
    Color bounding_rect_color = {0, 0, 0, 1};
    std::function<void(TextButton &)> m_on_hover_stop = [](TextButton &) {};

private:
    std::function<void(TextButton &)> m_on_hover = [](TextButton &) {};
    std::function<void(Text &)> m_when_pushed = [](Text &) {};
};
class IconButton : public UIButtonI
{
public:
    IconButton(Texture &texture);

    virtual void draw(Renderer &canvas) override;
    void setWhenPushed(std::function<void(Sprite &)> pushed);

private:
    std::function<void(Sprite &)> m_when_pushed = [](Sprite &) {};
    Sprite m_icon;
};

void drawShinyRectangle(Renderer &canvas, utils::Vector2f pos, utils::Vector2f size, Color color);