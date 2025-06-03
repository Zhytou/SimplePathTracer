#ifndef SRE_BVH_HPP
#define SRE_BVH_HPP

#include <algorithm>
#include <cassert>
#include <iostream>
#include <vector>
#include <memory>

#include "AABB.hpp"
#include "Hittable.hpp"
#include "Triangle.hpp"

namespace spt {
class BVH : public Hittable {
 private:
  uint n;
  std::vector<std::shared_ptr<Hittable>> objects;
  AABB aabb;
  bool isLeaf;

 public:
  BVH(uint _n = 0);
  ~BVH() = default;

 public:
  // construct
  static std::shared_ptr<BVH> constructBVH( std::vector<std::shared_ptr<Hittable>>& objects, int beg, int end, int minCount=30);
  
  // sort
  static void sortObjects(std::vector<std::shared_ptr<Hittable>>& objects, int beg, int end, int axis) ;

  // compute
  static float computeSAH(const AABB& parent, const AABB& left, const AABB& right, int leftCount, int rightCount);

  // getter
  uint getSize() const { return n; }
  virtual Vec3<float> getMinXYZ() const override;
  virtual Vec3<float> getMaxXYZ() const override;

  // print
  virtual void printStatus() const override;

  // hit
  virtual void hit(const Ray &ray, HitResult &res) const override;
};

}  // namespace spt

#endif
