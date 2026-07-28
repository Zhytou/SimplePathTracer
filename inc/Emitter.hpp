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
    virtual int getPrimitiveID() const { return -1; }
    virtual float getArea() const { return 0.f; }
    Vec3<float> getColor() const { return m_color; }

    virtual Vec3<float> sample(const Vec3<float>& p) const                          = 0;
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const = 0;

   private:
    int m_id = -1;

    Vec3<float> m_color = Vec3<float>(0.f);
};

class AreaEmitter : public Emitter {
   public:
    AreaEmitter(int id, const Vec3<float>& color, std::weak_ptr<Primitive> primitive) : Emitter(id, color), m_primitive(primitive) {}
    ~AreaEmitter() {}

    virtual bool isDelta() const override { return false; }
    virtual int getPrimitiveID() const override;
    virtual float getArea() const override;

    virtual Vec3<float> sample(const Vec3<float>& p) const override;
    virtual float pdf(const Vec3<float>& wo, const Vec3<float>& n, float dis) const override;

   private:
    std::weak_ptr<Primitive> m_primitive;
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