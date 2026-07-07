#ifndef SPT_CAMERA_HPP
#define SPT_CAMERA_HPP

#include <array>
#include <cmath>
#include <iostream>

#include "Ray.hpp"

namespace spt {

class Camera {
   public:
    Camera()          = default;
    virtual ~Camera() = default;

    virtual Ray emit(int row, int col) = 0;
    void update();

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getFovy() const { return m_fovy; }
    float getFocus() const { return m_focus; }
    float getPixel() const { return m_pixel; }
    const Vec3<float>& getEye() const { return m_eye; }
    const Vec3<float>& getTarget() const { return m_target; }
    const Vec3<float>& getUp() const { return m_up; }
    const Vec3<float>& getAxis(int index) const { return m_axises[index]; }

    void setEye(const Vec3<float>& eye) {
        m_eye = eye;
        update();
    }
    void setTarget(const Vec3<float>& target) {
        m_target = target;
        update();
    }
    void setUp(const Vec3<float>& up) {
        m_up = up;
        update();
    }
    void setWidth(int width) {
        m_width = width;
        update();
    }
    void setHeight(int height) {
        m_height = height;
        update();
    }
    void setFovy(float fovy) {
        m_fovy = fovy;
        update();
    }

   protected:
    Vec3<float> m_eye;
    Vec3<float> m_target;
    Vec3<float> m_up;
    std::array<Vec3<float>, 3> m_axises;

    int m_width;
    int m_height;
    float m_fovy;
    float m_focus;
    float m_pixel;
};

class PerspectiveCamera : public Camera {
   public:
    PerspectiveCamera()  = default;
    ~PerspectiveCamera() = default;

    virtual Ray emit(int row, int col) override;
};

class OrthographicCamera : public Camera {
   public:
    OrthographicCamera()  = default;
    ~OrthographicCamera() = default;

    virtual Ray emit(int row, int col) override;
};

} // namespace spt

#endif