#include "CollisionSystem.h"
#include "GameObject.h"

#include "Systems/System.h"

namespace Collisions
{

    std::vector<std::tuple<GameObject *, GameObject *, CollisionData>> collisions; //! for debugging

    CollisionSystem::CollisionSystem(PostOffice &messenger, utils::ContiguousColony<CollisionComponent, int> &comps)
        : p_post_office(&messenger), m_components(comps)
    {
        messenger.registerEvents<CollisionEventEntities, CollisionEventTypeEntity, CollisionEventTypes>();
        //! init the trees
        for (int i = 0; i < static_cast<int>(ObjectType::Count); ++i)
        {
            m_object_type2tree[static_cast<ObjectType>(i)] = {};
        }
    }

    void CollisionSystem::insertObject(GameObject &object)
    {
        auto bounding_rect = m_components.get(object.getId()).shape.getBoundingRect().inflate(1.5f);
        m_object_type2tree[object.getType()].addRect(bounding_rect, object.getId());
    }

    void CollisionSystem::removeObject(GameObject &object)
    {
        m_object_type2tree.at(object.getType()).removeObject(object.getId());
    }

    void CollisionSystem::preUpdate(float dt, EntityRegistryT &entities)
    {

        auto &comps = m_components.data;
        auto &comp_ids = m_components.data_ind2id;
        for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
        {
            auto &comp = comps[comp_id];
            for (auto &shape : comp.shape.convex_shapes)
            {
                shape.setPosition(entities.at(comp_ids.at(comp_id))->getPosition());
                shape.setScale(entities.at(comp_ids.at(comp_id))->getSize() / 2.);
                shape.setRotation(entities.at(comp_ids.at(comp_id))->getAngle());
            }
        }

        for (std::size_t comp_id = 0; comp_id < comps.size(); ++comp_id)
        {
            //! update the tree if the entity moved outside of it's BoundingBox
            // auto& entity
            auto &comp = comps[comp_id];
            auto &tree = m_object_type2tree.at(comp.type);
            auto entity_ind = comp_ids.at(comp_id);

            auto fitting_rect = comp.shape.getBoundingRect();
            auto big_bounding_rect = tree.getObjectRect(entity_ind);

            //! if object moved in a way that rect in the collision tree does not fully contain it
            if (makeUnion(fitting_rect, big_bounding_rect).volume() > big_bounding_rect.volume())
            {
                tree.removeObject(entity_ind);
                tree.addRect(fitting_rect.inflate(1.5f), entity_ind);
            }
        }

        for (auto &[type_pair, callback] : m_registered_resolvers)
        {
            auto &[type_a, type_b] = type_pair;
            auto &tree_a = m_object_type2tree.at((ObjectType)type_a);
            auto &tree_b = m_object_type2tree.at((ObjectType)type_b);

            std::vector<std::pair<int, int>> close_pairs;
            if (type_a == type_b)
            {
                close_pairs = tree_a.findClosePairsWithin();
            }
            else
            {
                close_pairs = tree_a.findClosePairsWith2(tree_b);
            }

            narrowPhase2(close_pairs, entities, callback);
        }

        m_collided2.clear();
    }

    void CollisionSystem::shapesCollide(const std::vector<Polygon>& shape1, const std::vector<Polygon>& shape2,
                    GameObject& obj1, GameObject& obj2, CollisionCallbackT &callback)
    {
        for (auto &sub_shape1 : shape1)
        {
            for (auto &sub_shape2 : shape2)
            {

                CollisionData collision_data = getCollisionData(sub_shape1, sub_shape2);

                if (collision_data.minimum_translation > 0) //! there is a collision
                {

                    collisions.push_back({&obj1, &obj2, collision_data});

                    p_post_office->send(CollisionEvent{obj1.getId(), obj2.getId()});
                    callback(obj1, obj2, collision_data);
                    //! Fuck this shit, do not collide with multiple subshapes?
                    return;
                }
            }
        }
    }
    void CollisionSystem::narrowPhase2(const std::vector<std::pair<int, int>> &colliding_pairs,
                                       EntityRegistryT &entities, CollisionCallbackT &callback)
    {
        for (auto [i1, i2] : colliding_pairs)
        {
            assert(i1 != i2 && m_collided2.count({i1, i2}) == 0); //! no self collisions and evaluate each collision once

            auto &obj1 = *entities.at(i1);
            auto &obj2 = *entities.at(i2);
            m_collided2.insert({i1, i2});

            auto &shape1 = m_components.get(i1).shape.convex_shapes;
            auto &shape2 = m_components.get(i2).shape.convex_shapes;
            shapesCollide(shape1, shape2, obj1, obj2, callback);
        }
    }

