#ifndef SRE_VEC_HPP
#define SRE_VEC_HPP

#include <cmath>

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
};

template <typename T>
Vec3<T> normalize(const Vec3<T>& v) {
  float d = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  return Vec3<T>(v.x / d, v.y / d, v.z / d);
}

template <typename T>
Vec3<T> cross(const Vec3<T>& v1, const Vec3<T>& v2) {
  return Vec3<T>(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z,
                 v1.x * v2.y - v1.y * v2.x);
}

template <typename T>
T dot(const Vec3<T>& v1, const Vec3<T>& v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

#endif