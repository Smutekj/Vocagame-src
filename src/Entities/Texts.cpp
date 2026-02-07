#include "Texts.h"

#include <DrawLayer.h>
#include <Text.h>
#include <Sprite.h>

#include "CollisionSystem.h"
#include "GameWorld.h"
#include "SoundSystem.h"
#include "Player.h"
#include "VisualEffects.h"
#include "Animation.h"
#include "Utils/RandomTools.h"
#include "Assets.h"

void fitInside(utils::Vector2f rect_size, float aspect, Transform &transformable)
{
    utils::Vector2f old_scale = transformable.getScale();
    float box_aspect = rect_size.y / rect_size.x;
    if (box_aspect <= aspect)
    {
        //! fit y-dir
        transformable.setScale(rect_size.y / aspect / 2.f, rect_size.y / 2.f);
    }
    else
    {
        //! fit x-dir
        transformable.setScale(rect_size.x / 2.f, rect_size.x / 2.f * aspect);
    }
}

void TextBubble::setText(const std::string &text)
{
    m_text = text;
    m_correct_form = text;
    m_is_correct = true;
    m_drawable.setText(text);
}
const std::string &TextBubble::getText() const
{
    return m_text;
}

const std::string &TextBubble::getTranslation() const
{
    return m_translation;
}

bool TextBubble::firstTouch() const
{
    return first_touch;
}

void TextBubble::setImage(Sprite word_image)
{
    m_word_image = word_image;
    m_has_image = true;
}

void TextBubble::setTranslation(const std::string &trans)
{
    m_translation = trans;
}

void TextBubble::setCorrectForm(const std::string &text)
{
    m_correct_form = text;
    m_is_correct = m_correct_form == m_text;
}

void TextBubble::setFont(Font &font)
{
    m_font = &font;
    m_drawable.setFont(&font);
}

const std::string &TextBubble::getCorrectForm() const
{
    return m_correct_form;
}

bool TextBubble::isCorrect() const
{
    return m_is_correct;
}

TextBubble::TextBubble(GameWorld &world, const Spec &spec, int ent_id)
    : GameObject(&world, spec, ent_id, spec.obj_type), m_effects_maker(world)
{

    setText(spec.text);
    setTranslation(spec.translation);
    setCorrectForm(spec.correct_form);

    m_drawable.m_edge_color = spec.border_color;
    m_drawable.m_glow_color = spec.glow_color;
    m_drawable.setText(m_text);
    m_bottom_color = spec.bottom_color;
    m_top_color = spec.top_color;
        
    light_rect.setPosition(m_pos);
    light_rect.setScale(300, 300);

    CollisionComponent c_comp;
    c_comp.shape.convex_shapes.emplace_back(4);
    c_comp.type = spec.obj_type;

    m_world->m_systems.addEntityDelayed(getId(), c_comp);
    if (spec.path.stepsCount() > 0)
    {
        PathComponent p_comp;
        p_comp.path = spec.path;
        for (auto &step : p_comp.path.steps)
        {
            step.target += spec.pos;
        }
        p_comp.current_step = 0;
        // p_comp.speed = 50;
        m_world->m_systems.addEntityDelayed(getId(), p_comp);
    }
}

void TextBubble::onCreation()
{
}

void TextBubble::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (m_collision_resolvers.contains(obj.getType()))
    {
        m_collision_resolvers.at(obj.getType())(obj, c_data);
        return;
    }
    if (obj.getType() == ObjectType::Player)
    {
        if (firstTouch()) //! if this is first time player touched the thing
        {
            //! should be done by event listener
            if (isCorrect())
            {
                SoundSystem::play("UpShort");
                kill();
                return;
            }
            else
            {
                m_drawable.setColor({255, 0, 0, 255});
                SoundSystem::play("Explosion2", 100);
                m_effects_maker.create(EffectId::PurpleExplosion, m_pos, std::max(m_size.x, m_size.y) * 2.69, 0.69);
            }
        }
        m_player_is_standing = true;
    }
}

void TextBubble::update(float dt)
{

    m_timers.update(dt);

    if (m_pos.y < 0.f && m_vel.y < 0.f)
    {
        m_vel.y = std::abs(m_vel.y);
    }
    m_pos += m_vel * dt;
}

