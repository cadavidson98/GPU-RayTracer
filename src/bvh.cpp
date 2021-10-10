#include "bvh.h"

#include <vector>
#include <algorithm>

using namespace std;

/**
 * Create a new bvh and store the triangles internally
 */ 
bvh::bvh(vector<TriangleGL> &tris) {
    // start by making the bvh leaves
    triangles_.insert(triangles_.end(), make_move_iterator(tris.begin()), make_move_iterator(tris.end()));
    vector<triangle_info> leaves = boundTriangles();
    // now construct the bvh
    NodeGL root;
    bvh_nodes_.push_back(root);
    buildRecurse(0, leaves);
}

/**
 * Create a compact version of the BVH which has all structs properly aligned for the GPU
 * param num_nodes - integer value which identifies the number of nodes generated
 */
NodeGL * bvh::getCompact(int &num_nodes) {
    num_nodes = bvh_nodes_.size();
    return data(bvh_nodes_);
}

/**
 * Get all the triangles stored in the bvh
 */ 
TriangleGL* bvh::getTriangles(int& num_triangles) {
    num_triangles = triangles_.size();
    return data(triangles_);
}

/**
 * Construct a leaf node containing 1 singular triangle
 * This is trivially done by calculating the extent of the triangle
 */ 
vector<triangle_info> bvh::boundTriangles() {
    size_t num_tri = triangles_.size();
    vector<triangle_info> tri_nodes(num_tri);
    for(size_t i = 0; i < num_tri; ++i) {
        tri_nodes[i].tri_offset_ = i;
        
        Dimension tri_bnd;
        pair<float, float> x_bnds = minmax({triangles_[i].p1[0], triangles_[i].p2[0], triangles_[i].p3[0]});
        pair<float, float> y_bnds = minmax({triangles_[i].p1[1], triangles_[i].p2[1], triangles_[i].p3[1]});
        pair<float, float> z_bnds = minmax({triangles_[i].p1[2], triangles_[i].p2[2], triangles_[i].p3[2]});
        
        tri_nodes[i].AABB_.min_x = x_bnds.first;
        tri_nodes[i].AABB_.max_x = x_bnds.second;
        
        tri_nodes[i].AABB_.min_y = y_bnds.first;
        tri_nodes[i].AABB_.max_y = y_bnds.second;
        
        tri_nodes[i].AABB_.min_z = z_bnds.first;
        tri_nodes[i].AABB_.max_z = z_bnds.second;

        tri_nodes[i].centroid_ = Point3D(.5*x_bnds.second+.5*x_bnds.first, .5*y_bnds.second+.5*y_bnds.first, .5*z_bnds.second+.5*z_bnds.first);
    }
    return tri_nodes;
}

/**
 * Determine the smallest boundary that encapsulates all the
 * triangles
 */ 
DimensionGL bvh::getExtent(const vector<triangle_info> &tris) {
    DimensionGL extent;
    size_t num_tris = tris.size();
    for(size_t i = 0; i < num_tris; ++i) {
        extent.max_x = max(extent.max_x, tris[i].AABB_.max_x);
        extent.max_y = max(extent.max_y, tris[i].AABB_.max_y);
        extent.max_z = max(extent.max_z, tris[i].AABB_.max_z);

        extent.min_x = min(extent.min_x, tris[i].AABB_.min_x);
        extent.min_y = min(extent.min_y, tris[i].AABB_.min_y);
        extent.min_z = min(extent.min_z, tris[i].AABB_.min_z);
    }
    return extent;
}

/**
 * Determine the smallest boundary that encapsulates all the
 * points
 */ 
DimensionGL bvh::getExtent(const vector<Point3D> &pts) {
    DimensionGL extent;
    for(size_t i = 0; i < pts.size(); ++i) {
        extent.max_x = max(extent.max_x, pts[i].x);
        extent.max_y = max(extent.max_y, pts[i].y);
        extent.max_z = max(extent.max_z, pts[i].z);

        extent.min_x = min(extent.min_x, pts[i].x);
        extent.min_y = min(extent.min_y, pts[i].y);
        extent.min_z = min(extent.min_z, pts[i].z);
    }
    return extent;
}

