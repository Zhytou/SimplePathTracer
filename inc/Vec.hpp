#ifndef SPT_VEC_HPP
#define SPT_VEC_HPP

#include <cassert>
#include <cmath>
#include <concepts>
#include <initializer_list>
#include <sstream>
#include <string>
#include <type_traits>

namespace spt {

template <typename T>
concept arithmetic = std::integral<T> || std::floating_point<T>;

template <arithmetic T, size_t N>
struct VecBase {
    T data[N];

    explicit VecBase(T t = static_cast<T>(0)) {
        for (int i = 0; i < N; i++) { data[i] = t; }
    }
    VecBase(const std::initializer_list<T>& list) {
        if (list.size() != N) { throw std::runtime_error("VecBase::VecBase: initializer_list size must match N"); }
        for (int i = 0; const auto& val : list) { data[i++] = val; }
    }
    VecBase(const VecBase<T, N>& other) {
        for (int i = 0; i < N; i++) { data[i] = other.data[i]; }
    }
    VecBase& operator=(const VecBase<T, N>& other) {
        for (int i = 0; i < N; i++) { data[i] = other.data[i]; }
        return *this;
    }
};

template <arithmetic T>
struct VecBase<T, 2> {
    union {
        T data[2];
        struct {
            T x, y;
        };
        struct {
            T u, v;
        };
    };

    explicit VecBase(T t = static_cast<T>(0)) {
        for (int i = 0; i < 2; i++) { data[i] = t; }
    }
    VecBase(const std::initializer_list<T>& list) {
        if (list.size() != 2) { throw std::runtime_error("VecBase<T, 2>::VecBase: initializer_list size must match 2"); }
        for (int i = 0; const auto& val : list) { data[i++] = val; }
    }
    VecBase(T t1, T t2) {
        x = t1;
        y = t2;
    }
    VecBase(const VecBase<T, 2>& other) {
        x = other.x;
        y = other.y;
    }
    VecBase& operator=(const VecBase<T, 2>& other) {
        x = other.x;
        y = other.y;
        return *this;
    }
};

template <arithmetic T>
struct VecBase<T, 3> {
    union {
        T data[3];
        struct {
            T x, y, z;
        };
        struct {
            T r, g, b;
        };
    };

    explicit VecBase(T t = static_cast<T>(0)) {
        for (int i = 0; i < 3; i++) { data[i] = t; }
    }
    VecBase(const std::initializer_list<T>& list) {
        if (list.size() != 3) { throw std::runtime_error("VecBase<T, 3>::VecBase: initializer_list size must match 3"); }
        for (int i = 0; const auto& val : list) { data[i++] = val; }
    }
    VecBase(T t1, T t2, T t3) {
        x = t1;
        y = t2;
        z = t3;
    }
    VecBase(const VecBase<T, 3>& other) {
        x = other.x;
        y = other.y;
        z = other.z;
    }
    VecBase& operator=(const VecBase<T, 3>& other) {
        x = other.x;
        y = other.y;
        z = other.z;
        return *this;
    }
};

template <arithmetic T>
struct VecBase<T, 4> {
    union {
        T data[4];
        struct {
            T x, y, z, w;
        };
        struct {
            T r, g, b, a;
        };
    };

    explicit VecBase(T t = static_cast<T>(0)) {
        for (int i = 0; i < 4; i++) { data[i] = t; }
    }
    VecBase(const std::initializer_list<T>& list) {
        if (list.size() != 4) { throw std::runtime_error("VecBase<T, 4>::VecBase: initializer_list size must match 4"); }
        for (int i = 0; const auto& val : list) { data[i++] = val; }
    }
    VecBase(T t1, T t2, T t3, T t4) {
        x = t1;
        y = t2;
        z = t3;
        w = t4;
    }
    VecBase(const VecBase<T, 4>& other) {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
    }
    VecBase& operator=(const VecBase<T, 4>& other) {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
        return *this;
    }
};

/**
 * @brief N-dimensional vector class.
 * 
 * Follows GLM conventions: treated as a column vector (N x 1) in matrix multiplications 
 * (e.g., M * v applies transformation matrix M to vector v).
 * 
 * @tparam T Component arithmetic type
 * @tparam N Vector dimension
 */
template <arithmetic T, size_t N>
struct Vec : public VecBase<T, N> {
    // Inherit constructors. The subclass directly reuses all of the base class's constructors and avoid manually writing forwarding constructors: Vec(const T arr[N]) : VecBase(arr) {}
    using VecBase<T, N>::VecBase;
    // Bring dependent names into the scope of the subclass. Avoid compilation error due to the two-phase lookup rules for templates
    using VecBase<T, N>::data;

