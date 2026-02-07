#pragma once

#include "Screen.h"
#include "../Entities/Specs.h"
#include "../Assets.h"
#include "DrawLayer.h"
#include "TimedEventManager.h"

#include <Text.h>

inline unsigned char getUTF8ByteCount(unsigned char b)
{
    if ((b & 0x80) == 0x00)
        return 1; // 0xxxxxxx
    if ((b & 0xE0) == 0xC0)
        return 2; // 110xxxxx
    if ((b & 0xF0) == 0xE0)
        return 3; // 1110xxxx
    if ((b & 0xF8) == 0xF0)
        return 4; // 11110xxx
    return 0;     // invalid
}

inline unsigned int getUTF8Code(const unsigned char *s)
{
    unsigned int c = 0;
    int count = getUTF8ByteCount(s[0]);

    if (count == 1)
        return s[0];

    if (count == 2)
        return ((s[0] & 0x1F) << 6) |
               (s[1] & 0x3F);

    if (count == 3)
        return ((s[0] & 0x0F) << 12) |
               ((s[1] & 0x3F) << 6) |
               (s[2] & 0x3F);

    if (count == 4)
        return ((s[0] & 0x07) << 18) |
               ((s[1] & 0x3F) << 12) |
               ((s[2] & 0x3F) << 6) |
               (s[3] & 0x3F);

    return 0; // invalid
}

struct UTF8String
{

    UTF8String(const std::string &text = "")
        : m_data(text)
    {
        std::size_t pos = 0;
        while (pos < text.size())
        {
            auto count = getUTF8ByteCount(*(text.c_str() + pos));
            pos += count;
            m_byte_counts.push_back(count);
        }
    }

    std::size_t size() const
    {
        return m_byte_counts.size();
    }

    const std::string &getString() const
    {
        return m_data;
    }

    void operator+=(const char *character_code)
    {
        push_back(character_code);
    }

    void push_back(const char *character_code)
    {
        auto byte_count = getUTF8ByteCount(character_code[0]);
        m_byte_counts.push_back(byte_count);
        std::string new_text = character_code;
        assert(new_text.size() == byte_count);
        m_data += character_code;
    }

    void erase(std::size_t start, std::size_t count)
    {
        std::size_t byte_start = std::accumulate(begin(m_byte_counts), begin(m_byte_counts) + start, 0);
        auto total_erased_byte_count = std::accumulate(begin(m_byte_counts) + start, begin(m_byte_counts) + start + count, 0);
        m_data.erase(byte_start, total_erased_byte_count);

        m_byte_counts.erase(begin(m_byte_counts) + start, begin(m_byte_counts) + start + count);
    }

    unsigned int getCode(std::size_t character_pos)
    {
        if(character_pos >= size())
        {
            return 0;
        }

        std::size_t code_pos = 0;
        std::size_t i = 0;
        while (i < character_pos)
        {
            code_pos += getUTF8ByteCount(*(m_data.c_str() + code_pos));
            i++;
        }

        return getUTF8Code((const unsigned char*)(m_data.c_str() + code_pos));
    }

private:
    std::string m_data;
    std::vector<unsigned char> m_byte_counts;
};

struct VocabularyQuestionUI : public Transform
{

    VocabularyQuestionUI(WordRepresentation &repre, Font &font);

    void handleEvent(const SDL_Event &event, utils::Vector2f mouse_pos);
    void update(float dt);
    void draw(LayersHolder &layers, TextureHolder2 &textures);

private:
    void drawBoundingBox(utils::Vector2f pos, utils::Vector2f scale, Renderer &window, Color color, float thickness = 2.f);
    void handleEventClick(const SDL_Event &event, utils::Vector2f mouse_pos);

private:
    bool is_focused = false;
    WordRepresentation m_repre;
    Font &m_font;
    TimedEventManager m_timers;
    float m_cursor_width = 0.f;
    Rectf m_textfield_bounds;
    
    utils::Vector2f top_rect_size;
    Text input_word;
    
    std::size_t m_cursor_pos = 0;
    UTF8String m_input;
};

class VocabularyExam : public Screen
{

public:
    VocabularyExam(Renderer &window, Assets &assets, std::vector<WordRepresentation> &word_repres);

    virtual void update(float dt) override;
    virtual void handleEvent(const SDL_Event &event) override;
    virtual void draw(Renderer &window) override;

private:
    LayersHolder m_layers;
    Renderer &m_window;
    TextureHolder2 &m_textures;

    std::vector<WordRepresentation> m_word_repres;
    VocabularyQuestionUI m_question;
};