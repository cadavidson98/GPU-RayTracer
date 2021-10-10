#ifndef bvh_h
#define bvh_h

#include <vector>

#include "structs.h"

using namespace std;

struct triangle_info {
  DimensionGL AABB_;
  Point3D centroid_;
  int tri_offset_;
};

/**
 * bvh - A bounding volume heirarchy that uses the midpoint method to split the search space.
 * Midpoint is essential because it ensure the tree is evenly balanced, which allows for particular
 * Maximum memory assumptions in the GPU ray tracer.
*/
class bvh {
  public:
    bvh() {};
    bvh(vector<TriangleGL> &tris);
    NodeGL * getCompact(int & num_nodes);
    TriangleGL * getTriangles(int& num_triangles);
  private:
    // All the triangles in the scene
    vector<TriangleGL> triangles_;
    // All the bvh nodes, sorted for depth first traversal
    vector<NodeGL> bvh_nodes_;

    // Helper functions for bounding all scene information
    DimensionGL getExtent(const vector<triangle_info> &tris);
    DimensionGL getExtent(const vector<Point3D> &pts);
    vector<triangle_info> boundTriangles();
    
    // Functions for constructing the bvh
    void buildRecurse(int node_offset, vector<triangle_info>& tris);
    bool splitMidpoint(vector<triangle_info> &in_tris, vector<triangle_info> &bin_1, vector<triangle_info> &bin_2);
};

#endif  // bvh_h