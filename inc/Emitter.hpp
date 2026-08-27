#ifndef SPT_EMITTER_HPP
#define SPT_EMITTER_HPP

#include "Utils.hpp"

namespace spt {

class Primitive;
class Ray;

class Emitter {
   public:
    Emitter(int id, const Vec3<float>& radiance) : m_id(id), m_radiance(radiance) {}
    virtual ~Emitter() {}

    virtual bool isDelta() const { return true; }
    int getID() const { return m_id; }
    virtual const char* getTypeName() const = 0;
    std::shared_ptr<Primitive> getPrimitive() const { return m_primitive.lock(); }
    void setID(int id) { m_id = id; }
    void setPrimitive(std::shared_ptr<Primitive> primitive) { m_primitive = primitive; }

    /**
    * @brief  Sample the emitter and return the emitted radiance divided by the PDF.
    * 
    * @param point_ref The reference point whose lighting is being evaluated 
    * @param[out] point The sampled point 
    * @param[out] normal The normal on the sampled point
    * @return The emitted radiance value divided by the probability density of the sampled point, namely radiance / pdf_a.
    */
    virtual Vec3<float> sample(const Vec3<float>& point_ref, Vec3<float>& point, Vec3<float>& normal) const = 0;
    /**
     * @brief Sample the emitter and return the emitted radiance divided by the PDF.
     * 
     * @param[out] ray The sampled ray.
     * @param[out] normal The normal on the sampled point.
     * @return The emitted radiance value divided by the probability density of the sampled direction, namely radiance * cos / (pdf_a * pdf_w).
     */
    virtual Vec3<float> sample(Ray& ray, Vec3<float>& normal) const = 0;
    /**
     * @brief Evaluate the radiance of emitter at a given direction.
     * 
     * @param dir_local The direction to evaluate
     * @return The emitter radiance at the direction
     */
    virtual Vec3<float> le(const Vec3<float>& dir_local) const { return m_radiance; }
    /**
     *  @brief Calculate the PDF of area for NEE(direct light sampling).
     * 
     * @return The probability density of emitter ray origin.
     */
    virtual float pdf() const = 0;
    /**
     * @brief Calculate the PDF of direction for bpt(emitter vertex sampling).
     * 
     * @param dir_local The direction to evaluate
     * @return The probability density of emitter ray direction.
     */
    virtual float pdf(const Vec3<float>& dir_local) const = 0;

   protected:
    int m_id = -1;
    Vec3<float> m_radiance{0.f};
    std::weak_ptr<Primitive> m_primitive;
};

class AreaEmitter : public Emitter {
   public:
    AreaEmitter(int id, const Vec3<float>& radiance) : Emitter(id, radiance) {}
    ~AreaEmitter() {}

    virtual bool isDelta() const override { return false; }
    virtual const char* getTypeName() const override { return "AreaEmitter"; }

    virtual Vec3<float> sample(const Vec3<float>& point_x, Vec3<float>& point_y, Vec3<float>& normal_y) const override;
    virtual Vec3<float> sample(Ray& ray, Vec3<float>& normal) const override;
    virtual float pdf() const override;
    virtual float pdf(const Vec3<float>& dir_local) const override;
};

class DES {
   public:
    /**
     * @brief Constructs a Direct Emission Sampler from a collection of scene emitters.
     * 
     * @param emitters Vector of emitters (light sources) to be sampled.
     * @return Shared pointer to the newly created DES instance.
     */
    static std::shared_ptr<DES> create(const std::vector<std::shared_ptr<Emitter>>& emitters);
    /**
     * @brief Sample an emitter from the collection of emitters.
     * 
     * @return The selected emitter.
    */
    std::shared_ptr<Emitter> sample() const;
    /**
     * @brief Calculate the selection probability of a emitter.
     */
    float prob(std::shared_ptr<Emitter>) const;

   private:
    std::vector<std::shared_ptr<Emitter>> m_delta_emitters;
    std::vector<std::shared_ptr<Emitter>> m_nondelta_emitters;
    std::vector<float> m_areas;
};

} // namespace spt

#endif