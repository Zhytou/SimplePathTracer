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
     * @return The importance weight / Radiance factor (We) contributed by the camera sample.
     */
    virtual Vec3f emit(const Vec2f& coord, Ray& ray) const = 0;
    /**
     * @brief Sample a camera ray passing through a specified subpixel coordinate or towards a target point.
     * 
     * @param point_ref The reference target point in world space (used for directional light path connecting/sampling).
     * @param[out] point The sampled point on camera lens.
     * @param[out] coord The corresponding subpixel image coordinates.
     * @return The importance weight / Radiance factor (We) contributed by the camera sample.
     */
    virtual Vec3f sample(const Vec3f& point_ref, Vec3f& point, Vec2f& coord) const = 0;
    /**
     * @brief Calculate the pdf of camera ray origin (in area).
     */
    float pdf() const {
        return m_lens_radius == 0.f ? 1.f : 1 / (PI * m_lens_radius * m_lens_radius);
    }
    /**
     * @brief Calculate the pdf of camera ray direction (in solid angle).
     */
    float pdf(const Vec3f& dir_local) const {
        float cos_theta3 = dir_local.z * dir_local.z * dir_local.z;
        float focus2     = m_focus * m_focus;
        float pixel_area = m_pixel * m_pixel; // pixel size
        return focus2 / (cos_theta3 * pixel_area);
    }
    /**
     * @brief Calculate the importance weight / Radiance factor (We) contributed by the camera sample.
     * 
     * @param dir_local The direction vector in local space.
     * @return The importance weight / Radiance factor (We) contributed by the camera sample.
     */
    virtual Vec3f we(const Vec3f dir_local) const = 0;
    /**
     * @brief Update the camera parameters.
     */
    void update();

    /**
     * @brief Project a direction vector from camera space to image space.
     * 
     * @param dir The direction vector in camera space.
     * @return Vec2f The corresponding image space coordinates (in pixel space, e.g., [0, width] x [0, height]).
     */
    Vec2f project(const Vec3f& dir) const;
    /**
     * @brief Unproject a coordinate from image space to camera space.
     * 
     * @param coord The coordinate in image space
     * @return corresponding camera space coordinate corresponding camera space coordinate.
     */
    Vec3f unproject(const Vec2f& coord) const;

    Vec3f toLocal(const Vec3f& dir) const { return spt::toLocal(dir, m_axises[0], m_axises[1], m_axises[2]); }
    Vec3f toWorld(const Vec3f& dir) const { return spt::toWorld(dir, m_axises[0], m_axises[1], m_axises[2]); }

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

    int m_width         = 0;
    int m_height        = 0;
    float m_fovy        = 0.f; // field of view in vertical direction
    float m_focus       = 0.f; // focus length
    float m_pixel       = 0.f; // pixel size
    float m_lens_radius = 0.f; //TODO: add UniformDiskDistribution with radius of lens (radius of lens disk)
};

class PerspectiveCamera : public Camera {
   public:
    PerspectiveCamera()  = default;
    ~PerspectiveCamera() = default;

    virtual const char* getTypeName() const override { return "Perspective"; }

    virtual Vec3f emit(const Vec2f& coord, Ray& ray) const override;
    virtual Vec3f sample(const Vec3f& point_ref, Vec3f& point, Vec2f& coord) const override;
    virtual Vec3f we(const Vec3f dir_local) const override;
};

class OrthographicCamera : public Camera {
   public:
    OrthographicCamera()  = default;
    ~OrthographicCamera() = default;

    virtual const char* getTypeName() const override { return "Orthographic"; }

    virtual Vec3f emit(const Vec2f& coord, Ray& ray) const override;
    virtual Vec3f sample(const Vec3f& point_ref, Vec3f& point, Vec2f& coord) const override;
    virtual Vec3f we(const Vec3f dir_local) const override;
};

} // namespace spt

#endif