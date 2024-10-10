#ifndef SRE_VEC_HPP
#define SRE_VEC_HPP

#include <cmath>
#include <functional>

#define PI M_PI

namespace sre {

template <typename T>
struct Vec2 {
  T u, v;

  // 构造与析构
  Vec2() = default;
  ~Vec2() = default;
  Vec2(const T& a, const T& b) : u(a), v(b) {}

  // 运算符
  Vec2<T> operator-(const Vec2<T>& other) const {
    return Vec2<T>(u - other.u, v - other.v);
  }
  Vec2<T> operator+(const Vec2<T>& other) const {
    return Vec2<T>(u + other.u, v + other.v);
  }
  template <typename K>
  Vec2<T> operator*(const K& k) const {
    return Vec2<T>(u * k, v * k);
  }
};

template <typename T>
struct Vec3 {
  T x, y, z;

  // 构造与析构
  Vec3() = default;
  ~Vec3() = default;
  Vec3(const T& a, const T& b, const T& c) : x(a), y(b), z(c) {}

  // 赋值
  Vec3& operator=(const Vec3<T>& other) {
    x = other.x;
    y = other.y;
    z = other.z;
    return *this;
  }

  // 运算符
  bool operator==(const Vec3<T>& other) const {
    return x == other.x && y == other.y && z == other.z;
  }
  bool operator!=(const Vec3<T>& other) const {
    return x != other.x || y != other.y || z != other.z;
  }
  Vec3<T> operator-() const { return Vec3<T>(-x, -y, -z); }
  Vec3<T> operator-(const Vec3<T>& other) const {
    return Vec3<T>(x - other.x, y - other.y, z - other.z);
  }
  Vec3<T> operator+(const Vec3<T>& other) const {
    return Vec3<T>(x + other.x, y + other.y, z + other.z);
  }
  template <typename K>
  Vec3<T> operator*(const K& k) const {
    return Vec3<T>(x * k, y * k, z * k);
  }
  Vec3<T> operator*(const Vec3<T>& other) const {
    return Vec3<T>(x * other.x, y * other.y, z * other.z);
  }
  template <typename K>
  Vec3<T> operator/(const K& k) const {
    return Vec3<T>(x / k, y / k, z / k);
  }
  Vec3<T>& operator-=(const Vec3<T>& other) {
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
  }
  Vec3<T>& operator+=(const Vec3<T>& other) {
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
  }
  template <typename K>
  Vec3<T>& operator*=(const K& k) {
    x *= k;
    y *= k;
    z *= k;
    return *this;
  }
  template <typename K>
  Vec3<T>& operator/=(const K& k) {
    x /= k;
    y /= k;
    z /= k;
    return *this;
  }

  Vec3<T>& normalize() {
    float d = sqrt(x * x + y * y + z * z);
    x /= d;
    y /= d;
    z /= d;
    return *this;
  }

  T length() const { return sqrt(x * x + y * y + z * z); }

  static Vec3<T> normalize(const Vec3<T>& v) {
    float d = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    return Vec3<T>(v.x / d, v.y / d, v.z / d);
  }

  static Vec3<T> cross(const Vec3<T>& v1, const Vec3<T>& v2) {
    return Vec3<T>(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                   v1.x * v2.y - v1.y * v2.x);
  }

  static T dot(const Vec3<T>& v1, const Vec3<T>& v2) {
    return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
  }

  static T distance(const Vec3<T>& v1, const Vec3<T>& v2) {
    return sqrt((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y) +
                (v1.z - v2.z) * (v1.z - v2.z));
  }
};
}  // namespace sre

template <typename T>
struct std::hash<sre::Vec3<T>> {
  std::size_t operator()(sre::Vec3<T> const& v) const noexcept {
    std::hash<T> hasher;
    std::size_t h1 = hasher(v.x);
    std::size_t h2 = hasher(v.y);
    std::size_t h3 = hasher(v.z);

    return h1 ^ (h2 << 1) ^ (h3 << 2);  // or use boost::hash_combine
  }
};
#endif