    CollisionData CollisionSystem::getCollisionData(const Polygon &pa, const Polygon &pb) const
    {
        auto points_a = pa.getPointsInWorld();
        auto points_b = pb.getPointsInWorld();
        auto c_data = calcCollisionData(points_a, points_b);

        if (c_data.minimum_translation < 0.f)
        {
            return c_data; //! there is no collision so we don't need to extract manifold
        }
        auto center_a = pa.getPosition();
        auto center_b = pb.getPosition();
        //! make separation axis point always from a to b
        auto are_flipped = dot((center_a - center_b), c_data.separation_axis) > 0;
        if (are_flipped)
        {
            c_data.separation_axis *= -1.f;
        }

        auto col_feats1 = obtainFeatures(c_data.separation_axis, points_a);
        auto col_feats2 = obtainFeatures(-1.f * c_data.separation_axis, points_b);

        auto clipped_edge = clipEdges(col_feats1, col_feats2, c_data.separation_axis);
        if (clipped_edge.size() == 0) //! clipping failed so we don't do collision
        {
            c_data.minimum_translation = -1.f;
            return c_data;
        }
        for (auto ce : clipped_edge)
        {
            c_data.contact_point += ce;
        }
        c_data.contact_point /= (float)clipped_edge.size();

        return c_data;
    }

    std::vector<int> CollisionSystem::findNearestObjectInds(ObjectType type, utils::Vector2f center, float radius) const
    {
        auto &tree = m_object_type2tree.at(type);

        AABB collision_rect({center - utils::Vector2f{radius, radius}, center + utils::Vector2f{radius, radius}});
        return tree.findIntersectingLeaves(collision_rect);
    }

    std::vector<CollisionComponent *> CollisionSystem::findIntersections(ObjectType type, Polygon collision_body)
    {
        auto nearest_inds = m_object_type2tree.at(type).findIntersectingLeaves(collision_body.getBoundingRect());
        auto points = collision_body.getPointsInWorld();

        std::vector<CollisionComponent *> collision_ids;
        for (auto ind : nearest_inds)
        {
            auto &collision_comp = m_components.get(ind);

            for (auto &shape : collision_comp.shape.convex_shapes)
            {
                auto points_other = shape.getPointsInWorld();
                auto c_data = calcCollisionData(points, points_other);
                if (c_data.minimum_translation > 0.)
                {
                    collision_ids.push_back(&collision_comp);
                }
            }
        }
        return collision_ids;
    }

    std::vector<CollisionComponent *> CollisionSystem::findNearestObjects(ObjectType type, utils::Vector2f center, float radius) const
    {
        auto &tree = m_object_type2tree.at(type);

        AABB collision_rect({center - utils::Vector2f{radius, radius}, center + utils::Vector2f{radius, radius}});
        auto nearest_inds = tree.findIntersectingLeaves(collision_rect);
        std::vector<CollisionComponent *> objects;
        for (auto ind : nearest_inds)
        {
            auto &collision_comp = m_components.get(ind);
            auto mvt = collision_comp.shape.convex_shapes[0].getMVTOfSphere(center, radius);
            if (norm2(mvt) > 0.001f)
            {
                objects.push_back(&collision_comp);
            }
        }
        return objects;
    }

    utils::Vector2f CollisionSystem::findClosestIntesection(ObjectType type, utils::Vector2f at, utils::Vector2f dir, float length)
    {
        utils::Vector2f closest_intersection = at + dir * length;
        float min_dist = 200.f;
        auto inters = m_object_type2tree.at(type).rayCast(at, dir, length);
        for (auto ent_ind : inters)
        {
            auto &comp = m_components.get(ent_ind);
            for (auto &shape : comp.shape.convex_shapes)
            {
                auto points = shape.getPointsInWorld();
                int next = 1;
                for (int i = 0; i < points.size(); ++i)
                {
                    utils::Vector2f r1 = points.at(i);
                    utils::Vector2f r2 = points.at(next);

                    utils::Vector2f segment_intersection;
                    if (utils::segmentsIntersect(r1, r2, at, at + dir * length, segment_intersection))
                    {
                        auto new_dist = dist(segment_intersection, at);
                        if (new_dist < min_dist)
                        {
                            closest_intersection = segment_intersection;
                            min_dist = new_dist;
                        }
                    }
                    next++;
                    if (next == points.size())
                    {
                        next = 0;
                    }
                }
            }
        }
        return closest_intersection;
    }

