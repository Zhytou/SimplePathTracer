#include "Camera.hpp"
#include "Utils.hpp"

namespace spt {

void Camera::update() {
    Vec3<float> forward = normalize(m_target - m_eye);

    m_axises[0] = normalize(cross(forward, m_up)); // X
    m_axises[1] = cross(m_axises[0], forward);     // Y
    m_axises[2] = forward;                         // Z

    m_focus = (m_target - m_eye).length();
    m_pixel = (2.f * ::tanf(m_fovy * 0.5f * PI / 180.f) * m_focus) / m_height;
}

Ray PerspectiveCamera::emit(int row, int col) {
    float x = (col + 0.5f - m_width / 2.f) * m_pixel;
    float y = -(row + 0.5f - m_height / 2.f) * m_pixel;
    float z = m_focus;

    Vec3<float> pos = m_eye;
    Vec3<float> dir = m_axises[0] * x + m_axises[1] * y + m_axises[2] * z;

    return Ray(pos, dir);
}

Ray OrthographicCamera::emit(int row, int col) {
    float x = (col + 0.5f - m_width / 2.f) * m_pixel;
    float y = -(row + 0.5f - m_height / 2.f) * m_pixel;
    float z = m_focus;

    Vec3<float> pos = m_eye + m_axises[0] * x + m_axises[1] * y;
    Vec3<float> dir = m_axises[2] * z;

    return Ray(pos, dir);
}

} // namespace spt