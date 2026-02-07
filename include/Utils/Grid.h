#pragma once

#include <Utils/Vector2.h>
#include <array>

namespace utils
{

  class Grid
  {

  public:
    utils::Vector2i m_cell_count = {1,1};
    utils::Vector2f m_cell_size = {1.f,1.f};

  public:
    Grid() = default;
    Grid(utils::Vector2i n_cells, utils::Vector2f box_size);

    void setCellCount(utils::Vector2i cell_count);
    void setCellSize(utils::Vector2i cell_size);
    void setBoxSize(utils::Vector2f box_size);

    [[nodiscard]] size_t coordToCell(float x, float y) const;
    [[nodiscard]] size_t coordToCell(utils::Vector2f r) const;
    // [[nodiscard]] size_t coordToCell(utils::Vector2i r_coord) const;
    [[nodiscard]] size_t cellIndex(int ix, int iy) const;
    [[nodiscard]] size_t cellIndex(utils::Vector2i) const;

    [[nodiscard]] size_t cellCoordX(size_t cell_index) const;
    [[nodiscard]] size_t cellCoordY(size_t cell_index) const;

    [[nodiscard]] size_t cellCoordX(utils::Vector2f r_coord) const;
    [[nodiscard]] size_t cellCoordY(utils::Vector2f r_coord) const;
    [[nodiscard]] utils::Vector2f cellCenter(std::size_t cell_index) const;

    [[nodiscard]] utils::Vector2i cellCoords(utils::Vector2f r_coord) const;
    [[nodiscard]] utils::Vector2i cellCoords(utils::Vector2i r_coord) const;
    [[nodiscard]] utils::Vector2i cellCoords(size_t cell_index) const;

    utils::Vector2f getSize() const;
    float getSizeX() const;
    float getSizeY() const;

    size_t getNCells() const;
  };

  //! \class represents grids that are used for searching for nearest neighbours
  class SearchGrid : public Grid
  {

  public:
    SearchGrid(utils::Vector2i n_cells, utils::Vector2f cell_size);
    bool isInGrid(utils::Vector2i cell_coords) const;

    void calcNearestCells(const int cell_ind, std::array<int, 9> &nearest_neighbours, int &n_nearest_cells) const;
    void calcNearestCells2(const int cell_ind, std::array<int, 9> &nearest_neighbours, int &n_nearest_cells) const;
  };

} // namespace utils;