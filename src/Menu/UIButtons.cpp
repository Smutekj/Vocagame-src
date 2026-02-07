#include "UIButtons.h"

#include "DrawLayer.h"

UIButtonI::UIButtonI()
{
}

bool UIButtonI::isPushed() const
{
    return m_is_pushed;
}
int UIButtonI::getFingerId() const
{
    return m_finger_id;
}

void UIButtonI::setOnClick(std::function<void()> on_click)
{
    m_on_click = on_click;
}
void UIButtonI::setOnRelease(std::function<void()> on_release)
{
    m_on_release = on_release;
}

Rectf UIButtonI::getBoundingBox() const
{
    return {getPosition().x - getScale().x,
            getPosition().y - getScale().y,
            getScale().x * 2, getScale().y * 2};
}

void UIButtonI::handleEvent(const SDL_Event &event, utils::Vector2f cursor_pos)
{
    if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
    {
    }
    if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT)
    {
    }
    if (event.type == SDL_MOUSEMOTION)
    {
        // handleMotionEvent(cursor_pos, , 0);
    }
}

void UIButtonI::handleMotionEvent(utils::Vector2f init_pos, utils::Vector2f final_pos, int finger_id)
{
    auto bb = getBoundingBox();
    if (m_is_pushed && !bb.contains(final_pos) && finger_id == m_finger_id)
    {
        m_on_release();
        m_is_pushed = false;
        m_finger_id = -1;
    }
}
void UIButtonI::handleReleaseEvent(utils::Vector2f touch_pos)
{
    if (getBoundingBox().contains(touch_pos))
    {
        m_on_release();
    }
    m_finger_id = -1;
    m_is_pushed = false;
}
void UIButtonI::handlePushEvent(utils::Vector2f touch_pos, int finger_id)
{
    if (getBoundingBox().contains(touch_pos))
    {
        m_finger_id = finger_id;
        m_on_click();
        m_is_pushed = true;
    }
}

TextButton::TextButton(const std::string &text, Font &font)
    : m_text(text)
{
    m_text.setFont(&font);
}



void TextButton::draw(LayersHolder &layers, Renderer& window)
{
    auto& bg_canvas = layers.getCanvas("Background");
    auto& text_canvas = layers.getCanvas("Text");
    auto& shiny_canvas = layers.getCanvas("Bloom");

    bool hovered = getBoundingBox().contains(bg_canvas.getMouseInWorld());
    if (hovered && !m_is_hovered)
    {
        m_on_hover(*this);
    }
    else if (!hovered && m_is_hovered)
    {
        m_on_hover_stop(*this);
    }
    m_is_hovered = hovered;

    RectangleSimple bg;
    bg.m_color = m_background_color;
    bg.setPosition(getPosition());
    bg.setScale(getScale()*1.3);
    bg_canvas.drawRectangle(bg, "gradientCircle");

    drawShinyRectangle(shiny_canvas, getPosition(), getScale()*1.3, m_border_color);

    m_text.centerAround(getPosition());
    m_text.setScale(1.f, 1.f);
    auto text_bb = m_text.getBoundingBox();
    float aspect = text_bb.width / text_bb.height;
    m_text.setScale(getScale().x / text_bb.width, getScale().x / text_bb.width);
    if (m_is_pushed)
    {
        m_when_pushed(m_text);
    }

    text_canvas.drawText2(m_text);
}
void TextButton::draw(Renderer &canvas)
{
    bool hovered = getBoundingBox().contains(canvas.getMouseInWorld());
    if (hovered && !m_is_hovered)
    {
        m_on_hover(*this);
    }
    else if (!hovered && m_is_hovered)
    {
        m_on_hover_stop(*this);
    }
    m_is_hovered = hovered;

    m_text.setPosition(getPosition());
    m_text.centerAroundX(getPosition().x);
    m_text.setScale(1.f, 1.f);
    auto text_bb = m_text.getBoundingBox();
    float aspect = text_bb.width / text_bb.height;
    // m_text.setScale(getScale().x / text_bb.width, getScale().x / text_bb.width);
    if (m_is_pushed)
    {
        m_when_pushed(m_text);
    }

    canvas.drawText2(m_text);
}
void TextButton::setWhenPushed(std::function<void(Text &)> pushed)
{
    m_when_pushed = pushed;
}

IconButton::IconButton(Texture &texture)
    : m_icon(texture)
{
}

void IconButton::draw(Renderer &canvas)
{
    m_icon.setPosition(getPosition());
    m_icon.setScale(getScale());
    m_icon.setRotation(getRotation());
    if (m_is_pushed)
    {
        m_when_pushed(m_icon);
    }
    canvas.drawSprite(m_icon);
}
void IconButton::setWhenPushed(std::function<void(Sprite &)> pushed)
{
    m_when_pushed = pushed;
}

void TextButton::setOnHover(std::function<void(TextButton &)> hover)
{
    m_on_hover = hover;
}

void drawShinyRectangle(Renderer &canvas, utils::Vector2f pos, utils::Vector2f size, Color color)
{

    float line_w = 2.0f;
    utils::Vector2f l_size = {size.x, size.y};
    // RectangleSimple line_rect;
    // line_rect.setColor(color);
    // line_rect.setScale(line_w * 2, size.y + line_w);
    // line_rect.setPosition(Vec2{pos.x - size.x / 2.f - line_w, pos.y});
    // canvas.drawRectangle(line_rect, color, "gradientX");
    // line_rect.setRotation(180.f);
    // line_rect.setPosition(Vec2{pos.x + size.x / 2.f + line_w, pos.y});
    // canvas.drawRectangle(line_rect, color, "gradientX");
    // line_rect.setRotation(0.f);
    // line_rect.setScale(size.x + line_w, line_w * 2);
    // line_rect.setPosition(Vec2{pos.x, pos.y - size.y / 2.f - line_w});
    // canvas.drawRectangle(line_rect, color, "gradientY");
    // line_rect.setRotation(180.f);
    // line_rect.setPosition(Vec2{pos.x, pos.y + size.y / 2.f + line_w});
    // canvas.drawRectangle(line_rect, color, "gradientY");
    canvas.drawLineBatched({pos.x + size.x / 2.f, pos.y + l_size.y / 2.f + line_w / 2.f}, {pos.x + l_size.x / 2.f, pos.y - l_size.y / 2.f - line_w / 2.f}, line_w, color);
    canvas.drawLineBatched({pos.x + l_size.x / 2.f + line_w / 2.f, pos.y - l_size.y / 2.f}, {pos.x - l_size.x / 2.f - line_w / 2.f, pos.y - l_size.y / 2.f}, line_w, color);
    canvas.drawLineBatched({pos.x - l_size.x / 2.f, pos.y - l_size.y / 2.f - line_w / 2.f}, {pos.x - l_size.x / 2.f, pos.y + l_size.y / 2.f + line_w / 2.f}, line_w, color);
    canvas.drawLineBatched({pos.x - l_size.x / 2.f - line_w / 2.f, pos.y + l_size.y / 2.f}, {pos.x + l_size.x / 2.f + line_w / 2.f, pos.y + l_size.y / 2.f}, line_w, color);
}