/**
 * Construct a bvh by using the Surface Area Heuristic to subdivide
 * the leaf nodes provided 
 */
void bvh::buildRecurse(int node_offset, vector<triangle_info> &tris) {
    if(tris.size() == 2) {
        NodeGL l_child, r_child;
        l_child.l_child_offset = -1;
        l_child.r_child_offset = -1;
        l_child.AABB = tris[0].AABB_;
        l_child.triangle_offset = tris[0].tri_offset_;

        r_child.l_child_offset = -1;
        r_child.r_child_offset = -1;
        r_child.AABB = tris[1].AABB_;
        r_child.triangle_offset = tris[1].tri_offset_;

        bvh_nodes_.push_back(l_child);
        bvh_nodes_.push_back(r_child);

        bvh_nodes_[node_offset].AABB = getExtent(tris);
        bvh_nodes_[node_offset].triangle_offset = -1;
        bvh_nodes_[node_offset].l_child_offset = node_offset + 1;
        bvh_nodes_[node_offset].r_child_offset = node_offset + 2;
        return;
    }
    if(tris.size() == 1) {
        NodeGL l_child;
        l_child.l_child_offset = -1;
        l_child.r_child_offset = -1;
        l_child.AABB = tris[0].AABB_;
        l_child.triangle_offset = tris[0].tri_offset_;

        bvh_nodes_.push_back(l_child);

        bvh_nodes_[node_offset].AABB = getExtent(tris);
        bvh_nodes_[node_offset].triangle_offset = -1;
        bvh_nodes_[node_offset].l_child_offset = node_offset + 1;
        bvh_nodes_[node_offset].r_child_offset = -1;
        return;
    }
    bvh_nodes_[node_offset].AABB = getExtent(tris);
    bvh_nodes_[node_offset].triangle_offset = -1;
    int split_index;
    vector<triangle_info> bin_1(0), bin_2(0);
    splitMidpoint(tris, bin_1, bin_2);
    // create left child
    NodeGL new_node;
    bvh_nodes_.push_back(new_node);
    int l_offset = node_offset + 1;
    bvh_nodes_[node_offset].l_child_offset = l_offset;
    buildRecurse(l_offset, bin_1);
    // create right child
    bvh_nodes_.push_back(new_node);
    int r_offset = bvh_nodes_.size() - 1;
    bvh_nodes_[node_offset].r_child_offset = r_offset;
    buildRecurse(r_offset, bin_2);
}

/**
 * Recursively build the bvh using the midpoint method.
*/
bool bvh::splitMidpoint(vector<triangle_info> &in_tris, vector<triangle_info> &bin_1, vector<triangle_info> &bin_2) {
    vector<Point3D> centroids;
    for (size_t i = 0; i < in_tris.size(); ++i) {
        centroids.push_back(in_tris[i].centroid_);
    }
    DimensionGL d = getExtent(centroids); 
    float thresh;
    float arr[3];
    arr[0] = abs(d.max_x - d.min_x);
    arr[1] = abs(d.max_y - d.min_y);
    arr[2] = abs(d.max_z - d.min_z);
    int index = 0;
    float max = arr[0];
    for(int i = 1; i < 3; ++i) {
        if(arr[i] > max) {
            index = i;
            max = arr[i];
        }
    }
    // now split
    if(index == 0) {
        // split on x
        sort(in_tris.begin(), in_tris.end(), [](const triangle_info& tri_1, const triangle_info &tri_2) {
            return tri_1.centroid_.x < tri_2.centroid_.x;
        });
    }
    else if (index == 1) {
        // split on y
        sort(in_tris.begin(), in_tris.end(), [](const triangle_info& tri_1, const triangle_info &tri_2) {
            return tri_1.centroid_.y < tri_2.centroid_.y;
        });
    }
    else {
        // split on z
        sort(in_tris.begin(), in_tris.end(), [](const triangle_info& tri_1, const triangle_info &tri_2) {
            return tri_1.centroid_.z < tri_2.centroid_.z;
        });
    }

    // now give half the leaves to each child
    int half = in_tris.size() / 2;
    bin_1 = vector<triangle_info>(in_tris.begin(), in_tris.begin() + half);
    bin_2 = vector<triangle_info>(in_tris.begin() + half, in_tris.end());
    return true;
}