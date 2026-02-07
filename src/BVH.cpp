#include "BVH.h"

#include <stack>
#include <queue>

const BVHNode &BoundingVolumeTree::getNode(int node_index) const
{
    return nodes.at(node_index);
}

//! \brief adds new leaf node holding object with index \p object_index
//! \brief which is bound by rectangle \p rect
//! \brief the object with that index must not exist in the tree!
//! \brief finds suitable place for the new node so as to minimize tree volume increase
//! \brief (unless tree is empty) creates 2 new nodes (one leaf for the object and one internal)
//! \brief in the new tree each internal node has exactly two children!
void BoundingVolumeTree::addRect(AABB rect, int object_index)
{
    assert(object2node_indices.count(object_index) == 0);

    //! we ran out of free node spots, so we increase node spots
    if (free_indices.empty())
    {
        for (int i = nodes.size(); i < 2 * nodes.size() + 1; ++i)
        {
            free_indices.insert(i);
        }
        nodes.resize(2 * nodes.size() + 1);
    }

    int new_parent = *free_indices.begin();
    free_indices.erase(free_indices.begin());

    if (object2node_indices.empty()) //! tree is empty
    {
        root_ind = 0;
        object2node_indices[object_index] = root_ind;
        nodes[new_parent] = {rect, -1, -1, -1, object_index};
        return;
    }

    assert(!containsCycle());
    assert(isConsistent());

    int new_leaf = *free_indices.begin();
    free_indices.erase(free_indices.begin());
    assert(object2node_indices.count(object_index) == 0);
    object2node_indices[object_index] = new_leaf;

    // nodes.push_back(new_node);

    //! find best sibling
    int best_index = findBestSiblingGreedy(rect);

    int old_parent = nodes.at(best_index).parent_index;
    nodes.at(new_parent).height = nodes.at(best_index).height + 1;
    nodes.at(new_parent).child_index_1 = best_index;
    nodes.at(new_parent).child_index_2 = new_leaf;
    nodes.at(new_parent).parent_index = old_parent;

    //! fix old parents children
    if (old_parent != -1) //! not a root
    {
        if (nodes.at(old_parent).child_index_1 == best_index)
        {
            nodes.at(old_parent).child_index_1 = new_parent;
        }
        else
        {
            nodes.at(old_parent).child_index_2 = new_parent;
        }
    }
    else
    { //! best sibling was a root
        root_ind = new_parent;
    }

    nodes.at(best_index).parent_index = new_parent;
    nodes.at(new_leaf).parent_index = new_parent;
    nodes.at(new_leaf).rect = rect;
    nodes.at(new_leaf).object_index = object_index;
    nodes.at(new_parent).rect = makeUnion(nodes.at(best_index).rect, rect);

    //! refit bounding volumes
    refitFrom(nodes.at(new_leaf).parent_index);

    assert(!containsCycle());
    assert(isConsistent());
    //! profit
}

//! \brief removes object with \p object_index from the tree
//! \brief the object must be present in the tree!
void BoundingVolumeTree::removeObject(int object_index)
{
    assert(object2node_indices.count(object_index) > 0);

    auto leaf_index = object2node_indices.at(object_index);

    removeLeaf(leaf_index);
    object2node_indices.erase(object_index);
    assert(!containsCycle());
    assert(isConsistent());
    // std::cout << "max balance is: " << calcMaxDepth() << "\n";
}