void TextBubble::setTextHeight(float height)
{
    m_size.y = height;
}

void TextBubble::draw(LayersHolder &layers, Assets &assets)
{

    setFont(*assets.fonts.at("Tahoma"));
    m_drawable.setFont(m_font);
    

    auto &canvas = layers.getCanvas("Text");
    //! set color if standing based on correctness
    if (m_player_is_standing)
    {
        m_drawable.m_glow_color = m_is_correct ? ColorByte{0, 255, 0, 255} : ColorByte{255, 0, 0, 255};
        m_is_correct ? m_drawable.setColor({255, 255, 255, 255}) : m_drawable.setColor({255, 255, 255, 255});
        
        auto c = m_drawable.m_glow_color;
        light_rect.m_color = {c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a / 255.f};
        light_rect.m_color = light_rect.m_color * (c.a / 255.f);
    }
    else
    {
        m_top_color.a = 255 * m_opacity;
        m_drawable.setColor(m_top_color);
    }
    m_drawable.m_edge_color.a = 255 * m_opacity;
    m_drawable.m_glow_color.a = 255 * m_opacity;

    if (layers.getCanvasP("Light"))
    {
        auto &light_canvas = layers.getCanvas("Light");

        light_rect.setPosition(m_pos);
        light_canvas.drawRectangle(light_rect, "Light");
    }

    if (m_has_image)
    {
        float aspect = m_word_image.m_tex_rect.height / (float)(m_word_image.m_tex_rect.width);
        m_word_image.setScale(m_size.x / 2.f, m_size.x * aspect / 2.f);
        fitInside(m_size, aspect, m_word_image);
        float real_y_size = m_word_image.getScale().y;
        //! position image so that upper left corner concides with collision box
        m_word_image.setPosition(m_pos.x, m_pos.y + m_size.y / 2.f - real_y_size);
        m_word_image.m_color.a = 255 * m_opacity;
        canvas.drawSprite(m_word_image, "fillBackground");

        auto bb = m_drawable.getBoundingBox();

        // m_size.x = bb.width * m_size.y / (float)bb.height;
        float text_aspect = bb.height / (float)bb.width;
        float size_x = bb.width * m_size.y / (float)bb.height / (float)bb.height;
        //! text is below the image
        float text_pos_y = m_word_image.getPosition().y - m_word_image.getScale().y - bb.height / 2.f - 10.f;
        if (m_player_is_standing && !m_is_correct)
        {
            m_drawable.setText(m_correct_form);
        }
        else
        {
            m_drawable.setText(m_text);
        }
        m_drawable.centerAround({m_pos.x, text_pos_y});
        canvas.drawText2(m_drawable);
        return;
    }

    m_drawable.setText(m_translation);
    m_drawable.setScale(1.f, 1.f);
    auto bb = m_drawable.getBoundingBox();

    m_size = {bb.width, bb.height};
    float aspect = (float)bb.height / (float)bb.width;

    m_drawable.centerAround(m_pos);
    // m_drawable.m_draw_bounding_box = true;
    canvas.drawText2(m_drawable);
    // m_drawable.m_draw_bounding_box = false;

    float text_height = bb.height;
    if (m_player_is_standing && !m_is_correct)
    {
        m_drawable.setText(m_correct_form);
    }
    else
    {
        m_drawable.setText(m_text);
    }
    m_drawable.setScale(1.f, 1.f);
    auto lower_bb = m_drawable.getBoundingBox();
    m_bottom_color.a = 255 * m_opacity;
    m_drawable.setColor(m_bottom_color);
    m_drawable.centerAround({m_pos.x, m_pos.y - m_font->getLineHeight() - 0 * (lower_bb.height / 2.f + bb.height / 2.f)});
    canvas.drawText2(m_drawable);
}

TextPlatform::TextPlatform(GameWorld &world, const Spec &spec, int ent_id)
    : TextBubble(world, spec, ent_id)
{
}

