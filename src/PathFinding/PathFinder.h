#pragma once

#include <future>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <Triangulation.h>

class SurfaceManager;

struct Portal
{
    cdt::Vector2f first; //! left point of portal when looking through the portal
    cdt::Vector2f second;

    Portal(cdt::Vector2f a, cdt::Vector2f b) : first(a), second(b) {}
};

//! \note first and last element of portals should be one singular point with left == right
typedef std::deque<Portal> Funnel;

namespace pathfinding
{

    using namespace cdt;
    using Vertex = Vector2i;

    struct Edgef
    {
        cdt::Vector2f from;
        cdt::Vector2f t = {0.f, 0.f};
        float l = 0;

        Edgef(const cdt::Vector2f &v1, const cdt::Vector2f &v2)
            : from(v1)
        {
            const auto t_new = (v2 - v1);
            l = norm(t_new);
            t = t_new / l;
        }
        Edgef(const Vertex &v1, const Vertex &v2)
            : Edgef(asFloat(v1), asFloat(v2)) {}
        Edgef() = default;
        cdt::Vector2f to() const { return from + t * l; }
    };

    //! \class contains data and methods for pathfinding on a constrianed Delaunay triangulation
    class PathFinder
    {

    private:
        //! \struct holds data needed by prority_queue in Astar
        struct AstarDataPQ
        {
            TriInd next;                                       //! nexttriangle to visit
            float f_value = std::numeric_limits<float>::max(); //! sum of heuristic (h_value) and g_value
        };

        struct AstarData
        {
            float g_value = std::numeric_limits<float>::max(); //! sum of heuristic (h_value) and g_value
            float h_value = 0;
            int prev;
        };

        typedef int ReducedVertexInd;
        //! \struct contains together all the properties needed during a single Astar iteration
        struct AstarReducedData
        {
            float g_value = 0;
            float h_value = 0;
            ReducedVertexInd prev; //! back-pointer to a reduced vertex used to backtrack path
            int ind_in_tri;
        };

        //! data needed by priority_queue in Astar on reduced graph
        struct AstarReducedDataPQ
        {
            float f_value;
            ReducedVertexInd next; //! next reduced vertex to visit in Astar
        };

        //! \struct contains information about width of each triangle sides
        struct TriangleWidth
        {
            float widths[3];
            TriangleWidth(Triangle<Vertex> &tri);
            TriangleWidth()
            {
                widths[0] = std::numeric_limits<float>::max();
                widths[1] = std::numeric_limits<float>::max();
                widths[2] = std::numeric_limits<float>::max();
            }
        };

    public:
        struct PathData
        {
            std::deque<cdt::Vector2f> path;
            std::deque<Edgef> portals;
            Funnel funnel;
        };

    public:
        explicit PathFinder(Triangulation<Vertex> &cdt);
        explicit PathFinder(cdt::Triangulation<cdt::Vector2i> &cdt, SurfaceManager& surfaces);
    

        void update();

        void findSubOptimalPathCenters(const cdt::Vector2f r_start, const cdt::Vector2f r_end, float radius, Funnel &funnel);
        void findSubOptimalPathCenters(const cdt::Vector2f r_start, const cdt::Vector2f r_end,
                                       float radius, Funnel &funnel,
                                       std::function<bool(cdt::TriInd, cdt::TriInd)> path_rules);

        PathData doPathFinding(const cdt::Vector2f r_start, const cdt::Vector2f r_end, const float radius);
        PathData doPathFinding(const cdt::Vector2f r_start, const cdt::Vector2f r_end,
                               const float radius, std::function<bool(cdt::TriInd, cdt::TriInd)> path_rules);

    private:
        std::pair<TriInd, int> closestPointOnNavigableComponent(const cdt::Vector2f &r, const TriInd start_tri_ind,
                                                                const int end_component,
                                                                const int navigable_component) const;

        PathData pathFromFunnel(const cdt::Vector2f r_start, const cdt::Vector2f r_end, const float radius,
                                Funnel &fd) const;

        float sign(cdt::Vector2f a, cdt::Vector2f b, cdt::Vector2f c) const;

        Edgef pushAwayFromCorner(cdt::Vector2f &r_to_push, const cdt::Vector2f &r_prev, const cdt::Vector2f &r_next,
                                 const float distance, bool left) const;

    public:
        std::vector<TriangleWidth> triangle2tri_widths_;
        SurfaceManager* m_surfaces = nullptr;
        std::vector<std::array<bool,3>> m_tri2edge2is_navigable; 
        Triangulation<Vertex> &m_cdt; //! underlying triangulation

    private:
        std::vector<TriInd> m_back_pointers;
        std::vector<float> m_g_values;

    };

}; //! namespace pathfinding