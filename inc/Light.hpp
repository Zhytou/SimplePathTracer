#ifndef SPT_LIGHT_HPP
#define SPT_LIGHT_HPP

#include <map>
#include <memory>
#include <vector>

#include "BVH.hpp"
#include "Triangle.hpp"

namespace spt {

class Light {
   public:
    Light()  = default;
    ~Light() = default;

    const std::vector<std::shared_ptr<Triangle>>& getTriangles() const { return m_triangles; }
    const std::vector<float>& getPsums() const { return m_psums; }
    float getSum() const { return m_sum; }

    // Set a light triangle
    void add(std::shared_ptr<Triangle> triangle);

    // Sample a point on the light triangle
    std::pair<int, Vec3<float>> sample() const;

    // Probability density function of the light triangle
    float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis);

   private:
    std::vector<std::shared_ptr<Triangle>> m_triangles;
    std::vector<float> m_psums; // partial sums of triangle areas
    float m_sum = 0.f;          // total sum of triangle areas
};

} // namespace spt

#endif