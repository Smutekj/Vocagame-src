

#include <cmath>
#include <cassert>

#include "Grid.h"

namespace utils{

//! \brief computes cell_index of the cell containing given point
//! \param x 
//! \param y
//! \returns cell index
size_t Grid::coordToCell(const float x, const float y) const { return coordToCell({x, y}); }

//! \brief computes cell_index of the cell containing given point
//! \param r_coords given point 
//! \returns cell index
size_t Grid::coordToCell(const utils::Vector2f r_coord) const {
    const size_t ix = static_cast<size_t>(std::floor(r_coord.x / m_cell_size.x)) % m_cell_count.x;
    const size_t iy = static_cast<size_t>(std::floor(r_coord.y / m_cell_size.y)) % m_cell_count.y;

    assert(ix + iy * m_cell_count.x < m_cell_count.x * m_cell_count.y);
    return ix + iy * m_cell_count.x;
}

//! \brief computes cell_index of the cell corresponding to given 2D cell coordinates
//! \param ix
//! \param iy 
//! \returns cell index
[[nodiscard]] size_t Grid::cellIndex(const int ix, const int iy) const { return cellIndex({ix, iy}); }

//! \brief computes cell_index of the cell corresponding to given 2D cell coordinates
//! \param ixy given 2D cell coordinates
//! \returns cell index
[[nodiscard]] size_t Grid::cellIndex(utils::Vector2i ixy) const { return ixy.x + ixy.y * m_cell_count.x; }

//! \brief computes the x component of 2D cell coordinates 
//! \param cell_index
//! \returns x component of 2D cell coordinates
size_t Grid::cellCoordX(const size_t cell_index) const { return cell_index % m_cell_count.x; }

//! \brief computes the y component of 2D cell coordinates 
//! \param cell_index
//! \returns y component of 2D cell coordinates
size_t Grid::cellCoordY(const size_t cell_index) const { return (cell_index / m_cell_count.x) % m_cell_count.y; }

//! \brief computes the 2D cell coordinates of a given cell 
//! \param cell_index
//! \returns 2D cell coordinates
utils::Vector2i Grid::cellCoords(const size_t cell_index) const {
    return {static_cast<int>(cellCoordX(cell_index)), static_cast<int>(cellCoordY(cell_index))};
}

//! \brief computes the 2D cell coordinates of cell containing the given point 
//! \param r_coord given point
//! \returns x component of 2D cell coordinates
size_t Grid::cellCoordX(const utils::Vector2f r_coord) const { return static_cast<size_t>(r_coord.x / m_cell_size.x); }

//! \brief computes the 2D cell coordinates of cell containing the given point 
//! \param r_coord given point
//! \returns y component of 2D cell coordinates
size_t Grid::cellCoordY(const utils::Vector2f r_coord) const { return static_cast<size_t>(r_coord.y / m_cell_size.y); }

//! \brief computes the 2D cell coordinates of cell containing the given point 
//! \param r_coord given point
//! \returns 2D cell coordinates
utils::Vector2i Grid::cellCoords(const utils::Vector2f r_coord) const {
    const auto cell_coord_x = static_cast<int>(r_coord.x / m_cell_size.x);
    const auto cell_coord_y = static_cast<int>(r_coord.y / m_cell_size.y);
    return {cell_coord_x, cell_coord_y};
}

//! \brief computes the 2D cell coordinates of cell containing the given point 
//! \param r_coord given point
//! \returns 2D cell coordinates
utils::Vector2i Grid::cellCoords(const utils::Vector2i r_coord) const {
    const auto cell_coord_x = static_cast<int>(r_coord.x / m_cell_size.x);
    const auto cell_coord_y = static_cast<int>(r_coord.y / m_cell_size.y);
    return {cell_coord_x, cell_coord_y};}


Grid::Grid(utils::Vector2i n_cells, utils::Vector2f box_size)
    : m_cell_count(n_cells)
    , m_cell_size(box_size.x/n_cells.x, box_size.y/n_cells.y) {}


utils::Vector2f Grid::cellCenter(std::size_t cell_index) const
{
    auto cell_coords = cellCoords(cell_index);
    return utils::Vector2f{(cell_coords.x+0.5)*m_cell_size.x,(cell_coords.y+0.5)*m_cell_size.y}; 
}

//! \brief computes total number of cells in grid;
//! \returns total number of cells in the grid
size_t Grid::getNCells() const{
    return m_cell_count.x*m_cell_count.y;
}

//! \returns size of the grid in the cell_size units
utils::Vector2f Grid::getSize() const{
    return {m_cell_count.x*m_cell_size.x, m_cell_count.y*m_cell_size.y};
}
//! \returns size of the grid in the cell_size units
float Grid::getSizeX() const{
    return m_cell_count.x*m_cell_size.x;
}
//! \returns size of the grid in the cell_size units
float Grid::getSizeY() const{
    return  m_cell_count.y*m_cell_size.y;
}


void Grid::setCellCount(utils::Vector2i cell_count)
{
    auto box_size = getSize();
    m_cell_count = cell_count;
    m_cell_size = {box_size.x/m_cell_count.x, box_size.y/m_cell_count.y};
}
void Grid::setCellSize(utils::Vector2i cell_size)
{
    m_cell_size = cell_size;
}
void Grid::setBoxSize(utils::Vector2f box_size)
{   
    m_cell_size = {box_size.x/m_cell_count.x, box_size.y/m_cell_count.y};
}



//! \param cell_coords 2D cell coordinates
//! \returns true if point is within grid bounds
bool SearchGrid::isInGrid(utils::Vector2i cell_coords) const {
    return cell_coords.x < m_cell_count.x && cell_coords.x >= 0 && cell_coords.x < m_cell_count.y && cell_coords.y >= 0;
}


SearchGrid::SearchGrid(utils::Vector2i n_cells, utils::Vector2f cell_size)
    : Grid(n_cells, cell_size) {
}

//! \brief calculates indices of (up to) 9 closest cells EXCLUDING CENTER CELL, takes boundary into account, 
//! \param cell_ind
//! \param nearest_cells array containing cell_indices of closest cells (right now only looking at 9 nearest cells)
//! \param n_nearest_cells number of nearest cells
void SearchGrid::calcNearestCells(const int cell_ind, std::array<int, 9>& nearest_cells, int& n_nearest_cells) const {

    const auto cell_coords = cellCoords(cell_ind);
    n_nearest_cells = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            auto neighbour_cell_ind = cell_ind + i + j * m_cell_count.x;
            if (isInGrid({cell_coords.x + i, cell_coords.y + j}) && (neighbour_cell_ind != cell_ind)) {
                nearest_cells.at(n_nearest_cells) = neighbour_cell_ind;
                n_nearest_cells++;
            }
        }
    }
    assert(n_nearest_cells != 0);
}

//! \brief calculates cell indices of (up to) 9 closest cells EXCLUDING CENTER CELL, 
//! \brief will add only neighbour cell indices whose value is larger than \p cell_ind  
//! \brief (this is useful to prevent double counting)
//! \param cell_ind
//! \param nearest_cells array containing cell_indices of closest cells (right now only looking at 9 nearest cells)
//! \param n_nearest_cells number of nearest cells
void SearchGrid::calcNearestCells2(const int cell_ind, std::array<int, 9>& nearest_cells, int& n_nearest_cells) const {

    const auto cell_coords = cellCoords(cell_ind);
    n_nearest_cells = 0;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            auto neighbour_cell_ind = cell_ind + i + j * m_cell_count.x;
            if (isInGrid({cell_coords.x + i, cell_coords.y + j}) && (neighbour_cell_ind > cell_ind)) {
                nearest_cells.at(n_nearest_cells) = neighbour_cell_ind;
                n_nearest_cells++;
            }
        }
    }
    assert(n_nearest_cells != 0); //! this would be silly
}




} // namespace utils