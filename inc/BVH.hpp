#ifndef SPT_BVH_HPP
#define SPT_BVH_HPP

#include <algorithm>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

#include "AABB.hpp"
#include "Hittable.hpp"
#include "Triangle.hpp"

namespace spt {
class BVH : public Hittable {
   public:
    BVH(int count, bool leaf) : Hittable(-1), m_count(count), m_leaf(leaf) {}

    // Construct bounding volume hierarchy structure with [beg, end) triangles
    // @param triangles: vector of shared pointers to triangles
    // @param beg: begin index of the triangles to construct the bvh
    // @param end: end index of the triangles to construct the bvh
    // @param cnt: minimum number of triangles in a leaf node
    // @return: shared pointer to the root node of the bvh
    static std::shared_ptr<BVH> constructBVH(std::vector<std::shared_ptr<Triangle>>& triangles, int beg, int end, int cnt);
    static void sortObjects(std::vector<std::shared_ptr<Triangle>>& triangles, int beg, int end, int axis);
    static float computeSAH(const AABB& parent, const AABB& left, const AABB& right, int cnt1, int cnt2);
    static AABB mergeAABBs(std::vector<std::shared_ptr<Triangle>>& triangles, int beg, int end);

    virtual bool hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const override;
    virtual AABB wrap() const override { return m_aabb; }

   private:
    int m_count;
    bool m_leaf;
    std::vector<std::shared_ptr<Hittable>> m_children;
    AABB m_aabb;
};

} // namespace spt

#endif
