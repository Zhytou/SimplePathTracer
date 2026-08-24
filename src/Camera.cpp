#include "Camera.hpp"
#include "Film.hpp"
#include "Utils.hpp"

namespace spt {

void Camera::setFilm(std::shared_ptr<Film> film) {
    m_film = film;
    if (m_film) {
        m_width  = m_film->getWidth();
        m_height = m_film->getHeight();
        update();
    }
}

void Camera::update() {
    Vec3f forward = normalize(m_target - m_eye);

    m_axises[0] = normalize(cross(forward, m_up)); // X
    m_axises[1] = cross(m_axises[0], forward);     // Y
    m_axises[2] = forward;                         // Z

    m_focus = length(m_target - m_eye);
    m_pixel = (2.f * ::tanf(m_fovy * 0.5f * PI / 180.f) * m_focus) / m_height;
}

Vec2f Camera::project(const Vec3f& dir) const {
    float x = dir[0], y = dir[1], z = dir[2];
    float c = m_width / 2.f + (x * m_focus / z) / m_pixel;  // column
    float r = m_height / 2.f - (y * m_focus / z) / m_pixel; // row

    return {r, c};
}

Vec3f Camera::unproject(const Vec2f& coord) const {
    float r = coord[0], c = coord[1]; // row and column
    float x = (c - m_width / 2.f) * m_pixel;
    float y = -(r - m_height / 2.f) * m_pixel;
    float z = m_focus; // focus distance

    return {x, y, z};
}

Vec3f PerspectiveCamera::emit(const Vec2f& coord, Ray& ray) const {
    Vec3f pos       = m_eye;            // TODO: add UniformDiskDistribution
    Vec3f dir_local = unproject(coord); // coordinates of subpixel samples in caller
    Vec3f dir       = toWorld(dir_local);
    ray.setOrigin(pos);
    ray.setDirection(dir);

    // (we * cos_theta) / (pdf_pos * pdf_dir) is always 1.f
    return Vec3f(1.f);
}

Vec3f PerspectiveCamera::sample(const Vec3f& point_ref, Vec3f& point, Vec2f& coord) const {
    Vec3f pos       = m_eye; // TODO: add UniformDiskDistribution
    Vec3f dir       = normalize(point_ref - pos);
    Vec3f dir_local = toLocal(dir);
    float cos_theta = dir_local.z;

    point = pos;
    coord = project(dir_local);

    return we(dir_local) / pdf();
}

Vec3f PerspectiveCamera::we(const Vec3f dir_local) const {
    float cos_theta4 = dir_local.z * dir_local.z * dir_local.z * dir_local.z;
    float focus2     = m_focus * m_focus;
    float pixel_area = m_pixel * m_pixel;
    float lens_area  = m_lens_radius == 0.f ? 1.f : PI * m_lens_radius * m_lens_radius;
    return Vec3f(focus2 / (lens_area * pixel_area * cos_theta4));
}

Vec3f OrthographicCamera::emit(const Vec2f& coord, Ray& ray) const {
    Vec3f point = unproject(coord);
    Vec3f pos   = m_eye + toWorld({point.x, point.y, 0.f});
    Vec3f dir   = toWorld({0.f, 0.f, point.z});
    ray.setOrigin(pos);
    ray.setDirection(dir);

    return Vec3f(1.f);
}

Vec3f OrthographicCamera::sample(const Vec3f& point_ref, Vec3f& point, Vec2f& coord) const {
    throw std::runtime_error("OrthographicCamera::sample is not implemented");
}

Vec3f OrthographicCamera::we(const Vec3f dir_local) const {
    throw std::runtime_error("OrthographicCamera::we is not implemented");
}

} // namespace spt