#pragma once

#include "Grid.h"

#include "PostOffice.h"
#include "PostBox.h"
#include "GameEvents.h"
#include "Utils/ContiguousColony.h"

class GameObject;


struct GridPosGenerator
{

    GridPosGenerator(PostOffice &messenger, utils::Vector2f grid_size, utils::Vector2f box_size);
    
    utils::Vector2f generateFreePos(GameObject& obj, utils::Vector2f player_pos);
    
    void removeCoveredCells(utils::Vector2f pos, utils::Vector2f size);
    void setBoxSize(utils::Vector2f box_size);
    void setCellSize(utils::Vector2f cell_size);

    private:
    std::unique_ptr<PostBox<EntityDiedEvent>> m_on_text_death;

    utils::Grid m_grid;
    std::unordered_map<std::size_t, GameObject *> m_grid2obj;
    std::unordered_map<int, std::size_t> m_obj2grid;

    utils::ContiguousColony<std::size_t, std::size_t> m_free_cells;
};
