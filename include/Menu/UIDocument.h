#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>

#include <Rect.h>
#include <Text.h>
#include <Sprite.h>

enum class UIEvent
{
    MOUSE_ENETERED,
    MOUSE_LEFT,
    KEYBOARD_UP,
    KEYBOARD_DOWN,
    TEXT_INPUT,
    CLICK
};

enum class Layout
{
    X,
    Y,
    Grid,
    Hidden,
};

enum class Alignement
{
    None,
    Left,
    Center,
    CenterX,
    CenterY,
    Bottom,
    Top,
    Right,
};

struct Style
{
    Alignement align;
    Layout display;
};

enum class Sizing
{
    FIXED,
    SCALE_TO_FIT,
    RESCALE_CHILDREN,
};

enum class Position
{
    ABSOLUTE,
    RELATIVE,
};

class Renderer;
class Font;

struct Percentage
{
    float value;
    operator float() const { return value; }
};

struct Pixels
{
    float value;
    operator float() const { return value; }
};

struct Viewport
{
    float value;
    operator float() const { return value; }
};
using SizeVariant = std::variant<Percentage, Pixels, Viewport>;

struct UIElement
{
    using UIElementP = std::shared_ptr<UIElement>;

    utils::Vector2<SizeVariant> dimensions = {Viewport{0}, Viewport{0}};

    bool m_hidden = false;
    Rect<int> bounding_box;
    UIElement *parent = nullptr;
    UIElement *prev_sibling = nullptr;
    UIElement *next_sibling = nullptr;
    std::string id;

    int max_width;

    enum class DimensionType
    {
        Relative,
        Absolute,
    };

    DimensionType width_style = DimensionType::Absolute;
    DimensionType height_style = DimensionType::Absolute;

    bool mouse_hovering = false;

    utils::Vector2i margin = {0, 0};
    utils::Vector2i padding = {0, 0};
    int column_count = 3;

    Sizing sizing = Sizing::FIXED;
    Layout layout = Layout::X;
    Alignement align_x = Alignement::None;
    Alignement align_y = Alignement::None;
    Alignement content_align = Alignement::Left;
    Alignement content_align_x = Alignement::Left;
    Alignement content_align_y = Alignement::Top;

    std::vector<UIElementP> children;

    std::unordered_map<UIEvent, std::function<void(UIElementP)>> event_callbacks;

public:
    virtual ~UIElement() = default;
    virtual void update();
    virtual void draw(Renderer &canvas);

    void drawBoundingBox(Renderer& canvas);
    
    template <class... Args>
    void addChildren(Args... child_el);
    
    void addChild(UIElementP child_el);
    void addChildAfter(UIElementP child_el, UIElementP after_who);
    
    UIElement *getElementById(const std::string &id);
    bool removeElementById(const std::string &id);


    void drawX(Renderer &canvas);
    
    int widthContent() const;
    int width() const;
    int heightContent() const;
    int height() const;
    int x() const;
    int y() const;
    int rightContent() const;
    int right() const;
    int leftContent() const;
    int left() const;
    int topContent() const;
    int top() const;
    utils::Vector2i centerContent() const;
    int center() const;
    
private:
    
    void calculateBoundingBoxSize();

    utils::Vector2i totalChildrenMargin() const;
    
    double maxChildrenHeight() const;
    double maxChildrenWidth() const;
    double totalChildrenWidth() const;
    double totalChildrenHeight() const;
};

struct KeyFrame
{
    Rect<int> bounding_box;
    utils::Vector2i padding;
    float border_size;
};

struct Transition
{

    std::vector<KeyFrame> m_frames;
    std::vector<float> m_durations;

    int current_frame;
    float duration_from_last_frame;

    bool is_active = false;

    KeyFrame interpolate(const KeyFrame &k1, const KeyFrame &k2, float t_from_frame_start, float frame_duration)
    {
        float x = t_from_frame_start / frame_duration;

        KeyFrame result = k1;
        result.bounding_box.width = k1.bounding_box.width * (1 - x) + k2.bounding_box.width * x;
        result.bounding_box.height = k1.bounding_box.height * (1 - x) + k2.bounding_box.height * x;
        result.bounding_box.pos_x = k1.bounding_box.pos_x * (1 - x) + k2.bounding_box.pos_x * x;
        result.bounding_box.pos_y = k1.bounding_box.pos_y * (1 - x) + k2.bounding_box.pos_y * x;
        return result;
    }

    void update(float dt, UIElement *el)
    {
        duration_from_last_frame += dt;
        el->bounding_box = interpolate(m_frames.at(current_frame), m_frames.at(current_frame + 1),
                                       duration_from_last_frame, m_durations.at(current_frame))
                               .bounding_box;

        if (duration_from_last_frame >= m_durations.at(current_frame))
        {
            current_frame++;
            duration_from_last_frame = 0.;
        }

        if (current_frame == m_frames.size() - 1)
        {
            is_active = false;
        }
    }

    void start()
    {
        is_active = true;
        duration_from_last_frame = 0.;
        current_frame = 0;
    }
};

class UIDocument
{
public:
    UIDocument(Renderer &window_canvas);

    void onEvent(UIEvent event, UIElement::UIElementP node_p);

    void handleEvent(UIEvent event);

    UIElement *getElementById(const std::string &id) const;
    bool removeElementById(const std::string &id) const;

    void forEach(std::function<void(UIElement::UIElementP)> callback);

    void drawBoundingBoxes();

    void drawUI();

    UIElement::UIElementP root = nullptr;
    Renderer &document;
};

struct MultiLineUIElement : public UIElement
{
    MultiLineUIElement(Font &font, std::string text);

    virtual void draw(Renderer &canvas) override;
    
    // void setFont(Font &font);
    MultiLineText m_text;
};

struct TextUIELement : public UIElement
{
    TextUIELement(Font &font, std::string text);

    virtual void draw(Renderer &canvas) override;

    void setFont(Font &font);

    Text m_text;
};

struct SpriteUIELement : public UIElement
{
    SpriteUIELement(std::string shader_id = "", Texture *texture = nullptr)
        : m_shader_id(shader_id)
    {
        if(texture)
        {
            image.setTexture(0, *texture);
        }
    }

    virtual void draw(Renderer &canvas) override;
    void setTexture(Texture &tex);

    std::string m_shader_id;
    Sprite image;
};

// struct SliderUIELement : UIElement
// {

//     SliderUIELement()
//     {

//     }
//     // virtual void draw(Renderer &canvas) override;
//     void setTexture(Texture &tex);

//     double value = 0.;
//     double min_value = 0.;
//     double max_value = 1.;

//     Sprite image;
// };

struct TextInputElement : public UIElement
{
    TextInputElement() : UIElement()
    {
        m_input.setText("Placeholder");

        event_callbacks[UIEvent::TEXT_INPUT] = [](auto node) {

        };
    }

    virtual void draw(Renderer &canvas) override
    {
    }

    Text m_input;
};

template <class... Args>
void UIElement::addChildren(Args... child_el)
{
    (addChild(child_el), ...);
}
