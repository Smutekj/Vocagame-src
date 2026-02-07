#pragma once

#include "BVH.h"

#include <vector>
#include <unordered_set>

#include "GameObject.h"
#include "Polygon.h"

#include "PostOffice.h"
#include "Systems/System.h"
#include "Renderer.h"

#include "ObjectRegistry.h"


struct CollisionShape
{
    std::vector<Polygon> convex_shapes;

    AABB getBoundingRect() const
    {
        assert(convex_shapes.size() > 0);
        AABB box = convex_shapes.at(0).getBoundingRect();
        for (std::size_t i = 1; i < convex_shapes.size(); ++i)
        {
            box = makeUnion(box, convex_shapes.at(i).getBoundingRect());
        }
        return box;
    }
};

enum class ColliderType
{
    Wall,
    OneSidedWall,
};

struct CollisionComponent
{
    CollisionShape shape;
    ObjectType type;
    std::function<void(int, ObjectType)> on_collision = [](auto, auto) {};
};

namespace Collisions
{

    struct pair_hash
    {
        inline std::size_t operator()(const std::pair<int, int> &v) const
        {
            return (((std::size_t)v.first) << 16) + (unsigned long)v.second;
}
    };

    using CollisionCallbackT = std::function<void(GameObject &, GameObject &, CollisionData)>;
    class CollisionSystem : public SystemI
    {

        std::unordered_map<int, std::weak_ptr<GameObject>> m_objects;
        std::unordered_map<ObjectType, BoundingVolumeTree> m_object_type2tree;

    public:
        CollisionSystem(PostOffice &messanger, utils::ContiguousColony<CollisionComponent, int> &comps);

        void insertObject(GameObject &object);
        void removeObject(GameObject &object);

        virtual void preUpdate(float dt, EntityRegistryT &entities) override;
        virtual void update(float dt) override {}
        virtual void postUpdate(float dt, EntityRegistryT &entities) override {}

        void draw(Renderer &canvas);

        void registerResolver(ObjectType type_a, ObjectType type_b, CollisionCallbackT callback = nullptr);

        std::vector<int> findNearestObjectInds(ObjectType type, utils::Vector2f center, float radius) const;
        std::vector<CollisionComponent *> findNearestObjects(ObjectType type, utils::Vector2f center, float radius) const;
        std::vector<CollisionComponent *> findIntersections(ObjectType type, Polygon collision_body);
        std::vector<int> findIntersectingObjectInds(ObjectType type, Polygon collision_body);

        utils::Vector2f findClosestIntesection(ObjectType type, utils::Vector2f at, utils::Vector2f dir, float length);

    private:
        void shapesCollide(const std::vector<Polygon> &shape1, const std::vector<Polygon> &shape2,
                           GameObject &obj1, GameObject &obj2, CollisionCallbackT &callback);
        void narrowPhase2(const std::vector<std::pair<int, int>> &colliding_pairs,
                          EntityRegistryT &entities,
                          CollisionCallbackT &callback);

        CollisionData getCollisionData(const Polygon &pa, const  Polygon &pb) const;

    private:
        PostOffice *p_post_office;
        std::unordered_map<std::pair<int, int>, CollisionCallbackT, pair_hash> m_registered_resolvers;
        std::unordered_set<std::pair<int, int>, pair_hash> m_collided2;

        utils::ContiguousColony<CollisionComponent, int> &m_components;
    };

    struct Edge
    {
        utils::Vector2f from;
        utils::Vector2f t;
        float l;
        Edge() = default;
        Edge(utils::Vector2f from, utils::Vector2f to) : from(from)
        {
            t = to - from;
            l = norm(t);
            t /= l;
        }
        utils::Vector2f to() const { return from + t * l; }
    };

    struct CollisionFeature
    {
        utils::Vector2f best_vertex;
        Edge edge;
    };

    CollisionData inline calcCollisionData(const std::vector<utils::Vector2f> &points1,
                                           const std::vector<utils::Vector2f> &points2);
    int inline furthestVertex(utils::Vector2f separation_axis, const std::vector<utils::Vector2f> &points);
    CollisionFeature inline obtainFeatures(const utils::Vector2f axis, const std::vector<utils::Vector2f> &points);
    std::vector<utils::Vector2f> inline clip(utils::Vector2f v1, utils::Vector2f v2, utils::Vector2f n, float overlap);

    std::vector<utils::Vector2f> inline clipEdges(
        CollisionFeature &ref_features,
        CollisionFeature &inc_features,
        utils::Vector2f n);

    void bounce(GameObject &obj1, GameObject &obj2, CollisionData c_data);

} //! namespace Collisions
