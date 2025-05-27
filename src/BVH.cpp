#include "BVH.hpp"

namespace spt {
BVH::BVH() : isLeaf(false) {}

// construct
std::shared_ptr<BVH> BVH::constructBVH(std::vector<std::shared_ptr<Hittable>>& objects, int beg, int end, int minCount) {
  auto bvh = std::make_shared<BVH>();

  // bvh aabb
  bvh->aabb = AABB(objects.begin()+beg, objects.begin()+end);

  // total node count
  int totCount = end - beg;

  // leaf bvh node
  if (totCount <= minCount) {
    bvh->isLeaf = true;
    bvh->objects.assign(objects.begin()+beg, objects.begin()+end);
    return bvh;
  }

  // delta xyz
  Vec3<float> deltaXYZ = bvh->getMaxXYZ()-bvh->getMinXYZ();
  int axis = 0;
  if (deltaXYZ.y > std::max(deltaXYZ.x, deltaXYZ.z)) {
    axis = 1;
  } else if(deltaXYZ.z > std::max(deltaXYZ.x, deltaXYZ.y)) {
    axis = 2;
  }

  // sort objects by the longest axis
  sortObjects(objects, beg, end, axis);
  
  // iterate to find best split
  int bestSplit = -1;
  float minCost = -1;

  // adaptive step to accelerate
  int step = std::max(1, totCount/10);

  for (int split = beg+1; split < end; split += step) {
    AABB aabb1(objects.begin()+beg, objects.begin()+split); // [beg, split)
    AABB aabb2(objects.begin()+split, objects.begin()+end); // [split, end)
    
    int leftCount = split - beg;
    int rightCount = totCount - leftCount;

    float cost = computeSAH(bvh->aabb, aabb1, aabb2, leftCount, rightCount);

    if (minCost == -1 || cost < minCost) {
      minCost = cost;
      bestSplit = split;
    }
  }

  // if no split improves cost, make this a leaf node
  if (bestSplit == -1 || minCost >= totCount) {
    bvh->isLeaf=true;
    bvh->objects.assign(objects.begin()+beg, objects.begin()+end);
    return bvh;
  }

  // construct sub bvh
  bvh->objects.assign(2, nullptr);
  bvh->objects[0] = constructBVH(objects, beg, bestSplit);
  bvh->objects[1] = constructBVH(objects, bestSplit, end);

  return bvh;
}

// sort objects by axis
void BVH::sortObjects(std::vector<std::shared_ptr<Hittable>>& objects, int beg, int end, int axis) {
  std::stable_sort(objects.begin()+beg, objects.begin()+end, [axis](std::shared_ptr<Hittable> obj1, std::shared_ptr<Hittable> obj2){
    Vec3<float> xyz1 = obj1->getMinXYZ();
    Vec3<float> xyz2 = obj2->getMinXYZ();

    if (axis == 0) {
      return xyz1.x < xyz2.x;
    } else if (axis == 1) {
      return xyz1.y < xyz2.y;
    } else {
      return xyz1.z < xyz2.z;
    }
  });
}

// compute SAH cost
float BVH::computeSAH(const AABB& parent, const AABB& left, const AABB& right, int leftCount, int rightCount) {
  float parentArea = parent.getArea();
  float leftArea = left.getArea();
  float rightArea = right.getArea();

  float cost = 1 + leftArea / parentArea * leftCount + rightArea / parentArea * rightCount;

  return cost;
}

// getter
Vec3<float> BVH::getMinXYZ() const { return aabb.getMinXYZ(); }
Vec3<float> BVH::getMaxXYZ() const { return aabb.getMaxXYZ(); }

// print
void BVH::printStatus() const { 
  for (auto obj : objects) {
    if (isLeaf) {
      std::cout << obj->getId() << '\t';
    } else {
      obj->printStatus();
    }
  }
  if (isLeaf) {
    std::cout << '\n';
  }
}

// hit
void BVH::hit(const Ray &ray, HitResult &res) const {
  // reset
  res.isHit = false;

  aabb.hit(ray, res);
  if (!res.isHit) {
    return;
  }

  if (isLeaf) {
    // reset result
    res.isHit = false;
    // current result
    HitResult cres;
    // find the best result
    for (auto obj : objects){
      obj->hit(ray, cres);
      if (cres.isHit && (!res.isHit || res.distance > cres.distance)) {
        res = cres;
      }
    }
    return;
  }

  assert(objects.size()==2); // left and right sub bvh
  HitResult lres, rres;
  objects[0]->hit(ray, lres);
  objects[1]->hit(ray, rres);

  if (lres.isHit && rres.isHit) {
    if (lres.distance < rres.distance) {
      res = lres;
    } else {
      res = rres;
    }
  } else if (lres.isHit) {
    res = lres;
  } else if (rres.isHit) {
    res = rres;
  } else {
    res.isHit = false;
  }
  return ;
}
}  // namespace spt
