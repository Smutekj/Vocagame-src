#pragma once

#include <cmath>
#include <cassert>

#include <Utils/Vector2.h>

struct AABB
{
    utils::Vector2f r_min = {0, 0};
    utils::Vector2f r_max = {0, 0};

    AABB() = default;
    AABB(utils::Vector2f r_min, utils::Vector2f r_max) : r_min(r_min), r_max(r_max)
    {
    }

    AABB(utils::Vector2f r_min, float width, float height) : r_min(r_min), r_max({r_min.x + width, r_min.y + height})
    {
    }

    AABB &inflate(float scale)
    {
        auto dr = (r_max - r_min);
        auto new_dr = dr * scale;
        auto overlap_dr = (new_dr - dr) / 2.f;
        r_min = r_min - overlap_dr;
        r_max = r_max + overlap_dr;
        return *this;
    }

    utils::Vector2f getCenter()const
    {
      return (r_min + r_max) / 2.f;
    }

    utils::Vector2f getSize()const
    {
      return (r_max - r_min);
    }

    bool isIn(const utils::Vector2f &r) const
    {
        return r.x <= r_max.x && r.x > r_min.x && r.y <= r_max.y && r.y > r_min.y;
    }

    float volume() const
    {
        return (r_max.x - r_min.x) * (r_max.y - r_min.y);
    }
};

inline AABB makeUnion(const AABB &r1, const AABB &r2)
{
    AABB r12;
    r12.r_min.x = std::min(r1.r_min.x, r2.r_min.x);
    r12.r_min.y = std::min(r1.r_min.y, r2.r_min.y);

    r12.r_max.x = std::max(r1.r_max.x, r2.r_max.x);
    r12.r_max.y = std::max(r1.r_max.y, r2.r_max.y);
    return r12;
}



bool inline intersects(const AABB &r1, const AABB &r2)
{
    bool intersects_x = r1.r_min.x <= r2.r_max.x && r1.r_max.x >= r2.r_min.x;
    bool intersects_y = r1.r_min.y <= r2.r_max.y && r1.r_max.y >= r2.r_min.y;
    return intersects_x && intersects_y;
}

struct Projection1D
{
  float min = std::numeric_limits<float>::max();
  float max = -std::numeric_limits<float>::max();
};

bool inline overlap1D(const Projection1D &p1, const Projection1D &p2)
{
  return p1.min <= p2.max && p2.min <= p1.max;
}

float inline calcOverlap(const Projection1D &p1, const Projection1D &p2)
{

  assert(overlap1D(p1, p2));
  return std::min(p1.max, p2.max) - std::max(p1.min, p2.min);
}

Projection1D inline projectOnAxis(utils::Vector2f t, const std::vector<utils::Vector2f> &points)
{

  Projection1D projection;
  for (auto &point : points)
  {
    auto proj = dot(t, point);
    projection.min = std::min(projection.min, proj);
    projection.max = std::max(projection.max, proj);
  }
  return projection;
}