    T& operator[](size_t i) {
        if (i >= N) { throw std::out_of_range("Vec::operator[]: index out of range"); }
        return data[i];
    }

    const T& operator[](size_t i) const {
        if (i >= N) { throw std::out_of_range("Vec::operator[]: index out of range"); }
        return data[i];
    }

    Vec<T, N>& operator-=(const Vec<T, N>& other) {
        for (int i = 0; i < N; i++) { data[i] -= other.data[i]; }
        return *this;
    }

    Vec<T, N>& operator+=(const Vec<T, N>& other) {
        for (int i = 0; i < N; i++) { data[i] += other.data[i]; }
        return *this;
    }

    Vec<T, N>& operator*=(const Vec<T, N>& v) {
        for (int i = 0; i < N; i++) { data[i] *= v.data[i]; }
        return *this;
    }

    template <arithmetic K>
    Vec<T, N>& operator*=(const K& k) {
        for (int i = 0; i < N; i++) { data[i] *= k; }
        return *this;
    }

    Vec<T, N>& operator/=(const Vec<T, N>& v) {
        for (int i = 0; i < N; i++) { data[i] /= v.data[i]; }
        return *this;
    }

    template <arithmetic K>
    Vec<T, N>& operator/=(const K& k) {
        for (int i = 0; i < N; i++) { data[i] /= k; }
        return *this;
    }
};

template <arithmetic T>
using Vec2 = Vec<T, 2>;
template <arithmetic T>
using Vec3 = Vec<T, 3>;
template <arithmetic T>
using Vec4 = Vec<T, 4>;

using Vec2f = Vec2<float>;
using Vec3f = Vec3<float>;
using Vec4f = Vec4<float>;
using Vec2i = Vec2<int>;
using Vec3i = Vec3<int>;
using Vec4i = Vec4<int>;

template <arithmetic T, size_t N>
bool operator==(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    for (int i = 0; i < N; i++) {
        if (v1.data[i] != v2.data[i]) { return false; }
    }
    return true;
}

template <arithmetic T, size_t N>
bool operator!=(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    return !(v1 == v2);
}

template <arithmetic T, size_t N>
Vec<T, N> operator+(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) { v.data[i] = v1.data[i] + v2.data[i]; }
    return v;
}

template <arithmetic T, size_t N>
Vec<T, N> operator-(const Vec<T, N>& v) {
    Vec<T, N> vn; // negative vector
    for (size_t i = 0; i < N; ++i) { vn.data[i] = -v.data[i]; }
    return vn;
}

template <arithmetic T, size_t N>
Vec<T, N> operator-(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) { v.data[i] = v1.data[i] - v2.data[i]; }
    return v;
}

template <arithmetic T, arithmetic K, size_t N>
Vec<T, N> operator*(const Vec<T, N>& v, const K& k) {
    Vec<T, N> vm; // multiply vector
    for (int i = 0; i < N; i++) { vm.data[i] = v.data[i] * k; }
    return vm;
}

template <arithmetic T, arithmetic K, size_t N>
Vec<T, N> operator*(const K& k, const Vec<T, N>& v) {
    Vec<T, N> vm; // multiply vector
    for (int i = 0; i < N; i++) { vm.data[i] = v.data[i] * k; }
    return vm;
}

template <arithmetic T, size_t N>
Vec<T, N> operator*(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) { v.data[i] = v1.data[i] * v2.data[i]; }
    return v;
}

template <arithmetic T, arithmetic K, size_t N>
Vec<T, N> operator/(const Vec<T, N>& v, const K& k) {
    Vec<T, N> vd; // divide vector
    for (int i = 0; i < N; i++) { vd.data[i] = v.data[i] / k; }
    return vd;
}

