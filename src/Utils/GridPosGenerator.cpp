#include "GridPosGenerator.h"

#include "Utils/RandomTools.h"
#include "GameObject.h"

GridPosGenerator::GridPosGenerator(PostOffice &messenger, utils::Vector2f grid_size, utils::Vector2f box_size)
    : m_grid({std::floor(box_size.x / grid_size.x), std::floor(box_size.y / grid_size.y)}, box_size)
{
    for (int i = 0; i < m_grid.getNCells(); ++i)
    {
        m_free_cells.insert(i, i);
    }

    m_on_text_death = std::make_unique<PostBox<EntityDiedEvent>>(messenger, [this](const auto &events)
                                                                 {
        for(const auto& e : events)
        {
            if(m_obj2grid.contains(e.id))
            {
                auto grid_id = m_obj2grid.at(e.id);
                m_free_cells.insert(grid_id, grid_id);
                m_grid2obj.erase(grid_id);
                m_obj2grid.erase(e.id);
            }
        } });
}

utils::Vector2f GridPosGenerator::generateFreePos(GameObject &obj, utils::Vector2f player_pos)
{
    std::size_t free_cells_count = m_free_cells.size();
    if (free_cells_count == 0)
    {
        return utils::randomPosInBox({0, 0}, m_grid.getSize());
    }


    utils::Vector2f cell_pos;
    std::size_t rand_i;
    std::size_t cell_id;
    do
    {
        rand_i = utils::randi(0, free_cells_count - 1);
        cell_id = m_free_cells.data.at(rand_i);
        cell_pos = m_grid.cellCenter(cell_id);
    } while (utils::dist(player_pos, cell_pos) < 100);

    auto half_cell_size = m_grid.m_cell_size / 2.f;
    auto rand_pos = cell_pos + utils::Vector2f{utils::randf(0.f, half_cell_size.x / 2.f),
                                               utils::randf(0.f, half_cell_size.y / 2.f)};

    m_free_cells.erase(cell_id);

    obj.setPosition(rand_pos);
    m_obj2grid[obj.getId()] = cell_id;
    return rand_pos;
}

void GridPosGenerator::removeCoveredCells(utils::Vector2f pos, utils::Vector2f size)
{
    Rectf covering_rect = {pos.x - size.x / 2.f, pos.y - size.y / 2.f, size.x, size.y};

    //! do dfs on cell grid and remove covered cells
    auto cell_id = m_grid.coordToCell(pos);
    std::unordered_set<std::size_t> visited;
    std::vector<std::size_t> to_visit = {cell_id};
    while (!to_visit.empty())
    {
        auto current = to_visit.back();
        to_visit.pop_back();

        if(visited.contains(current))
        {
            continue;;
        }
        visited.insert(current);
        
        if(m_free_cells.contains(cell_id))
        {
            m_free_cells.erase(cell_id);
        }

        auto center = m_grid.cellCenter(cell_id);
        Vec2 left = {center.x + m_grid.m_cell_size.x, center.y};
        Vec2 right = {center.x - m_grid.m_cell_size.x, center.y};
        Vec2 top = {center.x, center.y + m_grid.m_cell_size.y};
        Vec2 down = {center.x, center.y - m_grid.m_cell_size.y};
        
        if (m_free_cells.contains(m_grid.coordToCell(left)) && covering_rect.contains(left))
        {
            to_visit.push_back(m_grid.coordToCell(left));
        }
        if (m_free_cells.contains(m_grid.coordToCell(right)) && covering_rect.contains(right))
        {
            to_visit.push_back(m_grid.coordToCell(right));
        }
        if (m_free_cells.contains(m_grid.coordToCell(top)) && covering_rect.contains(top))
        {
            to_visit.push_back(m_grid.coordToCell(top));
        }
        if (m_free_cells.contains(m_grid.coordToCell(down)) && covering_rect.contains(down))
        {
            to_visit.push_back(m_grid.coordToCell(down));
        }
    }
}
void GridPosGenerator::setBoxSize(utils::Vector2f box_size)
{
    m_grid.setBoxSize(box_size);
}
void GridPosGenerator::setCellSize(utils::Vector2f cell_size)
{
    m_grid.setCellSize(cell_size);
}