//! \brief removes leaf with node index \p leaf index
//! \brief does necessary bookeeping and refits the bounding volumes
void BoundingVolumeTree::removeLeaf(int leaf_index)
{

    assert(nodes.at(leaf_index).isLeaf());
    if (leaf_index == root_ind) //! there is just one leaf thus it is root
    {
        free_indices.insert(leaf_index);
        root_ind = -1;
        return;
    }

    const auto &removed_leaf_node = nodes.at(leaf_index);
    const auto removed_internal_index = removed_leaf_node.parent_index;
    const auto &removed_internal_node = nodes.at(removed_internal_index);

    bool is_left = true;
    auto sibling_index = removed_internal_node.child_index_2;
    if (leaf_index == removed_internal_node.child_index_2)
    {
        is_left = false;
        sibling_index = removed_internal_node.child_index_1;
    }
    auto &sibling_node = nodes.at(sibling_index);

    if (removed_internal_node.parent_index != -1)
    {
        sibling_node.parent_index = removed_internal_node.parent_index;

        auto &internal_parent = nodes.at(removed_internal_node.parent_index);

        if (internal_parent.child_index_1 == removed_internal_index)
        {
            internal_parent.child_index_1 = sibling_index;
        }
        else
        {
            internal_parent.child_index_2 = sibling_index;
        }
        const auto &child1 = nodes.at(internal_parent.child_index_1);
        const auto &child2 = nodes.at(internal_parent.child_index_2);
        internal_parent.rect = makeUnion(child1.rect, child2.rect);
        internal_parent.height = 1 + std::max(child1.height, child2.height);
    }
    else
    { //! parent of leaf is root  -> the sibling is the new root
        sibling_node.parent_index = -1;
        root_ind = sibling_index;
    }

    //! deactivate removed node
    free_indices.insert(leaf_index);
    free_indices.insert(removed_internal_index);
    nodes.at(leaf_index).child_index_1 = -1;
    nodes.at(leaf_index).child_index_2 = -1;
    nodes.at(leaf_index).parent_index = -1;
    nodes.at(leaf_index).height = 0;
    if (removed_internal_index != -1)
    {
        nodes.at(removed_internal_index).child_index_1 = -1;
        nodes.at(removed_internal_index).child_index_2 = -1;
        nodes.at(removed_internal_index).parent_index = -1; //! refit volumes for ancenstors
        nodes.at(removed_internal_index).height = 0;
    }

    refitFrom(sibling_node.parent_index);
}

//! \brief refits bounding volumes of all parent nodes nodes
//! \brief  from \p starting_node_index towards root
void BoundingVolumeTree::refitFrom(int starting_node_index)
{
    int current_index = starting_node_index;
    while (current_index != -1)
    {
        current_index = balance(current_index);

        auto &current_node = nodes.at(current_index);
        const auto &child_1 = nodes.at(current_node.child_index_1);
        const auto &child_2 = nodes.at(current_node.child_index_2);
        current_node.rect = makeUnion(child_1.rect, child_2.rect);
        current_node.height = 1 + std::max(child_1.height, child_2.height);

        current_index = nodes.at(current_index).parent_index;
    }
}

//! \brief finds best sibling for the new_rect
//! \brief so as to minimize increase in tree volume upon addition
//! \brief should find the best option and thus makes higher quality trees but slower
int BoundingVolumeTree::findBestSibling(const AABB &new_rect)
{
    auto calcCostChange = [&new_rect, this](int node_index)
    {
        return makeUnion(nodes.at(node_index).rect, new_rect).volume() - nodes.at(node_index).rect.volume();
    };

    int best_index = root_ind;
    auto current_index = root_ind;
    float insertion_cost = makeUnion(nodes.at(root_ind).rect, new_rect).volume();

    std::vector<std::pair<int, float>> to_visit;
    std::priority_queue pq(to_visit.begin(), to_visit.end(), [](const auto &p1, const auto &p2)
                           { return p1.second < p2.second; });

    pq.push({root_ind, 0});
    float best_cost = insertion_cost;

    while (!pq.empty())
    {
        auto current_index = pq.top().first;
        auto cumulated_cost = pq.top().second;
        pq.pop();
        auto current_cost = makeUnion(nodes.at(current_index).rect, new_rect).volume();

        auto child1 = nodes.at(current_index).child_index_1;
        auto child2 = nodes.at(current_index).child_index_2;

        //! cost of changing current node
        auto cost_of_exchange = current_cost + cumulated_cost;

        cumulated_cost += current_cost - nodes.at(current_index).rect.volume();

        if (cost_of_exchange < best_cost)
        {
            best_cost = cost_of_exchange;
            best_index = current_index;
        }
        auto cost_lower_bound = new_rect.volume() + cumulated_cost;
        if (cost_lower_bound < best_cost)
        {
            if (child1 != -1)
            {
                pq.push({child1, cumulated_cost});
            }
            if (child2 != -1)
            {
                pq.push({child2, cumulated_cost});
            }
        }
    }
    return best_index;
}

