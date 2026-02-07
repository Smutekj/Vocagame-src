#pragma once

#include <set>
#include <unordered_map>
#include <queue>
#include <limits>

#include "core.h"



struct BVHNode
{
    AABB rect;
    int child_index_1 = -1;
    int child_index_2 = -1;
    int parent_index = -1;
    int object_index = -1;
    int height = 0;

    bool isLeaf() const
    {
        return child_index_1 == -1 && child_index_2 == -1;
    }
};

struct RayCastData
{
    int entity_ind;
    utils::Vector2f hit_point;
    utils::Vector2f hit_normal;
};

class BoundingVolumeTree
{

    std::vector<BVHNode> nodes;
    std::unordered_map<int, int> object2node_indices; //! mapping from objects to leaves
    std::set<int> free_indices;                       //! holds node indices that can be used whe inserting new rect
    int root_ind = -1;

public:
    const BVHNode &getNode(int node_index) const;

    void addRect(AABB rect, int object_index);

    void removeObject(int object_index);

    const auto &getObjects() const
    {
        return object2node_indices;
    }


    
    std::vector<std::pair<int, int>> findClosePairsWithin() const;
    std::vector<std::pair<int, int>> findClosePairsWith(BoundingVolumeTree &tree) const;
    std::vector<std::pair<int, int>> findClosePairsWith2(BoundingVolumeTree &tree) const;
    std::vector<int> findIntersectingLeaves(AABB rect) const;

    void clear();

    const AABB &getObjectRect(int object_ind) const
    {
        return nodes.at(object2node_indices.at(object_ind)).rect;
    }

    std::vector<int> rayCast(utils::Vector2f from, utils::Vector2f dir, float length);

    bool intersectsLine(utils::Vector2f from, utils::Vector2f to, AABB rect);
private:
    int maxBalanceFactor() const;
    bool isLeaf(int node_index) const;
    int balance(int index);
    void removeLeaf(int leaf_index);
    int findBestSibling(const AABB &new_rect);
    int findBestSiblingGreedy(const AABB &new_rect);
    bool containsCycle() const;
    bool isConsistent() const;
    int calcMaxDepth() const;
    void moveNodeUp(int going_up_index);
    void refitFrom(int node_index);
};


