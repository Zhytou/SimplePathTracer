#ifndef SPT_CAMERA_HPP
#define SPT_CAMERA_HPP

#include "Ray.hpp"

namespace spt {

class Film;

class Camera {
   public:
    Camera()          = default;
    virtual ~Camera() = default;

    /**
     * @brief Emit a ray from the camera to the subpixel at (x, y)
     * 
     * @param coord The coordinate of subpixel
     * @param[out] ray The sampled ray from the camera
     * @return Vec3f the color of the pixel
     */
    virtual Vec3f emit(const Vec2f& coord, Ray& ray) const = 0;
    /**
     * @brief Sample a pixel from the camera with subpixel sampling
     * 
     * @return Vec3f the pixel color
     */
    // virtual Vec3f sample(Vec3f&, Ray& ray, Vec2f& coord) const = 0;
    void update();

    virtual const char* getTypeName() const = 0;
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getFovy() const { return m_fovy; }
    float getFocus() const { return m_focus; }
    float getPixel() const { return m_pixel; }
    const Vec3<float>& getEye() const { return m_eye; }
    const Vec3<float>& getTarget() const { return m_target; }
    const Vec3<float>& getUp() const { return m_up; }
    const Vec3<float>& getAxis(int index) const { return m_axises[index]; }
    std::shared_ptr<Film> getFilm() const { return m_film; }

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
    void setFilm(std::shared_ptr<Film> film);

   protected:
    std::shared_ptr<Film> m_film;

    Vec3<float> m_eye;
    Vec3<float> m_target;
    Vec3<float> m_up;
    std::array<Vec3<float>, 3> m_axises;

    int m_width;
    int m_height;
    float m_fovy;  // field of view in vertical direction
    float m_focus; // focus length
    float m_pixel; // pixel size
};

class PerspectiveCamera : public Camera {
   public:
    PerspectiveCamera()  = default;
    ~PerspectiveCamera() = default;

    virtual const char* getTypeName() const override { return "Perspective"; }

    virtual Vec3f emit(const Vec2f& coord, Ray& ray) const override;
};

class OrthographicCamera : public Camera {
   public:
    OrthographicCamera()  = default;
    ~OrthographicCamera() = default;

    virtual const char* getTypeName() const override { return "Orthographic"; }

    virtual Vec3f emit(const Vec2f& coord, Ray& ray) const override;
};

} // namespace spt

#endif