//! \brief tries to find best sibling for the new_rect
//! \brief so as to minimize increase in tree volume upon addition
//! \brief does not necessarily find the best option, but is faster and generally good enough
int BoundingVolumeTree::findBestSiblingGreedy(const AABB &new_rect)
{
    auto calcCostChange = [&new_rect, this](int node_index)
    {
        return makeUnion(nodes.at(node_index).rect, new_rect).volume() - nodes.at(node_index).rect.volume();
    };

    int current_index = root_ind;
    while (!nodes.at(current_index).isLeaf())
    {
        auto current_cost = makeUnion(nodes.at(current_index).rect, new_rect).volume();
        auto combined_vol = makeUnion(nodes.at(current_index).rect, new_rect).volume();
        float vol = nodes.at(current_index).rect.volume();
        float cumulated_cost = (combined_vol - vol);
        float cost = combined_vol;

        auto child1 = nodes.at(current_index).child_index_1;
        auto child2 = nodes.at(current_index).child_index_2;

        //! cost of changing current node
        auto cost_of_exchange = current_cost + cumulated_cost;

        float cost_1;
        auto new_vol = makeUnion(nodes.at(child1).rect, new_rect).volume();
        if (!nodes.at(child1).isLeaf())
        {
            cost_1 = new_vol + cumulated_cost;
        }
        else
        {
            auto old_vol = nodes.at(child1).rect.volume();
            cost_1 = new_vol - old_vol + cumulated_cost;
        }

        float cost_2;
        new_vol = makeUnion(nodes.at(child2).rect, new_rect).volume();
        if (!nodes.at(child2).isLeaf())
        {
            cost_2 = new_vol + cumulated_cost;
        }
        else
        {
            auto old_vol = nodes.at(child2).rect.volume();
            cost_2 = new_vol - old_vol + cumulated_cost;
        }
        if (cost < cost_1 && cost < cost_2)
        {
            break;
        }
        if (cost_1 < cost_2)
        {
            current_index = child1;
        }
        else
        {
            current_index = child2;
        }
    }
    return current_index;
}

bool BoundingVolumeTree::isLeaf(int node_index) const
{
    return nodes.at(node_index).child_index_1 == -1 && nodes.at(node_index).child_index_2 == -1;
}

//! \brief checks if tree contains cycles thus not being a tree (good for debugging)
bool BoundingVolumeTree::containsCycle() const
{
    if(root_ind == -1)
    {
        return false; //! no cycles in empty trees
    }
    std::vector<bool> visited(nodes.size(), false);
    std::stack<int> to_visit;
    to_visit.push(root_ind);
    while (!to_visit.empty())
    {
        auto current = to_visit.top();
        to_visit.pop();
        if (visited[current])
        {
            return true;
        }
        visited[current] = true;
        if (nodes.at(current).child_index_1 != -1)
        {
            to_visit.push(nodes.at(current).child_index_1);
            to_visit.push(nodes.at(current).child_index_2);
        }
    }
    return false;
}

//! \brief for every node checks if the child of the node has parent that is the node
bool BoundingVolumeTree::isConsistent() const
{
    if(root_ind == -1)
    {
        return true;
    }
    std::vector<bool> visited(nodes.size(), false);
    std::stack<int> to_visit;
    to_visit.push(root_ind);

    for (auto &[entity_ind, node_ind] : object2node_indices)
    {
        if (nodes.at(node_ind).child_index_1 != -1)
        {
            return node_ind != nodes.at(nodes.at(node_ind).child_index_1).parent_index;
        }
        if (nodes.at(node_ind).child_index_2 != -1)
        {
            return node_ind != nodes.at(nodes.at(node_ind).child_index_2).parent_index;
        }

        if (nodes.at(node_ind).parent_index == -1 && root_ind != node_ind)
        {
            return false;
        }
    }

    return true;
}

//! \brief does tree rotation on subtree at node_index \p index_a
//! \brief in order to make it more balanced
//! \returns index of the node that moved at position of node at \p index_a
int BoundingVolumeTree::balance(int index_a)
{

    assert(index_a != -1);

    auto &node_a = nodes.at(index_a);
    if (node_a.isLeaf() || node_a.height <= 1)
    {
        return index_a;
    }

    auto index_b = nodes.at(index_a).child_index_1;
    auto index_c = nodes.at(index_a).child_index_2;
    auto &node_c = nodes.at(index_c);
    auto &node_b = nodes.at(index_b);

    auto root_node_height = nodes.at(root_ind).height;
    auto height_diff = node_c.height - node_b.height;
    if (height_diff > 1)
    { //! C is higher so it needs to move closer to root
        moveNodeUp(index_c);
        return index_c;
    }
    if (height_diff < -1)
    {
        moveNodeUp(index_b);
        return index_b;
    }
    return index_a;
}

