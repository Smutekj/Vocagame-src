#pragma once

#include "GameObject.h"
#include "Components.h"
#include <Text.h>
#include "EffectsFactory.h"
#include "TimedEventManager.h"

class Font;

class TextBubble : public GameObject
{
public:
    struct Spec : public GameObjectSpec
    {
        Spec() : GameObjectSpec(typeid(Spec)) {}
        Spec(std::type_index rtti) : GameObjectSpec(rtti) {
            obj_type = TypeId::TextBubble;
        }

        Path path;
        std::string meaning_id = "Penis";
        std::string text = "Penis";
        std::string correct_form = "Penis";
        std::string translation = "Schlong";
        
        ColorByte top_color = {255, 255, 255,255};
        ColorByte bottom_color = {255, 255, 0, 255};
        ColorByte glow_color = {0,0,0,0};
        ColorByte border_color = {0, 0, 0, 255};
    };

public:
    TextBubble(GameWorld &world, const Spec &spec, int ent_id = -1);

    virtual void update(float dt) override;
    virtual void onCreation() override;
    virtual void draw(LayersHolder &target, Assets& assets) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

    void setText(const std::string &text);
    const std::string &getText() const;
    const std::string &getTranslation() const;
    bool firstTouch() const;
    void setImage(Sprite word_image);
    void setTranslation(const std::string &trans);
    void setCorrectForm(const std::string &text);
    void setFont(Font &font);
    const std::string &getCorrectForm() const;
    bool isCorrect() const;
    void setTextHeight(float height);
public:
    bool m_player_is_standing = false;
    bool first_touch = true;

    float m_opacity = 1.f;
    Text m_drawable;
    RectangleSimple light_rect;
    TimedEventManager m_timers;
protected:
    bool m_has_image = false;
    bool m_is_correct = false;
    std::string m_correct_form = "Testing";
    std::string m_text = "Testing";
    std::string m_translation = "Penis";
    Sprite m_word_image;
    Font *m_font = nullptr;

    ColorByte m_bottom_color;
    ColorByte m_top_color;

   

    EffectsFactory m_effects_maker;

};
BOOST_DESCRIBE_STRUCT(TextBubble::Spec, (GameObjectSpec), (path, text, correct_form, translation, top_color, glow_color, border_color));

class TextPlatform : public TextBubble
{
public:
    struct Spec : public TextBubble::Spec
    {
        Spec() : TextBubble::Spec(typeid(Spec))
        {
            obj_type = TypeId::TextPlatform;
        }

        float fall_speed = 10.f;
    };

public:
    TextPlatform(GameWorld &world, const Spec &spec, int ent_id = -1);

    // virtual void update(float dt) override;
    virtual void onCollisionWith(GameObject &obj, CollisionData &c_data) override;

private:
};
BOOST_DESCRIBE_STRUCT(TextPlatform::Spec, (TextBubble::Spec), (fall_speed));

class TextRect : public TextBubble
{
public:
    struct Spec : public TextBubble::Spec
    {
        Spec() : TextBubble::Spec(typeid(Spec))
        {
            obj_type = TypeId::TextRect;
        }
        float kek;
    };

public:
    TextRect(GameWorld &world, const Spec &spec, int ent_id);

    // virtual void update(float dt) override;
    virtual void draw(LayersHolder& layers, Assets& assets) override;

private:
};
BOOST_DESCRIBE_STRUCT(TextRect::Spec, (TextBubble::Spec), ());

