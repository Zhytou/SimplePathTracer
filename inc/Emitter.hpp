#ifndef SPT_EMITTER_HPP
#define SPT_EMITTER_HPP

#include "Utils.hpp"

namespace spt {

class Primitive;

class Emitter {
   public:
    Emitter(int id, const Vec3<float>& color) : m_id(id), m_color(color) {}
    Emitter(int id, const Vec3<float>& color, std::shared_ptr<Primitive> primitive) : m_id(id), m_color(color), m_primitive(primitive) {}
    virtual ~Emitter() {}

    virtual bool isDelta() const { return false; }
    int getID() const { return m_id; }
    Vec3<float> getColor() const { return m_color; }
    std::shared_ptr<Primitive> getPrimitive() const { return m_primitive.lock(); }
    void setID(int id) { m_id = id; }
    void setColor(const Vec3<float>& color) { m_color = color; }
    void setPrimitive(std::shared_ptr<Primitive> primitive) { m_primitive = primitive; }

    virtual Vec3<float> sample(const Vec3<float>& p) const                          = 0;
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const = 0;
    virtual float area() const { return 0.f; }

   protected:
    int m_id = -1;
    Vec3<float> m_color{0.f};
    std::weak_ptr<Primitive> m_primitive;
};

class AreaEmitter : public Emitter {
   public:
    AreaEmitter(int id, const Vec3<float>& color, std::shared_ptr<Primitive> primitive) : Emitter(id, color, primitive) {}
    ~AreaEmitter() {}

    virtual bool isDelta() const override { return false; }
    virtual Vec3<float> sample(const Vec3<float>& p) const override;
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