//! \brief does a subtree rotation of subtree rooted at \p going_up_index  to reduce tree depth
//! \brief node C is put on As place, node A moves to Bs place and his new parent is C
//! \brief Cs children are thus A and higher node from C1 and C2 (has larger node.height)
//! \brief As children are thus B and lower node from C1 and C2 (has smaller node.height)
//! \brief Bs parent is A and his children stay the same
//! \brief     Example, old tree:
//! \brief
//! \brief           A
//! \brief       /       \
//! \brief      B         C
//! \brief    /   \     /   \  
//! \brief   B1   B2   C1   C2
//! \brief
//! \brief          assuming C2.height > C1.height, new tree is
//! \brief
//! \brief          C
//! \brief       /     \
//! \brief      A       C2
//! \brief     / \                                      
//! \brief    B   C1
//! \brief   / \      
//! \brief  B1  B2
void BoundingVolumeTree::moveNodeUp(int going_up_index)
{

    auto &node_c = nodes.at(going_up_index);
    auto index_a = node_c.parent_index;
    auto &node_a = nodes.at(index_a);

    bool switched = false;
    int index_b = node_a.child_index_1;
    if (node_a.child_index_1 == going_up_index) //! find which child of a is going_up
    {
        index_b = node_a.child_index_2;
        switched = true;
    }
    auto &node_b = nodes.at(index_b);

    auto index_c1 = node_c.child_index_1;
    auto index_c2 = node_c.child_index_2;

    auto &node_c1 = nodes.at(index_c1);
    auto &node_c2 = nodes.at(index_c2);

    node_c.parent_index = node_a.parent_index;
    node_a.parent_index = going_up_index;
    node_c.child_index_1 = index_a;

    if (node_c.parent_index != -1) //! tell As old parent that he has a new kid
    {
        if (nodes.at(node_c.parent_index).child_index_1 == index_a)
        {
            nodes.at(node_c.parent_index).child_index_1 = going_up_index;
        }
        else
        {
            nodes.at(node_c.parent_index).child_index_2 = going_up_index;
        }
    }
    else
    { //! c is now root;
        root_ind = going_up_index;
    }

    if (node_c1.height > node_c2.height) //! c2 becomes child of A
    {
        node_c.child_index_2 = index_c1;
        node_c2.parent_index = index_a;
        if (!switched)
        {
            node_a.child_index_2 = index_c2;
        }
        else
        {
            node_a.child_index_1 = index_c2;
        }
        node_a.height = 1 + std::max(node_c2.height, node_b.height);
        node_c.height = 1 + std::max(node_c1.height, node_a.height);

        node_a.rect = makeUnion(node_b.rect, node_c2.rect);
        node_c.rect = makeUnion(node_a.rect, node_c1.rect);
    }
    else //! c1 becomes child of A
    {
        node_c.child_index_2 = index_c2;
        node_c1.parent_index = index_a;
        if (!switched)
        {
            node_a.child_index_2 = index_c1;
        }
        else
        {
            node_a.child_index_1 = index_c1;
        }

        node_a.height = 1 + std::max(node_c1.height, node_b.height);
        node_c.height = 1 + std::max(node_c2.height, node_a.height);

        node_a.rect = makeUnion(node_b.rect, node_c1.rect);
        node_c.rect = makeUnion(node_a.rect, node_c2.rect);
    }
}

//! \brief finds object indices that intersect a given \p rect
std::vector<int> BoundingVolumeTree::findIntersectingLeaves(AABB rect) const
{
    std::vector<int> intersecting_leaves;
    if (object2node_indices.empty()) //! if there are no objects there can be no intersections
    {
        return {};
    }

    std::stack<int> to_visit;
    auto &current_node = nodes.at(root_ind);
    to_visit.push(root_ind);
    while (!to_visit.empty())
    {
        auto current_index = to_visit.top();
        to_visit.pop();
        const auto &current_node = nodes.at(current_index);
        if (intersects(rect, current_node.rect))
        {

            if (current_node.child_index_1 != -1)
            {
                to_visit.push(current_node.child_index_1);
            }
            if (current_node.child_index_2 != -1)
            {
                to_visit.push(current_node.child_index_2);
            }

            if (current_node.isLeaf())
            {
                assert(current_node.object_index != -1);
                assert(object2node_indices.at(current_node.object_index) == current_index);
                intersecting_leaves.push_back(current_node.object_index);
            }
        }
    }

    return intersecting_leaves;
}

