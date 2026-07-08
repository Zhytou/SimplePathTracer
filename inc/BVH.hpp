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
