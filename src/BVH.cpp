#include "BVH.hpp"
#include "Primitive.hpp"

namespace spt {

std::shared_ptr<BVH> BVH::create(std::span<std::shared_ptr<Primitive>> primitives, const AABB& aabb, int max_leaf_size, int num_bins) {
    if (max_leaf_size < 1) { throw std::invalid_argument(std::format("BVH::init: maximum leaf node size {} must be greater than 0", max_leaf_size)); }

    // 0. Initialize config and bvh node
    bool use_binned_sah = num_bins > 0; // binned-sah indicator
    int num_prms        = primitives.size();
    int num_splits      = use_binned_sah ? num_bins : num_prms;

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
    Vec3<float> center1(INFINITY), center2(-INFINITY); // minimum and maximum center of primitives
    std::vector<AABB> prefix_aabbs, suffix_aabbs;      // shared bounding box of primitives  for both Binned-SAH and Exact-SAH
    std::vector<int> prefix_counts;

    if (use_binned_sah) { // Binned-SAH path
        // 2.1 Collect all primitive centers and compute centroid bounds
        std::vector<Vec3<float>> centers;
        for (int i = 0; i < num_prms; i++) {
            auto center = primitives[i]->wrap().center();
            center1     = min(center1, center);
            center2     = max(center2, center);
            centers.push_back(center);
        }

        // 2.2 Assign primitives into spatial bins
        bins.resize(num_bins);
        auto delta = center2 - center1;
        int axis   = argmax(delta);
        if (delta[axis] == 0) {
            bvh->m_leaf = true;
            bvh->m_primitives.assign(primitives.begin(), primitives.end());
            return bvh;
        }
        for (int i = 0; i < num_prms; i++) {
            int j = (centers[i][axis] - center1[axis]) / delta[axis] * num_bins; // j is bin index
            j     = std::clamp(j, 0, num_bins - 1);

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
        // 2.1 Sort primitives along the longest axis of node AABB
        auto delta = bvh->m_aabb.extent();
        int axis   = argmax(delta);
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
        float cost = INFINITY;
        AABB laabb, raabb;
    };
    Split best_split;
    for (int i = 1; i < num_splits; i++) { // split only happen ahead of i-th bin/primitive(in other words, i is the start index of the right child)
        int lcnt   = use_binned_sah ? prefix_counts[i - 1] : i - 1;
        int rcnt   = num_prms - lcnt;
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

    // 5. Partition primitives and recursively build left and right sub bvhs
    if (use_binned_sah) {
        Vec3<float> delta    = center2 - center1;
        int axis             = argmax(delta);
        float best_split_pos = delta[axis] / num_bins * best_split.index + center1[axis];
        int idx              = std::distance(primitives.begin(), std::partition(primitives.begin(), primitives.end(), [&](std::shared_ptr<Primitive> prm) { return prm->wrap().center()[axis] <= best_split_pos; }));

        if (idx == 0 || idx == num_prms) {
            bvh->m_leaf = true;
            bvh->m_primitives.assign(primitives.begin(), primitives.end());
            return bvh;
        }

        bvh->m_left  = create(primitives.subspan(0, idx), best_split.laabb, max_leaf_size, num_bins);
        bvh->m_right = create(primitives.subspan(idx), best_split.raabb, max_leaf_size, num_bins);
    } else {
        int idx = best_split.index;

        bvh->m_left  = create(primitives.subspan(0, idx), best_split.laabb, max_leaf_size, num_bins);
        bvh->m_right = create(primitives.subspan(idx), best_split.raabb, max_leaf_size, num_bins);
    }

    return bvh;
}

float BVH::eval(const AABB& parent, const AABB& left, const AABB& right, int lcount, int rcount) {
    return 1 + left.area() / parent.area() * lcount + right.area() / parent.area() * rcount;
}

bool BVH::intersect(Ray& ray, Intersection& its) const {
    // 0. Initialize variables
    bool hit = false;

    // 1. Check AABB intersection and update ray t range
    if (!m_aabb.intersect(ray)) { return false; }

    // 2. Check triangle intersection and update ray t range if leaf node
    if (m_leaf) {
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
    Intersection lits, rits;
    if (m_left->intersect(ray, lits)) {
        hit = true;
        its = lits;
        ray.setTMax(std::min(lits.distance, ray.getTMax()));
    }
    if (m_right->intersect(ray, rits)) {
        hit = true;
        its = rits;
    }

    return hit;
}

} // namespace spt