template <arithmetic T, size_t N>
Vec<T, N> operator/(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) { v.data[i] = v1.data[i] / v2.data[i]; }
    return v;
}

template <arithmetic T, size_t N>
std::ostream& operator<<(std::ostream& os, const Vec<T, N>& v) {
    std::stringstream ss;
    ss << "Vec" << N << "(";
    for (int i = 0; i < N; i++) {
        ss << v.data[i];
        if (i < N - 1) { ss << ", "; }
    }
    ss << ")";
    os << ss.str();
    return os;
}

template <arithmetic T, size_t N>
T length(const Vec<T, N>& v) {
    T d = 0;
    for (int i = 0; i < N; i++) { d += v.data[i] * v.data[i]; }
    return std::sqrt(d);
}

template <arithmetic T, size_t N>
Vec<T, N> normalize(const Vec<T, N>& v) {
    Vec<T, N> vn = v; // normalized vector
    T d          = length(v);
    if (d > 0) {
        for (int i = 0; i < N; i++) { vn.data[i] = vn.data[i] / d; }
    }
    return vn;
}

template <arithmetic T, size_t N>
Vec<T, N> cross(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    static_assert(N == 3, "cross product only defined for 3-dimensional Vec");
    v.data[0] = v1.data[1] * v2.data[2] - v1.data[2] * v2.data[1];
    v.data[1] = v1.data[2] * v2.data[0] - v1.data[0] * v2.data[2];
    v.data[2] = v1.data[0] * v2.data[1] - v1.data[1] * v2.data[0];
    return v;
}

template <arithmetic T, size_t N>
T dot(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    T d = 0.f;
    for (int i = 0; i < N; i++) { d += v1.data[i] * v2.data[i]; }
    return d;
}

template <arithmetic T, size_t N>
T distance(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    T d = 0.0;
    for (int i = 0; i < N; i++) { d += (v1.data[i] - v2.data[i]) * (v1.data[i] - v2.data[i]); }
    return ::sqrt(d);
}

template <arithmetic T, arithmetic K, size_t N>
Vec<T, N> pow(const Vec<T, N>& v, const K& k) {
    Vec<T, N> vp; // powered vector
    for (int i = 0; i < N; i++) { vp.data[i] = std::pow(v.data[i], k); }
    return vp;
}

template <arithmetic T, size_t N>
Vec<T, N> clamp(const Vec<T, N>& v, const T& t1, const T& t2) {
    Vec<T, N> vc; // clamped vector
    for (int i = 0; i < N; i++) { vc.data[i] = std::min(std::max(v.data[i], t1), t2); }
    return vc;
}

template <arithmetic T, size_t N>
T max(const Vec<T, N>& v) {
    T t = v.data[0];
    for (int i = 1; i < N; i++) {
        if (v.data[i] > t) { t = v.data[i]; }
    }
    return t;
}

template <arithmetic T, size_t N>
Vec<T, N> max(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) {
        v.data[i] = std::max(v1.data[i], v2.data[i]);
    }
    return v;
}

template <arithmetic T, size_t N>
size_t argmax(const Vec<T, N>& v) {
    size_t max_idx = 0;
    for (int i = 1; i < N; i++) {
        if (v.data[i] > v.data[max_idx]) { max_idx = i; }
    }
    return max_idx;
}

template <arithmetic T, size_t N>
T min(const Vec<T, N>& v) {
    T t = v.data[0];
    for (int i = 1; i < N; i++) {
        if (v.data[i] < t) { t = v.data[i]; }
    }
    return t;
}

template <arithmetic T, size_t N>
Vec<T, N> min(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) {
        v.data[i] = std::min(v1.data[i], v2.data[i]);
    }
    return v;
}

template <arithmetic T, size_t N>
size_t argmin(const Vec<T, N>& v) {
    size_t min_idx = 0;
    for (int i = 1; i < N; i++) {
        if (v.data[i] < v.data[min_idx]) { min_idx = i; }
    }
    return min_idx;
}

} // namespace spt

#endif