    CollisionData inline calcCollisionData(const std::vector<utils::Vector2f> &points1,
                                           const std::vector<utils::Vector2f> &points2)
    {
        CollisionData collision_result;

        int next = 1;
        const auto n_points1 = points1.size();
        const auto n_points2 = points2.size();

        Edge contact_edge;

        float min_overlap = std::numeric_limits<float>::max();
        utils::Vector2f &min_axis = collision_result.separation_axis;
        for (int curr = 0; curr < n_points1; ++curr)
        {

            auto t1 = points1[next] - points1[curr]; //! line perpendicular to current polygon edge
            utils::Vector2f n1 = {t1.y, -t1.x};
            if (utils::approx_equal_zero(norm2(n1)))
            {
                continue;
            }
            n1 /= norm(n1);
            auto proj1 = projectOnAxis(n1, points1);
            auto proj2 = projectOnAxis(n1, points2);

            if (!overlap1D(proj1, proj2))
            {
                collision_result.minimum_translation = -1;
                return collision_result;
            }
            else
            {
                auto overlap = calcOverlap(proj1, proj2);
                if (utils::approx_equal_zero(overlap))
                {
                    continue;
                }
                if (overlap < min_overlap)
                {
                    min_overlap = overlap;
                    min_axis = n1;
                }
            }

            next++;
            if (next == n_points1)
            {
                next = 0;
            }
        }
        next = 1;
        for (int curr = 0; curr < n_points2; ++curr)
        {

            auto t1 = points2[next] - points2[curr]; //! line perpendicular to current polygon edge
            utils::Vector2f n1 = {t1.y, -t1.x};
            if (utils::approx_equal_zero(norm2(n1)))
            {
                continue;
            }
            n1 /= norm(n1);
            auto proj2 = projectOnAxis(n1, points2);
            auto proj1 = projectOnAxis(n1, points1);

            if (!overlap1D(proj1, proj2))
            {
                collision_result.minimum_translation = -1;
                return collision_result;
            }
            else
            {
                auto overlap = calcOverlap(proj1, proj2);
                if (utils::approx_equal_zero(overlap))
                {
                    continue;
                }
                if (overlap < min_overlap)
                {
                    min_overlap = overlap;
                    min_axis = n1;
                    collision_result.belongs_to_a = false;
                }
            }

            next++;
            if (next == n_points2)
            {
                next = 0;
            }
        }

        collision_result.minimum_translation = min_overlap;
        return collision_result;
    }

    int inline furthestVertex(utils::Vector2f separation_axis, const std::vector<utils::Vector2f> &points)
    {
        float max_dist = -std::numeric_limits<float>::max();
        int index = -1;
        for (int i = 0; i < points.size(); ++i)
        {
            auto dist = dot(points[i], separation_axis);
            if (dist > max_dist)
            {
                index = i;
                max_dist = dist;
            }
        }

        return index;
    }

    CollisionFeature inline obtainFeatures(const utils::Vector2f axis, const std::vector<utils::Vector2f> &points)
    {

        const auto n_points = points.size();
        auto furthest_v_ind1 = furthestVertex(axis, points);

        auto v1 = points[furthest_v_ind1];
        auto v1_next = points[(furthest_v_ind1 + 1) % n_points];
        auto v1_prev = points[(furthest_v_ind1 - 1 + n_points) % n_points];

        auto from_next = v1 - v1_next;
        auto from_prev = v1 - v1_prev;
        from_next /= norm(from_next);
        from_prev /= norm(from_prev);
        Edge best_edge;
        if (dot(from_prev, axis) <= dot(from_next, axis))
        {
            best_edge = Edge(v1_prev, v1);
        }
        else
        {
            best_edge = Edge(v1, v1_next);
        }
        CollisionFeature feature = {v1, best_edge};
        return feature;
    }

    std::vector<utils::Vector2f> inline clip(utils::Vector2f v1, utils::Vector2f v2, utils::Vector2f n, float overlap)
    {

        std::vector<utils::Vector2f> cp;
        float d1 = dot(v1, n) - overlap;
        float d2 = dot(v2, n) - overlap;
        if (d1 >= 0.0)
        {
            cp.push_back(v1);
        }
        if (d2 >= 0.0)
        {
            cp.push_back(v2);
        }
        if (d1 * d2 < 0.0)
        {

            utils::Vector2f e = v2 - v1;
            // compute the location along e
            float u = d1 / (d1 - d2);
            e *= u;
            e += v1;
            cp.push_back(e);
        }
        return cp;
    }

