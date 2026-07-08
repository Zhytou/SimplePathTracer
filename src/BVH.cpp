#include "BVH.hpp"

namespace spt {

std::shared_ptr<BVH> BVH::constructBVH(std::vector<std::shared_ptr<Triangle>>& triangles, int beg, int end, int cnt) {
    int num     = end - beg;
    auto bvh    = std::make_shared<BVH>(num, false);
    bvh->m_aabb = mergeAABBs(triangles, beg, end);

    // 1. Construct leaf node if the number of triangles in the current bvh is less than or equal to cnt
    if (num <= cnt) {
        bvh->m_leaf = true;
        bvh->m_children.assign(triangles.begin() + beg, triangles.begin() + end);
        return bvh;
    }

    // 2. Find the longest axis to split the triangles by
    auto delta = bvh->m_aabb.getDelta();
    int axis   = delta.x >= delta.max() ? 0 : (delta.y >= delta.max() ? 1 : 2);
    sortObjects(triangles, beg, end, axis);

    // 3. Iterate to find best split
    std::pair<int, float> split = {-1, INFINITY};
    for (int idx = beg + 1; idx < end; idx += cnt) {
        int lcnt = idx - beg;
        int rcnt = num - lcnt;

        auto left  = mergeAABBs(triangles, beg, idx);
        auto right = mergeAABBs(triangles, idx, end);
        float cost = computeSAH(bvh->m_aabb, left, right, lcnt, rcnt);

        if (cost < split.second) {
            split = {idx, cost};
        }
    }

    // 4. Make it a leaf node if splitting is more expensive than keeping it as a leaf
    if (split.first == -1 || split.second >= num * 1.1f) {
        bvh->m_leaf = true;
        bvh->m_children.assign(triangles.begin() + beg, triangles.begin() + end);
        return bvh;
    }

    // construct sub bvh
    bvh->m_children.assign(2, nullptr);
    bvh->m_children[0] = constructBVH(triangles, beg, split.first, cnt);
    bvh->m_children[1] = constructBVH(triangles, split.first, end, cnt);

    return bvh;
}

void BVH::sortObjects(std::vector<std::shared_ptr<Triangle>>& triangles, int beg, int end, int axis) {
    std::stable_sort(triangles.begin() + beg, triangles.begin() + end, [axis](std::shared_ptr<Triangle> triangle1, std::shared_ptr<Triangle> triangle2) {
        auto xyz1 = triangle1->wrap().getCenter();
        auto xyz2 = triangle2->wrap().getCenter();

        if (axis == 0) {
            return xyz1.x < xyz2.x;
        } else if (axis == 1) {
            return xyz1.y < xyz2.y;
        } else {
            return xyz1.z < xyz2.z;
        }
    });
}

AABB BVH::mergeAABBs(std::vector<std::shared_ptr<Triangle>>& triangles, int beg, int end) {
    AABB aabb;
    for (int i = beg; i < end; i++) {
        auto aabbi = triangles[i]->wrap();
        aabb.merge(aabbi);
    }
    return aabb;
}

float BVH::computeSAH(const AABB& parent, const AABB& left, const AABB& right, int lcount, int rcount) {
    float cost = 1 + left.getArea() / parent.getArea() * lcount + right.getArea() / parent.getArea() * rcount;
    return cost;
}

bool BVH::hit(const Ray& ray, float tmin, float tmax, HitRecord& rec) const {
    // 1. Check AABB intersection
    if (!m_aabb.intersect(ray, tmin, tmax)) { return false; }

    // 2. Check triangle intersection, if leaf node
    if (m_leaf) {
        bool ishit = false;
        for (auto child : m_children) {
            HitRecord crec;
            if (child->hit(ray, tmin, tmax, crec)) {
                ishit = true;
                tmax  = std::min(crec.distance, tmax);
                rec   = crec;
            }
        }
        return ishit;
    }

    // 3. Check sub bvh intersection, if not leaf node
    assert(m_children.size() == 2);
    HitRecord lrec, rrec;
    bool lhit = m_children[0]->hit(ray, tmin, tmax, lrec);
    bool rhit = m_children[1]->hit(ray, tmin, lhit ? lrec.distance : tmax, rrec);
    if (rhit) {
        rec = rrec;
    } else if (lhit) {
        rec = lrec;
    } else {
        return false;
    }

    return true;
}

} // namespace spt
