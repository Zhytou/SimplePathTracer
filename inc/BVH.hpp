#ifndef SPT_BVH_HPP
#define SPT_BVH_HPP

#include "AABB.hpp"
#include "Intersection.hpp"
#include "Ray.hpp"

namespace spt {

class Primitive;

/**
 * @brief Bounding Volume Hierarchy (BVH) for fast ray tracing.
 */
class BVH {
   public:
    /**
     * @brief Recursively build BVH node using either Binned-SAH or Exact-SAH
     * 
     * @param primitives Span of primitives for current node
     * @param aabb Bounding box enclosing all input primitives
     * @param max_leaf_size Maximum primitive count inside a leaf node
     * @param num_bins Bin count for Binned-SAH; <=0 enables Exact-SAH
     * @return Shared pointer to newly constructed BVH node
     */
    static std::shared_ptr<BVH> create(std::span<std::shared_ptr<Primitive>> prms, const AABB& aabb, int max_leaf_size, int num_bins = -1);

    /**
     * @brief Evaluate SAH cost of a split
     * 
     * @param parent Parent bounding box
     * @param left Left bounding box
     * @param right Right bounding box
     * @param lcount Number of primitives in left child
     * @param rcount Number of primitives in right child
     * @return SAH cost of the split
     */
    static float eval(const AABB& parent, const AABB& left, const AABB& right, int lcount, int rcount);

    /**
     * @brief Test ray intersection against the BVH node under world coordinates.
     * 
     * @param ray Ray to check intersection
     * @param its Intersection info struct
     * @return True if the ray intersects the BVH node, False otherwise
     */
    bool intersect(Ray& ray, Intersection& its) const;

    /**
     * @brief Get the bounding box of the BVH node
     * 
     * @return Bounding box of the BVH node
     */
    AABB wrap() const { return m_aabb; }

    bool isLeaf() const { return m_leaf; }

   private:
    static constexpr int BVH_BIN_SAH_THRESHOLD = 2500; // primitive count threshold for binned-SAH to construct BVH

    bool m_leaf                  = false;
    std::shared_ptr<BVH> m_left  = nullptr;
    std::shared_ptr<BVH> m_right = nullptr;
    std::vector<std::shared_ptr<Primitive>> m_primitives;
    AABB m_aabb;
};

} // namespace spt

#endif
