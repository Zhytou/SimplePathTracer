#ifndef SRE_AABB_HPP
#define SRE_AABB_HPP

#include <cassert>
#include <vector>
#include <memory>

#include "Hittable.hpp"
#include "Vec.hpp"

namespace spt {

class AABB : public Hittable {
 private:
  Vec3<float> minXYZ, maxXYZ;

 public:
  AABB();
  AABB(const Vec3<float>& a, const Vec3<float>& b);
  AABB(const std::shared_ptr<Hittable>& object);
  AABB(const std::vector<std::shared_ptr<Hittable>>& objects);
  AABB(const std::vector<std::shared_ptr<Hittable>>::const_iterator& beg, const std::vector<std::shared_ptr<Hittable>>::const_iterator& end);

 public:
  // getter.
  virtual Vec3<float> getMinXYZ() const override;
  virtual Vec3<float> getMaxXYZ() const override;
  
  // calculate surface area
  float getArea() const;

  // print.
  virtual void printStatus() const override;

  // hit
  virtual void hit(const Ray& ray, HitResult& res) const override;

  // merge
  static AABB merge(const AABB& aabb1, const AABB& aabb2);
  static AABB merge(const std::vector<AABB>& aabbs);

  // expand
  void expand(const AABB& aabb);
};
}  // namespace spt

#endif