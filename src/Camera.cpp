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
    Vec3<float> forward = normalize(m_target - m_eye);

    m_axises[0] = normalize(cross(forward, m_up)); // X
    m_axises[1] = cross(m_axises[0], forward);     // Y
    m_axises[2] = forward;                         // Z

    m_focus = length(m_target - m_eye);
    m_pixel = (2.f * ::tanf(m_fovy * 0.5f * PI / 180.f) * m_focus) / m_height;
}

Vec3f PerspectiveCamera::emit(const Vec2f& coord, Ray& ray) const {
    float r = coord.x;  // row
    float c = coord.y;  // column
    float d = m_pixel;  // side length of pixel
    float h = m_height; // number of rows of pixel
    float w = m_width;  // number of columns of pixel
    float f = m_focus;  // focus distance

    float x = (c - w / 2.f) * d;
    float y = -(r - h / 2.f) * d;
    float z = f;

    Vec3<float> pos = m_eye;
    Vec3<float> dir = m_axises[0] * x + m_axises[1] * y + m_axises[2] * z;
    ray.setOrigin(pos);
    ray.setDirection(dir);

    return Vec3f(1.f);
}

Vec3f OrthographicCamera::emit(const Vec2f& coord, Ray& ray) const {
    float r = coord.x;  // row
    float c = coord.y;  // column
    float d = m_pixel;  // side length of pixel
    float h = m_height; // number of rows of pixel
    float w = m_width;  // number of columns of pixel
    float f = m_focus;  // focus distance

    float x = (c - w / 2.f) * d;
    float y = -(r - h / 2.f) * d;
    float z = f;

    Vec3<float> pos = m_eye + m_axises[0] * x + m_axises[1] * y;
    Vec3<float> dir = m_axises[2] * z;
    ray.setOrigin(pos);
    ray.setDirection(dir);

    return Vec3f(1.f);
}

} // namespace spt