#pragma once

#include <Utils/Vector2.h>
#include <Transform.h>
#include <Renderer.h>

#include <vector>

#include "core.h"

struct Polygon : public Transform
{
  std::vector<utils::Vector2f> points;

  Polygon(int n_points = 3, utils::Vector2f at = {0, 0});

  AABB getBoundingRect() const
  {
    auto r = getPosition();
    auto dr = utils::Vector2f{std::max(getScale().x, getScale().y)};
    return {r - dr, r + dr};
  }

  utils::Vector2f getCenter()
  {
    return getPosition();
  }

  std::vector<utils::Vector2f> getPointsInWorld() const;
  void move(utils::Vector2f by);
  void rotate(float by);
  void update(float dt);

  bool isCircle() const
  {
    return points.size() < 3;
  }
  utils::Vector2f getMVTOfSphere(utils::Vector2f center, float radius);
};

void inline drawShape(Renderer &canvas, const Polygon &shape)
{
  auto n_points = shape.points.size();
  auto points = shape.getPointsInWorld();
  for (int i = 0; i < n_points; ++i)
  {
    canvas.drawLineBatched(points[i], points[(i + 1) % n_points], 0.25, {0, 1., 0., 1.});
  }
}
