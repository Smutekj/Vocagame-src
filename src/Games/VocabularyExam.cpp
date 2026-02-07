#include "VocabularyExam.h"

#include "DrawLayer.h"

VocabularyQuestionUI::VocabularyQuestionUI(WordRepresentation &repre, Font &font)
    : m_font(font), m_repre(repre)
{
}

void VocabularyQuestionUI::handleEvent(const SDL_Event &event, utils::Vector2f mouse_pos)
{
    if (event.type == SDL_MOUSEBUTTONDOWN)
    {
        handleEventClick(event, mouse_pos);
    }
    else if (event.type == SDL_TEXTINPUT)
    {
        if (!m_font.containsUTF8Code(getUTF8Code((const unsigned char *)event.text.text)))
        {
            return;
        }
        m_input += event.text.text;
        m_cursor_pos++;
    }
    else if (event.type == SDL_KEYDOWN)
    {
        auto key = event.key.keysym.sym;
        if (key == SDLK_BACKSPACE && m_cursor_pos >= 1 && m_cursor_pos <= m_input.size())
        {
            m_input.erase(m_cursor_pos - 1, 1);
            m_cursor_pos--;
        }
        else if (key == SDLK_DELETE && m_cursor_pos + 1 <= m_input.size())
        {
            m_input.erase(m_cursor_pos, 1);
        }
        if (key == SDLK_RETURN || key == SDLK_RETURN2)
        {
        }
    }
}

void VocabularyQuestionUI::handleEventClick(const SDL_Event &event, utils::Vector2f mouse_pos)
{
    bool received_focus = false;
    bool mouse_in_input = m_textfield_bounds.contains(mouse_pos);

    if (mouse_in_input && !is_focused)
    {
        is_focused = true;
        received_focus = true;
    }
    else if (is_focused && !mouse_in_input)
    {
        is_focused = false;
        m_timers.clear();
    }
    //! change cursor position and make it blink
    if (is_focused)
    {
        m_cursor_pos = input_word.getCursor(mouse_pos.x);
    }
    if (received_focus)
    {
        m_timers.addInfiniteEvent([this](float t, int c)
                                  { m_cursor_width = 1.f; }, 1.f, 0.f);
        m_timers.addInfiniteEvent([this](float t, int c)
                                  { m_cursor_width = 0.f; }, 1.f, 0.6);
    }
}

void VocabularyQuestionUI::update(float dt)
{
    utils::Vector2f texfield_size = getScale() * 1.8;
    m_textfield_bounds = {
        getPosition().x - texfield_size.x / 2.f,
        getPosition().y - texfield_size.y / 2.f,
        texfield_size.x,
        texfield_size.y};

    m_timers.update(dt);
}

void VocabularyQuestionUI::drawBoundingBox(utils::Vector2f pos, utils::Vector2f scale, Renderer &window, Color color, float thickness)
{
    window.drawLineBatched({pos.x + scale.x, pos.y + scale.y}, {pos.x + scale.x, pos.y - scale.y}, thickness, color);
    window.drawLineBatched({pos.x + scale.x, pos.y - scale.y}, {pos.x - scale.x, pos.y - scale.y}, thickness, color);
    window.drawLineBatched({pos.x - scale.x, pos.y - scale.y}, {pos.x - scale.x, pos.y + scale.y}, thickness, color);
    window.drawLineBatched({pos.x - scale.x, pos.y + scale.y}, {pos.x + scale.x, pos.y + scale.y}, thickness, color);
}
void VocabularyQuestionUI::draw(LayersHolder &layers, TextureHolder2 &textures)
{

    Text target_word(m_repre.correct_form);
    target_word.setFont(&m_font);
    target_word.setPosition(getPosition());
    target_word.m_is_centered = true;
    auto bb = target_word.getBoundingBox();
    layers.getCanvas("Text").drawText(target_word);

    float bound_thickness = is_focused ? 3.f : 1.5f;
    utils::Vector2f input_rect_size = {getScale().x * 1.8, m_font.getLineHeight()};
    utils::Vector2f input_rect_pos = {getPosition().x, target_word.getPosition().y - 2 * input_rect_size.y / 2.f};
    drawBoundingBox(input_rect_pos, input_rect_size / 2.f, layers.getCanvas("Background"), {1, 1, 1, 1}, bound_thickness);

    input_word.setText(m_input.getString());
    input_word.setFont(&m_font);
    bb = input_word.getBoundingBox();
    float line_depth = input_word.getDepthUnderLine();
    input_word.setPosition(getPosition().x - getScale().x * 0.9 + 10, input_rect_pos.y - input_rect_size.y / 2.f + 0.2 * m_font.getLineHeight());
    layers.getCanvas("Text").drawText(input_word);

    if (is_focused)
    {
        //! draw cursor
        float cursor_x = input_word.getCursorPosition(m_cursor_pos);
        Vec2 cursor_low = {cursor_x, input_rect_pos.y - input_rect_size.y / 4.f};
        Vec2 cursor_high = {cursor_x, input_rect_pos.y + input_rect_size.y / 4.f};
        layers.getCanvas("Text").drawLineBatched(cursor_low, cursor_high, m_cursor_width, {1, 1, 1, 1});
    }
}

VocabularyExam::VocabularyExam(Renderer &window, Assets &assets, std::vector<WordRepresentation> &word_repres)
    : m_word_repres(word_repres),
      m_window(window),
      m_question(word_repres.at(0), *assets.fonts.begin()->second),
      m_textures(assets.atlases)
{
    TextureOptions text_options;
    text_options.data_type = TextureDataTypes::UByte;
    text_options.format = TextureFormat::RGBA;
    text_options.internal_format = TextureFormat::RGBA;
    text_options.mag_param = TexMappingParam::Linear;
    text_options.min_param = TexMappingParam::Linear;

    m_layers.addLayer("Text", 3, text_options, window.getTargetSize().x, window.getTargetSize().y);
    auto &bg_layer = m_layers.addLayer("Background", 2, text_options, window.getTargetSize().x, window.getTargetSize().y);
}

void VocabularyExam::update(float dt)
{
    m_question.update(dt);
}

void VocabularyExam::handleEvent(const SDL_Event &event)
{
    m_question.handleEvent(event, m_window.getMouseInWorld());

}

void VocabularyExam::draw(Renderer &window)
{
    Vec2 win_size = window.getTargetSize();
    m_question.setPosition(win_size / 2.f);
    m_question.setScale(win_size / 8.f);

    m_question.draw(m_layers, m_textures);

    m_layers.setView(window.getDefaultView());
    window.m_view = window.getDefaultView();
    m_layers.drawInto(window);
}