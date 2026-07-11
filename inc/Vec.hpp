#ifndef SPT_VEC_HPP
#define SPT_VEC_HPP

#include <cassert>
#include <cstring>
#include <sstream>
#include <string>
#include <type_traits>

namespace spt {

template <typename T, size_t N>
struct VecBase {
    T data[N];

    VecBase() { ::memset(data, 0, sizeof(T) * N); }
    VecBase(const T arr[N]) { ::memcpy(data, arr, sizeof(T) * N); }
    VecBase(const VecBase<T, N>& other) { ::memcpy(data, other.data, sizeof(T) * N); }
    VecBase& operator=(const VecBase<T, N>& other) {
        ::memcpy(data, other.data, sizeof(T) * N);
        return *this;
    }
};

template <typename T>
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

    VecBase() { ::memset(data, 0, sizeof(T) * 2); }
    VecBase(const T arr[2]) { ::memcpy(data, arr, sizeof(T) * 2); }
    VecBase(const VecBase<T, 2>& other) { ::memcpy(data, other.data, sizeof(T) * 2); }
    VecBase& operator=(const VecBase<T, 2>& other) {
        ::memcpy(data, other.data, sizeof(T) * 2);
        return *this;
    }
    VecBase(T t) {
        x = t;
        y = t;
    }
    VecBase(T t1, T t2) {
        x = t1;
        y = t2;
    }
};

template <typename T>
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

    VecBase() { ::memset(data, 0, sizeof(T) * 3); }
    VecBase(const T arr[3]) { ::memcpy(data, arr, sizeof(T) * 3); }
    VecBase(const VecBase<T, 3>& other) { ::memcpy(data, other.data, sizeof(T) * 3); }
    VecBase& operator=(const VecBase<T, 3>& other) {
        ::memcpy(data, other.data, sizeof(T) * 3);
        return *this;
    }
    VecBase(T t) {
        x = t;
        y = t;
        z = t;
    }
    VecBase(T t1, T t2, T t3) {
        x = t1;
        y = t2;
        z = t3;
    }
};

template <typename T, size_t N>
struct Vec : public VecBase<T, N> {
    // Inherit constructors. The subclass directly reuses all of the base class's constructors and avoid manually writing forwarding constructors: Vec(const T arr[N]) : VecBase(arr) {}
    using VecBase<T, N>::VecBase;
    // Bring dependent names into the scope of the subclass. Avoid compilation error due to the two-phase lookup rules for templates
    using VecBase<T, N>::data;

    Vec<T, N>& operator-=(const Vec<T, N>& other) {
        for (int i = 0; i < N; i++) { data[i] -= other.data[i]; }
        return *this;
    }

    Vec<T, N>& operator+=(const Vec<T, N>& other) {
        for (int i = 0; i < N; i++) { data[i] += other.data[i]; }
        return *this;
    }

    template <typename K>
    Vec<T, N>& operator*=(const K& k) {
        for (int i = 0; i < N; i++) { data[i] *= k; }
        return *this;
    }

    Vec<T, N>& operator*=(const Vec<T, N>& v) {
        for (int i = 0; i < N; i++) { data[i] *= v.data[i]; }
        return *this;
    }

    Vec<T, N>& operator/=(const Vec<T, N>& v) {
        for (int i = 0; i < N; i++) { data[i] /= v.data[i]; }
        return *this;
    }
};

template <typename T>
using Vec2 = Vec<T, 2>;
template <typename T>
using Vec3 = Vec<T, 3>;
template <typename T>
using Vec4 = Vec<T, 4>;

using Vec2f = Vec2<float>;
using Vec3f = Vec3<float>;
using Vec4f = Vec4<float>;

template <typename T, size_t N>
bool operator==(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    for (int i = 0; i < N; i++) {
        if (v1.data[i] != v2.data[i]) { return false; }
    }
    return true;
}

template <typename T, size_t N>
bool operator!=(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    for (int i = 0; i < N; i++) {
        if (v1.data[i] == v2.data[i]) { return false; }
    }
    return true;
}

template <typename T, size_t N>
Vec<T, N> operator+(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) { v.data[i] = v1.data[i] + v2.data[i]; }
    return v;
}

template <typename T, size_t N>
Vec<T, N> operator-(const Vec<T, N>& v) {
    Vec<T, N> vn; // negative vector
    for (size_t i = 0; i < N; ++i) { vn.data[i] = -v.data[i]; }
    return vn;
}

