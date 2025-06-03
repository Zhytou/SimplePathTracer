#include "AABB.hpp"

#include <iostream>

namespace spt {
AABB::AABB() : minXYZ(0, 0, 0), maxXYZ(0, 0, 0) {}

AABB::AABB(const Vec3<float>& a, const Vec3<float>& b) : minXYZ(a), maxXYZ(b) {}

AABB::AABB(const std::shared_ptr<Hittable>& object) {
  assert(object != nullptr);
  minXYZ = object->getMinXYZ();
  maxXYZ = object->getMaxXYZ();
}

AABB::AABB(const std::vector<std::shared_ptr<Hittable>>& objects) {
  for(int i = 0; i < objects.size(); i++) {
    if (i == 0) {
      minXYZ = objects[i]->getMinXYZ();
      maxXYZ = objects[i]->getMaxXYZ();
    } else {
      minXYZ.x = std::min(minXYZ.x, objects[i]->getMinXYZ().x);
      minXYZ.y = std::min(minXYZ.y, objects[i]->getMinXYZ().y);
      minXYZ.z = std::min(minXYZ.z, objects[i]->getMinXYZ().z);
    
      maxXYZ.x = std::max(maxXYZ.x, objects[i]->getMaxXYZ().x);
      maxXYZ.y = std::max(maxXYZ.y, objects[i]->getMaxXYZ().y);
      maxXYZ.z = std::max(maxXYZ.z, objects[i]->getMaxXYZ().z); 
    }
  }
}

AABB::AABB(const std::vector<std::shared_ptr<Hittable>>::const_iterator& beg, const std::vector<std::shared_ptr<Hittable>>::const_iterator& end) {
  for(auto itr = beg; itr != end; itr++) {
    if (itr == beg) {
      minXYZ = (*itr)->getMinXYZ();
      maxXYZ = (*itr)->getMaxXYZ();
    } else {
      minXYZ.x = std::min(minXYZ.x, (*itr)->getMinXYZ().x);
      minXYZ.y = std::min(minXYZ.y, (*itr)->getMinXYZ().y);
      minXYZ.z = std::min(minXYZ.z, (*itr)->getMinXYZ().z);
    
      maxXYZ.x = std::max(maxXYZ.x, (*itr)->getMaxXYZ().x);
      maxXYZ.y = std::max(maxXYZ.y, (*itr)->getMaxXYZ().y);
      maxXYZ.z = std::max(maxXYZ.z, (*itr)->getMaxXYZ().z); 
    }
  }
}

Vec3<float> AABB::getMinXYZ() const { return minXYZ; }

Vec3<float> AABB::getMaxXYZ() const { return maxXYZ; }

float AABB::getArea() const {
  Vec3<float> deltaXYZ = maxXYZ - minXYZ;

  return deltaXYZ.x * deltaXYZ.y + deltaXYZ.x * deltaXYZ.z + deltaXYZ.y * deltaXYZ.z;
}

void AABB::printStatus() const {
  std::cout << "AABB:\n"
            << "minXYZ: " << minXYZ.x << '\t' << minXYZ.y << '\t' << minXYZ.z
            << '\n'
            << "maxXYZ: " << maxXYZ.x << '\t' << maxXYZ.y << '\t' << maxXYZ.z
            << '\n';
  std::cout << std::endl;
}

AABB AABB::merge(const AABB& aabb1, const AABB& aabb2) {
  Vec3<float> minXYZ, maxXYZ;

  minXYZ.x = std::min(aabb1.getMinXYZ().x, aabb2.getMinXYZ().x);
  minXYZ.y = std::min(aabb1.getMinXYZ().y, aabb2.getMinXYZ().y);
  minXYZ.z = std::min(aabb1.getMinXYZ().z, aabb2.getMinXYZ().z);

  maxXYZ.x = std::max(aabb1.getMaxXYZ().x, aabb2.getMaxXYZ().x);
  maxXYZ.y = std::max(aabb1.getMaxXYZ().y, aabb2.getMaxXYZ().y);
  maxXYZ.z = std::max(aabb1.getMaxXYZ().z, aabb2.getMaxXYZ().z);

  return AABB(minXYZ, maxXYZ);
}

AABB AABB::merge(const std::vector<AABB>& aabbs) {
  Vec3<float> minXYZ, maxXYZ;

  for(int i = 0; i < aabbs.size(); i++) {
    if (i == 0) {
      minXYZ = aabbs[i].getMinXYZ();
      maxXYZ = aabbs[i].getMaxXYZ();
    } else {
      minXYZ.x = std::min(minXYZ.x, aabbs[i].getMinXYZ().x);
      minXYZ.y = std::min(minXYZ.y, aabbs[i].getMinXYZ().y);
      minXYZ.z = std::min(minXYZ.z, aabbs[i].getMinXYZ().z);
    
      maxXYZ.x = std::max(maxXYZ.x, aabbs[i].getMaxXYZ().x);
      maxXYZ.y = std::max(maxXYZ.y, aabbs[i].getMaxXYZ().y);
      maxXYZ.z = std::max(maxXYZ.z, aabbs[i].getMaxXYZ().z); 
    }
  }

  return AABB(minXYZ, maxXYZ);
}

void AABB::expand(const AABB& aabb) {
  minXYZ.x = std::min(minXYZ.x, aabb.getMinXYZ().x);
  minXYZ.y = std::min(minXYZ.y, aabb.getMinXYZ().y);
  minXYZ.z = std::min(minXYZ.z, aabb.getMinXYZ().z);

  maxXYZ.x = std::max(maxXYZ.x, aabb.getMaxXYZ().x);
  maxXYZ.y = std::max(maxXYZ.y, aabb.getMaxXYZ().y);
  maxXYZ.z = std::max(maxXYZ.z, aabb.getMaxXYZ().z);
}

void AABB::hit(const Ray& ray, HitResult& res) const {
  Vec3<float> origin = ray.getOrigin();
  Vec3<float> direction = ray.getDirection();
  Vec3<float> tMin, tMax;

  if (fabs(direction.x) < 0.000001f) {
    if (!(minXYZ.x <= origin.x && origin.x <= maxXYZ.x)) {
      res.hit = false;
      return;
    }
  } else {
    if (direction.x >= 0) {
      tMin.x = (minXYZ.x - origin.x) / direction.x;
      tMax.x = (maxXYZ.x - origin.x) / direction.x;
    } else {
      tMin.x = (maxXYZ.x - origin.x) / direction.x;
      tMax.x = (minXYZ.x - origin.x) / direction.x;
    }
  }

  if (fabs(direction.y) < 0.000001f) {
    if (!(minXYZ.y <= origin.y && origin.y <= maxXYZ.y)) {
      res.hit = false;
      return;
    }
  } else {
    if (direction.y >= 0) {
      tMin.y = (minXYZ.y - origin.y) / direction.y;
      tMax.y = (maxXYZ.y - origin.y) / direction.y;
    } else {
      tMin.y = (maxXYZ.y - origin.y) / direction.y;
      tMax.y = (minXYZ.y - origin.y) / direction.y;
    }
  }

  if (fabs(direction.z) < 0.000001f) {
    if (!(minXYZ.z <= origin.z && origin.z <= maxXYZ.z)) {
      res.hit = false;
      return;
    }
  } else {
    if (direction.z >= 0) {
      tMin.z = (minXYZ.z - origin.z) / direction.z;
      tMax.z = (maxXYZ.z - origin.z) / direction.z;
    } else {
      tMin.z = (maxXYZ.z - origin.z) / direction.z;
      tMax.z = (minXYZ.z - origin.z) / direction.z;
    }
  }

  float t0, t1;
  t0 = std::max(tMin.z, std::max(tMin.y, tMin.x));
  t1 = std::min(tMax.z, std::min(tMax.y, tMax.x));
  res.hit = t0 <= t1;
  return;
}
}  // namespace spt
