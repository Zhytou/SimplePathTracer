#ifndef SPT_EMITTER_HPP
#define SPT_EMITTER_HPP

#include "Utils.hpp"

namespace spt {

class Primitive;
class Distribution;

class Emitter {
   public:
    Emitter(int id, const Vec3<float>& radiance) : m_id(id), m_radiance(radiance) {}
    virtual ~Emitter() {}

    int getID() const { return m_id; }
    virtual const char* getTypeName() const = 0;
    std::shared_ptr<Primitive> getPrimitive() const { return m_primitive.lock(); }
    void setID(int id) { m_id = id; }
    void setPrimitive(std::shared_ptr<Primitive> primitive) { m_primitive = primitive; }

    /**
    * @brief  Sample the emitter and return the emitted radiance divided by the PDF.
    * 
    * @param point_x The point whose lighting is being evaluated 
    * @param[out] point_y The sampled point 
    * @param[out] normal_y The normal on the sampled point
    * @return The emitted radiance value divided by the probability density of the sampled point, namely radiance / pdf_a.
    */
    virtual Vec3<float> sample(const Vec3<float>& point_x, Vec3<float>& point_y, Vec3<float>& normal_y) const = 0;
    /**
     * @brief Sample the emitter and return the emitted radiance divided by the PDF.
     * 
     * @param[out] origin The origin point of the ray.
     * @param[out] direction The direction to sample from the emitter.
     * @param[out] normal The normal on the sampled point.
     * @param distribution The distribution to sample direction from.
     * @return The emitted radiance value divided by the probability density of the sampled direction, namely radiance * cos / (pdf_a * pdf_w).
     */
    virtual Vec3<float> sample(Vec3<float>& origin, Vec3<float>& direction, Vec3<float>& normal, Distribution& distribution) const = 0;
    /**
     * @brief Evaluate the radiance of emitter at a given direction.
     * 
     * @param wo The direction to evaluate
     * @return The emitter radiance at the direction
     */
    virtual Vec3<float> eval(const Vec3<float>& dir) const { return m_radiance; }
    /**
     *  @brief Calculate the PDF of area for NEE(direct light sampling).
     * 
     * @return The probability density of emitter
     */
    virtual float pdf() const = 0;

   protected:
    int m_id = -1;
    Vec3<float> m_radiance{0.f};
    std::weak_ptr<Primitive> m_primitive;
};

class AreaEmitter : public Emitter {
   public:
    AreaEmitter(int id, const Vec3<float>& radiance) : Emitter(id, radiance) {}
    ~AreaEmitter() {}

    virtual const char* getTypeName() const override { return "AreaEmitter"; }

    virtual Vec3<float> sample(const Vec3<float>& point_x, Vec3<float>& point_y, Vec3<float>& normal_y) const override;
    virtual Vec3<float> sample(Vec3<float>& origin, Vec3<float>& direction, Vec3<float>& normal, Distribution& distribution) const override;
    virtual float pdf() const override;
};

class DES {
   public:
    /**
     * @brief Constructs a Direct Emission Sampler from a collection of scene emitters.
     * 
     * @param emitters Vector of emitters (light sources) to be sampled.
     * @return Shared pointer to the newly created DES instance.
     */
    static std::shared_ptr<DES> create(std::vector<std::shared_ptr<Emitter>> emitters);
    /**
     * @brief Sample an emitter from the collection of emitters.
     * 
     * @return The selected emitter and corresponding probability.
    */
    std::pair<std::shared_ptr<Emitter>, float> sample() const;
    /**
     * @brief Calculate the selection probability of a emitter.
     */
    float prob(std::shared_ptr<Emitter>) const;

   private:
    std::vector<std::shared_ptr<Emitter>> m_emitters;
    std::vector<float> m_areas;
};

} // namespace spt

#endif