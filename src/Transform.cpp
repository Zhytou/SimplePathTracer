#include "Transform.hpp"

namespace spt {

Mat4x4f translate(const Vec3f& offset) {
    Mat4x4f mat = Mat4x4f::eye();
    mat(0, 3)   = offset[0];
    mat(1, 3)   = offset[1];
    mat(2, 3)   = offset[2];
    return mat;
}

Mat4x4f scale(const Vec3f& s) {
    Mat4x4f mat = Mat4x4f::eye();
    mat(0, 0)   = s[0];
    mat(1, 1)   = s[1];
    mat(2, 2)   = s[2];
    return mat;
}

Mat4x4f rotate(int axis, float rad) {
    Mat4x4f mat = Mat4x4f::eye();
    switch (axis) {
        case 0: {
            float c = std::cos(rad), s = std::sin(rad);
            mat(1, 1) = c;
            mat(1, 2) = -s;
            mat(2, 1) = s;
            mat(2, 2) = c;
        } break;
        case 1: {
            float c   = std::cos(rad);
            float s   = std::sin(rad);
            mat(0, 0) = c;
            mat(0, 2) = s;
            mat(2, 0) = -s;
            mat(2, 2) = c;
        } break;
        case 2: {
            float c   = std::cos(rad);
            float s   = std::sin(rad);
            mat(0, 0) = c;
            mat(0, 1) = -s;
            mat(1, 0) = s;
            mat(1, 1) = c;
        } break;
        default: {
            throw std::invalid_argument("axis must be 0, 1, or 2");
        }
    }

    return mat;
}

Mat4x4f rotate(const Vec3f& d) {
    Mat4x4f mat = Mat4x4f::eye();
    for (int i = 0; i < 3; ++i) {
        mat = rotate(i, radians(d[i])) * mat;
    }
    return mat;
}

} // namespace spt
