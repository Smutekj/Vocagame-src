#pragma once

#include "Game.h"

struct Assets;

class WordGrid
{

    using CellId = std::pair<int, int>;
    struct PairHash
    {
        std::size_t operator()(const std::pair<int, int> &p) const noexcept
        {
            // good hash combining technique
            std::size_t h1 = std::hash<int>{}(p.first);
            std::size_t h2 = std::hash<int>{}(p.second);
            return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2)); // boost::hash_combine
        }
    };

public:
    WordGrid(utils::Vector2f grid_size) : m_grid_size(grid_size) {}

    bool containsEntity(const utils::Vector2f &pos) const
    {
        return m_cell2entity.contains(coord2cell(pos));
    }

    CellId coord2cell(const utils::Vector2f &coord) const
    {
        return {std::floor(coord.x / m_grid_size.x), std::floor(coord.y / m_grid_size.y)};
    }

    bool insert(int ent_id, const utils::Vector2f &pos)
    {
        if (!containsEntity(pos))
        {
            auto cell_id = coord2cell(pos);
            m_cell2entity[cell_id] = ent_id;
            m_entity2cell[ent_id] = cell_id;
            return true;
        }
        return false;
    }

    utils::Vector2f getCellCenter(const utils::Vector2f &pos) const
    {
        auto cell_coords = coord2cell(pos);
        return {(cell_coords.first + 0.5f) * m_grid_size.x, (cell_coords.second + 0.5f) * m_grid_size.y};
    }

    void remove(int ent_id)
    {
        m_cell2entity.erase(m_entity2cell.at(ent_id));
        m_entity2cell.erase(ent_id);
    }

private:
    utils::Vector2f m_grid_size;
    std::unordered_map<CellId, int, PairHash> m_cell2entity;
    std::unordered_map<int, CellId> m_entity2cell;
};

class RaceGame : public Game
{

public:
    RaceGame(Renderer &window, KeyBindings &bindings, Assets &asset);
    virtual ~RaceGame() override {}

    // virtual void updateImpl(const float dt, Renderer &win);
    // virtual void handleEventImpl(const SDL_Event &event){}
    virtual void drawImpl(Renderer &window) override;

    std::shared_ptr<GameLevelA> levelFactory(int level_id, utils::Vector2f start_pos);

    void registerCollisions();
    void registerSystems();
    void restartGame();
    void initializeUI();

    std::string generateNewSequence();
    WordRepresentation generateNewWord();

    std::unique_ptr<PostBox<NewEntity<TextBubble>>> m_on_text_create;
    std::deque<std::shared_ptr<GameLevelA>> m_levels;

    std::unique_ptr<PostBox<CharacterGuessEvent>> m_on_char_guess;

    utils::Vector2f m_grid_size;

    WordGrid m_word_grid;

    WordRepresentation m_right_word;
    std::size_t m_guessing_index = 0;
    std::size_t m_guess_seq_len = 3;
    std::string m_right_sequence;
    std::string m_found_sequence;

    Text m_right_word_ui_text;
    float m_right_word_scale = 1.f;
    float m_ui_header_height = 30.f;
};