int BoundingVolumeTree::calcMaxDepth() const
{
    std::queue<std::pair<int, int>> to_visit;
    to_visit.push({root_ind, 0});
    int max_lvl = 0;
    while (!to_visit.empty())
    {

        auto current_index = to_visit.front().first;
        auto lvl = to_visit.front().second;
        to_visit.pop();
        if (lvl > max_lvl)
        {
            max_lvl = lvl;
        }
        auto &current = nodes.at(current_index);
        if (!current.isLeaf())
        {
            auto &child1 = nodes.at(current.child_index_1);
            auto &child2 = nodes.at(current.child_index_2);
            to_visit.push({current.child_index_1, lvl + 1});
            to_visit.push({current.child_index_2, lvl + 1});
        }
    }
    assert(max_lvl == nodes.at(root_ind).height);
    return max_lvl;
}

//! \brief finds if segment intersects given rectangle
//! \param from starting point of the segment
//! \param to end point of the segment
//! \param rect
//! \returns true if there is an intersection
bool BoundingVolumeTree::intersectsLine(utils::Vector2f from, utils::Vector2f to, AABB rect)
{

    auto dr = to - from;

    utils::Vector2f normal = {dr.y, -dr.x};
    normal /= norm(normal);

    utils::Vector2f r1 = rect.r_min;
    utils::Vector2f r2 = rect.r_min + utils::Vector2f{rect.r_max.x - rect.r_min.x, 0};
    utils::Vector2f r3 = rect.r_max;
    utils::Vector2f r4 = rect.r_min + utils::Vector2f{0, rect.r_max.y - rect.r_min.y};

    auto proj = projectOnAxis(normal, {r1, r2, r3, r4});
    auto proj2 = projectOnAxis(normal, {from});
    return overlap1D(proj, proj2);
}

//! \brief finds objects whose bounding rects intersect given a segment
//! \param from starting point of the segment
//! \param dir direction vector of the line
//! \param length of the segment
//! \returns list of object indices
std::vector<int> BoundingVolumeTree::rayCast(utils::Vector2f from, utils::Vector2f dir, float length)
{

    if (root_ind == -1) //! empty trees have nothing
    {
        return {};
    }

    utils::Vector2f to = from + dir * length;
    std::vector<int> intersections;

    std::queue<int> to_visit;
    to_visit.push(root_ind);
    while (!to_visit.empty())
    {

        int current_ind = to_visit.front();
        to_visit.pop();
        auto &current = nodes.at(current_ind);

        if (intersectsLine(from, to, current.rect))
        {
            if (!current.isLeaf())
            {
                to_visit.push(current.child_index_1);
                to_visit.push(current.child_index_2);
            }
            else
            {
                intersections.push_back(current.object_index);
            }
        }
    }

    return intersections;
}

void BoundingVolumeTree::clear()
{
    object2node_indices.clear();
    for (int i = 0; i < nodes.size(); ++i)
    {
        free_indices.insert(i);
    }
    root_ind = -1;
}

int BoundingVolumeTree::maxBalanceFactor() const
{
    int max_balance = 0;
    std::queue<int> to_visit;

    to_visit.push(root_ind);
    while (!to_visit.empty())
    {
        auto &node = nodes.at(to_visit.front());
        to_visit.pop();
        if (!node.isLeaf())
        {
            auto h1 = nodes.at(node.child_index_1).height;
            auto h2 = nodes.at(node.child_index_2).height;
            max_balance = std::max(max_balance, std::abs(h1 - h2));
            to_visit.push(node.child_index_1);
            to_visit.push(node.child_index_2);
        }
    }
    return max_balance;
}

