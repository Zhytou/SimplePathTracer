#include "BVH.hpp"
#include "Primitive.hpp"

namespace spt {

std::shared_ptr<BVH> BVH::create(std::span<std::shared_ptr<Primitive>> primitives, const AABB& aabb, int max_leaf_size, int num_bins) {
    if (max_leaf_size < 1) { throw std::invalid_argument(std::format("BVH::init: maximum leaf node size {} must be greater than 0", max_leaf_size)); }

    // 0. Initialize config and bvh node
    bool use_binned_sah = num_bins > 0 && primitives.size() > BVH_BIN_SAH_THRESHOLD; // binned-sah indicator
    int num_prms        = primitives.size();
    int num_splits      = use_binned_sah ? num_bins - 1 : num_prms - 1;

    auto bvh    = std::make_shared<BVH>();
    bvh->m_aabb = aabb;

    // 1. Construct leaf node if primitive count is within max threshold
    if (num_prms <= max_leaf_size) {
        bvh->m_leaf = true;
        bvh->m_primitives.assign(primitives.begin(), primitives.end());
        return bvh;
    }

    // 2. Preprocess with Binned or Exact SAH
    struct Bin {
        int count = 0;
        AABB aabb;
    };
    std::vector<Bin> bins;
    Vec3f center1(INF), center2(-INF);            // minimum and maximum center of primitives
    std::vector<AABB> prefix_aabbs, suffix_aabbs; // shared bounding box of primitives for both Binned-SAH and Exact-SAH
    std::vector<int> prefix_counts;

    // 2.0 Define bin index function for Binned-SAH
    int axis           = -1;
    float interval_inv = 0.f; // 1.f / (delta[axis] / num_bins)
    auto getBinIndex   = [&](const std::shared_ptr<Primitive>& prm) {
        Vec3f center = prm->wrap().center();
        int index    = (center[axis] - center1[axis]) * interval_inv;
        return std::clamp(index, 0, num_bins - 1);
    };

    if (use_binned_sah) { // Binned-SAH path
        // 2.1 Collect all primitive centers and compute centroid bounds
        for (int i = 0; i < num_prms; i++) {
            auto center = primitives[i]->wrap().center();
            center1     = min(center1, center);
            center2     = max(center2, center);
        }
        Vec3f delta  = center2 - center1;
        axis         = argmax(delta);
        interval_inv = num_bins / delta[axis];
        if (delta[axis] <= EPS) {
            bvh->m_leaf = true;
            bvh->m_primitives.assign(primitives.begin(), primitives.end());
            return bvh;
        }

        // 2.2 Assign primitives into spatial bins
        bins.resize(num_bins);
        for (int i = 0; i < num_prms; i++) {
            int j = getBinIndex(primitives[i]); // j is bin index

            bins[j].count++;
            bins[j].aabb.merge(primitives[i]->wrap());
        }

        // 2.3 Build prefix/suffix bounding boxes & cumulative primitive counts
        prefix_aabbs.resize(num_bins);
        prefix_counts.resize(num_bins);
        suffix_aabbs.resize(num_bins);

        prefix_aabbs[0]  = bins[0].aabb;
        prefix_counts[0] = bins[0].count;
        for (int i = 1; i < num_bins; i++) {
            prefix_aabbs[i] = bins[i].aabb;
            prefix_aabbs[i].merge(prefix_aabbs[i - 1]);
            prefix_counts[i] = bins[i].count + prefix_counts[i - 1];
        }

        suffix_aabbs[num_bins - 1] = bins[num_bins - 1].aabb;
        for (int i = num_bins - 2; i >= 0; i--) {
            suffix_aabbs[i] = bins[i].aabb;
            suffix_aabbs[i].merge(suffix_aabbs[i + 1]);
        }
    } else { // Exact-SAH path(by default)
        Vec3f delta = bvh->m_aabb.extent();
        axis        = argmax(delta);
        // 2.1 Sort primitives along the longest axis of node AABB
        std::stable_sort(primitives.begin(), primitives.end(), [axis](std::shared_ptr<Primitive> prm1, std::shared_ptr<Primitive> prm2) {
            auto center1 = prm1->wrap().center();
            auto center2 = prm2->wrap().center();
            return center1[axis] < center2[axis];
        });

        // 2.2 Build prefix/suffix bounding boxes
        prefix_aabbs.resize(num_prms);
        suffix_aabbs.resize(num_prms);

        prefix_aabbs[0] = primitives[0]->wrap();
        for (int i = 1; i < num_prms; i++) {
            prefix_aabbs[i] = primitives[i]->wrap();
            prefix_aabbs[i].merge(prefix_aabbs[i - 1]);
        }

        suffix_aabbs[num_prms - 1] = primitives[num_prms - 1]->wrap();
        for (int i = num_prms - 2; i >= 0; i--) {
            suffix_aabbs[i] = primitives[i]->wrap();
            suffix_aabbs[i].merge(suffix_aabbs[i + 1]);
        }
    }

    // 3. Evaluate SAH costs and find the optimal split candidate
    struct Split {
        int index  = -1;
        float cost = INF;
        AABB laabb, raabb;
    };
    Split best_split;
    for (int i = 1; i <= num_splits; i++) { // split only happen ahead of i-th bin/primitive(in other words, i is the start index of the right child)
        int lcnt = use_binned_sah ? prefix_counts[i - 1] : i;
        int rcnt = num_prms - lcnt;
        if (lcnt <= 0 || rcnt <= 0) { continue; } // avoid empty left/right child

        auto laabb = prefix_aabbs[i - 1];
        auto raabb = suffix_aabbs[i];

        float cost = eval(aabb, laabb, raabb, lcnt, rcnt);
        if (best_split.cost > cost) {
            best_split = Split{i, cost, laabb, raabb};
        }
    }

    // 4. Fallback to leaf if split provides no benefit
    if (best_split.index == -1 || best_split.cost >= num_prms * 1.1f) {
        bvh->m_leaf = true;
        bvh->m_primitives.assign(primitives.begin(), primitives.end());
        return bvh;
    }

    // 5. Partition primitives at the best split position
    int idx = use_binned_sah ? std::distance(primitives.begin(), std::partition(primitives.begin(), primitives.end(), [&](const std::shared_ptr<Primitive>& prm) { return getBinIndex(prm) < best_split.index; })) : best_split.index;

    // 6. Fallback to leaf if partition results in empty left/right child
    if (idx == 0 || idx == num_prms) {
        bvh->m_leaf = true;
        bvh->m_primitives.assign(primitives.begin(), primitives.end());
        return bvh;
    }

    // 7. Recursively build left and right sub bvhs
    bvh->m_left  = create(primitives.subspan(0, idx), best_split.laabb, max_leaf_size, num_bins);
    bvh->m_right = create(primitives.subspan(idx), best_split.raabb, max_leaf_size, num_bins);

    return bvh;
}

float BVH::eval(const AABB& parent, const AABB& left, const AABB& right, int lcount, int rcount) {
    return 1 + left.area() / parent.area() * lcount + right.area() / parent.area() * rcount;
}

bool BVH::intersect(Ray& ray, Intersection& its) const {
    // 1. Check AABB intersection and update ray t range
    if (!m_aabb.intersect(ray)) { return false; }

    // 2. Check triangle intersection and update ray t range if leaf node
    if (m_leaf) {
        bool hit = false;
        for (auto prm : m_primitives) {
            Intersection cits; // current intersection info struct
            if (prm->intersect(ray, cits)) {
                hit = true;
                its = cits;
                ray.setTMax(std::min(cits.distance, ray.getTMax()));
            }
        }
        return hit;
    }

    // 3. Check sub bvh intersection, if not leaf node
    float ldis      = distance(m_left->m_aabb.center(), ray.getOrigin());
    float rdis      = distance(m_right->m_aabb.center(), ray.getOrigin());
    auto first      = (ldis < rdis) ? m_left : m_right;
    auto second     = (ldis < rdis) ? m_right : m_left;
    bool hit_first  = first->intersect(ray, its);
    bool hit_second = second->intersect(ray, its);
    return hit_first || hit_second;
}

} // namespace spt