    std::vector<utils::Vector2f> inline clipEdges(CollisionFeature &ref_features, CollisionFeature &inc_features, utils::Vector2f n)
    {

        auto &ref_edge = ref_features.edge;
        auto &inc_edge = inc_features.edge;

        auto wtf_ref = std::abs(dot(ref_edge.t, n));
        auto wtf_inc = std::abs(dot(inc_edge.t, n));
        if (wtf_ref <= wtf_inc)
        {
        }
        else
        {
            std::swap(ref_features, inc_features);
        }

        utils::Vector2f ref_v = ref_edge.t;

        double o1 = dot(ref_v, ref_edge.from);
        // clip the incident edge by the first
        // vertex of the reference edge
        auto cp = clip(inc_edge.from, inc_edge.to(), ref_v, o1);
        auto cp_new = cp;
        // if we dont have 2 points left then fail
        if (cp.size() < 2)
        {
            return {};
        }

        double o2 = dot(ref_v, ref_edge.to());
        cp = clip(cp[0], cp[1], -ref_v, -o2);
        // if we dont have 2 points left then fail
        if (cp.size() < 2)
        {
            return {};
        }

        // get the reference edge normal
        utils::Vector2f refNorm = {-ref_v.y, ref_v.x};
        refNorm /= norm(refNorm);

        double max = dot(refNorm, ref_features.best_vertex);
        // make sure the final points are not past this maximum

        std::vector<float> depths(2);
        depths[0] = dot(refNorm, cp.at(0)) - max;
        depths[1] = dot(refNorm, cp.at(1)) - max;

        if (depths[0] < 0.0f)
        {
            cp.erase(cp.begin());
        }
        if (depths[1] < 0.0f)
        {
            cp.pop_back();
        }
        return cp;
    }

    void bounce(GameObject &obj1, GameObject &obj2, CollisionData c_data)
    {
        float mass1 = 1.f;
        float mass2 = 1.f;

        auto n = c_data.separation_axis;

        //! resolve interpenetration;
        float alpha = mass2 / (mass1 + mass2) * 0.5f;
        obj1.move(-c_data.separation_axis * c_data.minimum_translation * alpha); //! separation axis always points from 1 to 2
        obj2.move(c_data.separation_axis * c_data.minimum_translation * (1.f - alpha));

        auto cont_point = c_data.contact_point;

        auto v_rel = obj1.m_vel - obj2.m_vel;
        auto v_reln = dot(v_rel, n);

        float e = 1;
        float u_ab = 1. / mass1 + 1. / mass2;

        auto r_cont_coma = cont_point - obj1.getPosition();
        auto r_cont_comb = cont_point - obj2.getPosition();

        utils::Vector2f r_cont_coma_perp = {r_cont_coma.y, -r_cont_coma.x};
        utils::Vector2f r_cont_comb_perp = {r_cont_comb.y, -r_cont_comb.x};

        float ran = dot(r_cont_coma_perp, n);
        float rbn = dot(r_cont_comb_perp, n);

        float u_ab_rot = 0.f; // ran * ran / inertia1 + rbn * rbn / inertia2;

        float j_factor = -(1 + e) * v_reln / (u_ab + u_ab_rot);

        // angle_vel1 += ran * j_factor / inertia1;
        // angle_vel2 -= rbn * j_factor / inertia2;
        obj1.m_vel += j_factor / mass1 * n;
        obj2.m_vel -= j_factor / mass2 * n;
    }

    void drawComponent(const CollisionComponent &comp, Renderer &canvas)
    {
        for (auto &shape : comp.shape.convex_shapes)
        {
            drawShape(canvas, shape);
        }
    }

    void CollisionSystem::draw(Renderer &canvas)
    {

        for (std::size_t comp_id = 0; comp_id < m_components.data.size(); comp_id++)
        {
            drawComponent(m_components.data[comp_id], canvas);
        }

        //! draw physics collisions
        // for (auto &[obj1, obj2, c_data] : collisions)
        // {
        //     canvas.drawCricleBatched(obj1->getPosition(), 2., {1, 0, 0, 1});
        //     canvas.drawCricleBatched(obj2->getPosition(), 2., {0, 1, 1, 1});

        //     //! separation axis
        //     canvas.drawLineBatched(c_data.contact_point, c_data.contact_point + 10. * c_data.separation_axis, 0.2, {0, 0, 1, 1});
        // }
        collisions.clear();
    }

    void CollisionSystem::registerResolver(ObjectType type_a, ObjectType type_b, CollisionCallbackT callback)
    {
        if (!callback)
        {
            //! let objects deal with collisions themselves by deafult
            callback = [type_a, type_b](GameObject &obj_a, GameObject &obj_b, CollisionData c_data)
            {
                assert(obj_a.getType() == type_a && obj_b.getType() == type_b);

                obj_a.onCollisionWith(obj_b, c_data);
                obj_b.onCollisionWith(obj_a, c_data);
            };
        }

        m_registered_resolvers.insert({{(int)type_a, (int)type_b}, callback});
    }

} //! namespace collisions