template <typename T, size_t N>
Vec<T, N> operator-(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) { v.data[i] = v1.data[i] - v2.data[i]; }
    return v;
}

template <typename T, typename K, size_t N>
Vec<T, N> operator*(const Vec<T, N>& v, const K& k) {
    Vec<T, N> vm; // multiply vector
    for (int i = 0; i < N; i++) { vm.data[i] = v.data[i] * k; }
    return vm;
}

template <typename T, size_t N>
Vec<T, N> operator*(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) { v.data[i] = v1.data[i] * v2.data[i]; }
    return v;
}

template <typename T, typename K, size_t N>
Vec<T, N> operator/(const Vec<T, N>& v, const K& k) {
    Vec<T, N> vd; // divide vector
    for (int i = 0; i < N; i++) { vd.data[i] = v.data[i] / k; }
    return vd;
}

template <typename T, size_t N>
Vec<T, N> operator/(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) { v.data[i] = v1.data[i] / v2.data[i]; }
    return v;
}

template <typename T, size_t N>
std::ostream& operator<<(std::ostream& os, const Vec<T, N>& v) {
    os << "Vec" << N << "(";
    for (int i = 0; i < N; i++) {
        os << v.data[i] << ", ";
    }
    os << v.data[N - 1] << ")";
    return os;
}

template <typename T, size_t N>
T length(const Vec<T, N>& v) {
    T d = 0;
    for (int i = 0; i < N; i++) { d += v.data[i] * v.data[i]; }
    return std::sqrt(d);
}

template <typename T, size_t N>
Vec<T, N> normalize(const Vec<T, N>& v) {
    Vec<T, N> vn = v; // normalized vector
    T d          = length(v);
    if (d > 0) {
        for (int i = 0; i < N; i++) { vn.data[i] = vn.data[i] / d; }
    }
    return vn;
}

template <typename T, size_t N>
Vec<T, N> cross(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    static_assert(N == 3, "cross product only defined for 3-dimensional Vec");
    v.data[0] = v1.data[1] * v2.data[2] - v1.data[2] * v2.data[1];
    v.data[1] = v1.data[2] * v2.data[0] - v1.data[0] * v2.data[2];
    v.data[2] = v1.data[0] * v2.data[1] - v1.data[1] * v2.data[0];
    return v;
}

template <typename T, size_t N>
T dot(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    T d = 0.f;
    for (int i = 0; i < N; i++) { d += v1.data[i] * v2.data[i]; }
    return d;
}

template <typename T, size_t N>
T distance(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    T d = 0.0;
    for (int i = 0; i < N; i++) { d += (v1.data[i] - v2.data[i]) * (v1.data[i] - v2.data[i]); }
    return ::sqrt(d);
}

template <typename T, typename K, size_t N>
Vec<T, N> pow(const Vec<T, N>& v, const K& k) {
    Vec<T, N> vp; // powered vector
    for (int i = 0; i < N; i++) { vp.data[i] = std::pow(v.data[i], k); }
    return vp;
}

template <typename T, size_t N>
Vec<T, N> clamp(const Vec<T, N>& v, const T& t1, const T& t2) {
    Vec<T, N> vc; // clamped vector
    for (int i = 0; i < N; i++) { vc.data[i] = std::min(std::max(v.data[i], t1), t2); }
    return vc;
}

template <typename T, size_t N>
T max(const Vec<T, N>& v) {
    T t = v.data[0];
    for (int i = 1; i < N; i++) {
        if (v.data[i] > t) { t = v.data[i]; }
    }
    return t;
}

template <typename T, size_t N>
Vec<T, N> max(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) {
        v.data[i] = std::max(v1.data[i], v2.data[i]);
    }
    return v;
}

template <typename T, size_t N>
T min(const Vec<T, N>& v) {
    T t = v.data[0];
    for (int i = 1; i < N; i++) {
        if (v.data[i] < t) { t = v.data[i]; }
    }
    return t;
}

template <typename T, size_t N>
Vec<T, N> min(const Vec<T, N>& v1, const Vec<T, N>& v2) {
    Vec<T, N> v;
    for (int i = 0; i < N; i++) {
        v.data[i] = std::min(v1.data[i], v2.data[i]);
    }
    return v;
}

} // namespace spt

#endif
