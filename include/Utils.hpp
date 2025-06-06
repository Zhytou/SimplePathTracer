#ifndef SRE_UTILS_HPP
#define SRE_UTILS_HPP

#include <cassert>
#include <cmath>
#include <string>
#include <vector>
#include <ostream>
#include <sstream>
#include <type_traits>
#include <random>

#define PI 3.14159265358979323846f

namespace spt {

template <typename T>
struct Vec2 {
  T u, v;

  Vec2() = default;

  ~Vec2() = default;

  Vec2(const T& a, const T& b) : u(a), v(b) {}

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

  Vec3() = default;

  ~Vec3() = default;
  
  Vec3(const T& a, const T& b, const T& c) : x(a), y(b), z(c) {}

  Vec3(const T arr[3]) : x(arr[0]), y(arr[1]), z(arr[2]) {}

  Vec3& operator=(const Vec3<T>& other) {
    x = other.x;
    y = other.y;
    z = other.z;
    return *this;
  }

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

  Vec3<T> operator/(const Vec3<T>& other) const {
    return Vec3<T>(x / other.x, y / other.y, z / other.z);
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
    float d = ::sqrt(x * x + y * y + z * z);
    x /= d;
    y /= d;
    z /= d;
    return *this;
  }

  T length() const { return ::sqrt(x * x + y * y + z * z); }

};

template<typename T>
std::ostream& operator<<(std::ostream& os, const Vec2<T>& v) {
  os << "Vec2(" << v.u << ", " << v.v << ")";
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const Vec3<T>& v) {
  os << "Vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
  return os;
}

template<typename T>
static Vec3<T> normalize(const Vec3<T>& v) {
  float d = ::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
  return Vec3<T>(v.x / d, v.y / d, v.z / d);
}

template<typename T>
static Vec3<T> cross(const Vec3<T>& v1, const Vec3<T>& v2) {
  return Vec3<T>(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

template<typename T>
T dot(const Vec3<T>& v1, const Vec3<T>& v2) {
  return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template<typename T>
static T distance(const Vec3<T>& v1, const Vec3<T>& v2) {
  return ::sqrt((v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y) + (v1.z - v2.z) * (v1.z - v2.z));
}

template<typename T, typename K>
static Vec3<T> pow(const Vec3<T>& v, const K& k) {
  return Vec3<T>(::pow(v.x, k), ::pow(v.y, k), ::pow(v.z, k));
}

template<typename T>
T rand(T max, T min = 0) {
  static_assert(std::is_arithmetic<T>::value, "T must be numeric type");

  static std::random_device rd;
  static std::mt19937 gen(rd());

  if constexpr (std::is_integral<T>::value) {
    std::uniform_int_distribution<T> dis(min, max);
    return dis(gen);
  } else {
    std::uniform_real_distribution<T> dis(min, max);
    return dis(gen);
  }
}

static std::vector<std::string> split(const std::string& str, char delimiter=' ') {
  std::vector<std::string> tokens;
  std::string token;
  std::istringstream tokenStream(str);
  
  while (std::getline(tokenStream, token, delimiter)) {
    tokens.push_back(token);
  }
  
  return tokens;
}
}  // namespace spt

#endif