//! \brief finds intersectings bounding rectangles accross this and \p tree
//! \returns list of object indices whose bounding rects intersect
std::vector<std::pair<int, int>> BoundingVolumeTree::findClosePairsWith(BoundingVolumeTree &tree) const
{
    std::vector<std::pair<int, int>> close_pairs;

    for (auto &[object_ind, node_ind] : object2node_indices)
    {
        auto nearest_objects_inds = tree.findIntersectingLeaves(nodes.at(node_ind).rect);
        for (auto i : nearest_objects_inds)
        {
            if (i == object_ind)
            {
                continue;
            }
            close_pairs.push_back({object_ind, i});
        }
    }
    return close_pairs;
}


std::vector<std::pair<int, int>> BoundingVolumeTree::findClosePairsWithin() const
{
    if(root_ind == -1)
    {
        return {};
    }

    
    std::vector<std::pair<int, int>> close_pairs;
    
    const auto& root = nodes.at(root_ind);
    if(root.child_index_1 == -1 && root.child_index_2 == -1)
    {
        return {};
    }
    
    std::vector<std::pair<int, int>> to_visit = {{root.child_index_1, root.child_index_2}};
    to_visit.reserve(nodes.size());
    while(!to_visit.empty())
    {
        auto [node_ind_i, node_ind_j] = to_visit.back();
        to_visit.pop_back();

        const auto& node_i = nodes.at(node_ind_i);
        const auto& node_j = nodes.at(node_ind_j);
        
        if(!intersects(node_i.rect, node_j.rect))
        {
            continue;
        }
        if(!node_i.isLeaf() && !node_j.isLeaf())
        {
            to_visit.emplace_back(node_i.child_index_1, node_j.child_index_1);
            to_visit.emplace_back(node_i.child_index_1, node_j.child_index_2);
            to_visit.emplace_back(node_i.child_index_2, node_j.child_index_1);
            to_visit.emplace_back(node_i.child_index_2, node_j.child_index_2);
        }else if(!node_i.isLeaf() && node_j.isLeaf())
        {
            to_visit.emplace_back(node_i.child_index_1, node_ind_j);
            to_visit.emplace_back(node_i.child_index_2, node_ind_j);
        }
        else if (node_i.isLeaf() && !node_j.isLeaf())
        {
            to_visit.emplace_back(node_ind_i, node_j.child_index_1);
            to_visit.emplace_back(node_ind_i, node_j.child_index_2);
        }else{
            close_pairs.emplace_back(node_i.object_index, node_j.object_index);
        }
    }

    return close_pairs;
}

//! \brief finds intersectings bounding rectangles accross this and \p tree
//! \returns list of object indices whose bounding rects intersect
std::vector<std::pair<int, int>> BoundingVolumeTree::findClosePairsWith2(BoundingVolumeTree &tree) const
{
    std::vector<std::pair<int, int>> close_pairs;

    if (root_ind == -1 || tree.root_ind == -1)
    {
        return {};
    }

    std::vector<std::pair<int, int>> to_visit = {{root_ind, tree.root_ind}};
    to_visit.reserve(std::max(tree.nodes.size(), nodes.size()));

    while (!to_visit.empty())
    {
        auto [node_ind_i, node_ind_j] = to_visit.back();
        to_visit.pop_back();

        const auto &node_i = nodes.at(node_ind_i);
        const auto &node_j = tree.nodes.at(node_ind_j);

        if (!intersects(node_i.rect, node_j.rect))
        {
            continue;
        }
        if (!node_i.isLeaf() && !node_j.isLeaf())
        {
            to_visit.emplace_back(node_i.child_index_1, node_j.child_index_1);
            to_visit.emplace_back(node_i.child_index_1, node_j.child_index_2);
            to_visit.emplace_back(node_i.child_index_2, node_j.child_index_1);
            to_visit.emplace_back(node_i.child_index_2, node_j.child_index_2);
        }
        if (!node_i.isLeaf() && node_j.isLeaf())
        {
            to_visit.emplace_back(node_i.child_index_1, node_ind_j);
            to_visit.emplace_back(node_i.child_index_2, node_ind_j);
        }
        if (node_i.isLeaf() && !node_j.isLeaf())
        {
            to_visit.emplace_back(node_ind_i, node_j.child_index_1);
            to_visit.emplace_back(node_ind_i, node_j.child_index_2);
        }
        if (node_i.isLeaf() && node_j.isLeaf())
        {
            close_pairs.push_back({node_i.object_index, node_j.object_index});
        }
    }

    return close_pairs;
}
