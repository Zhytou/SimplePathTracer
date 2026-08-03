#ifndef SPT_EMITTER_HPP
#define SPT_EMITTER_HPP

#include "Utils.hpp"

namespace spt {

class Primitive;

class Emitter {
   public:
    Emitter(int id, const Vec3<float>& color) : m_id(id), m_color(color) {}
    virtual ~Emitter() {}

    virtual bool isDelta() const { return false; }
    int getID() const { return m_id; }
    Vec3<float> getColor() const { return m_color; }
    std::shared_ptr<Primitive> getPrimitive() const { return m_primitive.lock(); }
    void setID(int id) { m_id = id; }
    void setColor(const Vec3<float>& color) { m_color = color; }
    void setPrimitive(std::shared_ptr<Primitive> primitive) { m_primitive = primitive; }

    /**
     * @brief Sample a direction from a given shading point to the emitter.
     * 
     * @param p The shading point to sample from
     * @return The sampled direction
     */
    virtual Vec3<float> sample(const Vec3<float>& p) const = 0;
    /**
     * @brief Sample a random point directly on the surface of the emitter(ONLY supported for area emitter).
     * 
     * @return The sampled point 
     */
    virtual Vec3<float> sample() const { return Vec3<float>(NAN); }
    /**
     *  @brief Calculate the probability density function (PDF) for sampling the emitter from a shading point.
     * 
     * @param wo The outgoing direction
     * @param n The normal vector
     * @param dis The distance
     * @return The probability density
     */
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const = 0;
    /**
     * @brief Get the area of the emitter
     * 
     * @return The area of the emitter
     */
    virtual float area() const { return 0.f; }

   protected:
    int m_id = -1;
    Vec3<float> m_color{0.f};
    std::weak_ptr<Primitive> m_primitive;
};

class AreaEmitter : public Emitter {
   public:
    AreaEmitter(int id, const Vec3<float>& color) : Emitter(id, color) {}
    ~AreaEmitter() {}

    virtual bool isDelta() const override { return false; }
    virtual Vec3<float> sample(const Vec3<float>& p) const override;
    virtual Vec3<float> sample() const override;
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const override;
};

class PointEmitter : public Emitter {
   public:
    PointEmitter(int id, const Vec3<float>& color, const Vec3<float>& position) : Emitter(id, color), m_position(position) {}
    ~PointEmitter() {}

    virtual Vec3<float> sample(const Vec3<float>& p) const override { return normalize(m_position - p); }
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const override { return INFINITY; }

   private:
    Vec3<float> m_position;
};

class DirectionalEmitter : public Emitter {
   public:
    DirectionalEmitter(int id, const Vec3<float>& color, const Vec3<float>& direction) : Emitter(id, color), m_direction(normalize(direction)) {}
    ~DirectionalEmitter() {}

    virtual Vec3<float> sample(const Vec3<float>& p) const override { return -m_direction; }
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const override { return INFINITY; }

   private:
    Vec3<float> m_direction;
};

} // namespace spt

#endif