void TextPlatform::onCollisionWith(GameObject &obj, CollisionData &c_data)
{
    if (m_collision_resolvers.contains(obj.getType()))
    {
        m_collision_resolvers.at(obj.getType())(obj, c_data);
        return;
    }
    if (obj.getType() == ObjectType::Player)
    {

        auto surface_norm = c_data.separation_axis;
        auto rel_vel = obj.m_vel - m_vel;
        float v_in_norm = utils::dot(surface_norm, rel_vel);
        if (surface_norm.y > 0.f && v_in_norm <= 0.f)
        {
            if (firstTouch()) //! if this is first time player touched the thing
            {

                if (isCorrect())
                {
                    // auto &stars = m_world->addObject2<StarEmitter>();
                    // stars.setPosition(getPosition());
                }
                else
                {
                    static_cast<Player &>(obj).health--;
                    SoundSystem::play("WrongWord");
                }
            }
            // first_touch = false;
            m_player_is_standing = true;
        }
    }
}

TextRect::TextRect(GameWorld &world, const Spec &spec, int ent_id)
    : TextBubble(world, spec, ent_id)
{
}

void TextRect::draw(LayersHolder &layers, Assets &assets)
{
    setFont(*assets.fonts.begin()->second);
    m_drawable.setFont(m_font);

    auto &canvas = layers.getCanvas("Text");
    auto &bg_canvas = layers.getCanvas("Background");
    //! set color if standing based on correctness

    m_drawable.setColor({255, 255, 255, 255});

    if (m_has_image)
    {
        float aspect = m_word_image.m_tex_rect.height / (float)(m_word_image.m_tex_rect.width);
        m_word_image.setScale(m_size.x / 2.f, m_size.x * aspect / 2.f);
        float real_y_size = m_size.x * aspect / 2.f;
        //! position image so that upper left corner concides with collision box
        m_word_image.setPosition(m_pos.x, m_pos.y + m_size.y / 2.f - m_size.x * aspect / 2.f);
        canvas.drawSprite(m_word_image, "fillBackground");

        auto bb = m_drawable.getBoundingBox();

        if (m_player_is_standing && !m_is_correct)
        {
            m_drawable.setText(m_correct_form);
        }
        else
        {
            m_drawable.setText(m_text);
        }

        float text_aspect = bb.height / (float)bb.width;
        float size_x = bb.width * m_size.y / (float)bb.height / (float)bb.height;
        //! text is below the image
        float text_pos_y = m_word_image.getPosition().y - m_word_image.getScale().y - bb.height / 2.f - 10.f;
        m_drawable.centerAround({m_pos.x, text_pos_y});
        canvas.drawText2(m_drawable);
        return;
    }

    utils::Vector2f upper_size = {m_size.x, m_size.y * 1. / 2.};
    utils::Vector2f upper_center = {m_pos.x, m_pos.y + m_size.y / 2.f - upper_size.y / 2.f};
    utils::Vector2f lower_size = {m_size.x, m_size.y * 1. / 2.};
    utils::Vector2f lower_center = {m_pos.x, m_pos.y - m_size.y / 2.f + lower_size.y / 2.f};

    RectangleSimple r;
    r.setPosition(m_pos);
    r.setScale(m_size);
    r.m_color = {0., 0., 0., 1.0};
    bg_canvas.drawRectangle(r);

    m_drawable.setText(m_translation);
    m_drawable.setScale(1.f, 1.f);
    auto bb = m_drawable.getBoundingBox();

    // m_size = {bb.width, bb.height};
    float aspect = (float)bb.height / (float)bb.width;

    m_drawable.centerAround(upper_center);
    // m_drawable.m_draw_bounding_box = true;
    canvas.drawText2(m_drawable);

    float text_height = bb.height;
    if (m_player_is_standing && !m_is_correct)
    {
        m_drawable.setText(m_correct_form);
    }
    else
    {
        m_drawable.setText(m_text);
    }
    m_drawable.setScale(1.f, 1.f);
    auto lower_bb = m_drawable.getBoundingBox();
    m_drawable.m_edge_color = {255, 255, 0, 255};
    m_drawable.setColor({255, 255, 0, 255});
    m_drawable.centerAround({m_pos.x, lower_center.y});
    canvas.drawText2(m_drawable);
}