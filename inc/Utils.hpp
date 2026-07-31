#ifndef SPT_UTILS_HPP
#define SPT_UTILS_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <ostream>
#include <random>
#include <ranges>
#include <span>
#include <sstream>
#include <string>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#define SPT_USE_GLM
#include "Mat.hpp"
#include "Transform.hpp"
#include "Vec.hpp"

namespace spt {

constexpr float EPS = 1e-6f;

constexpr float DIS_EPS = 1e-4f;

constexpr float PDF_EPS = 1e-6f;

constexpr float PI = 3.14159265358979323846f;

template <typename T>
T rand(T min, T max) {
    static_assert(std::is_arithmetic<T>::value, "T must be numeric type");

    thread_local std::random_device rd;
    thread_local std::mt19937 gen(rd());

    if constexpr (std::is_integral<T>::value) {
        std::uniform_int_distribution<T> dis(min, max);
        return dis(gen);
    } else {
        std::uniform_real_distribution<T> dis(min, max);
        return dis(gen);
    }
}

constexpr float radians(float deg) {
    return deg * PI / 180.f;
}

constexpr float degrees(float rad) {
    return rad * 180.f / PI;
}

std::vector<std::string> split(const std::string& str, char delimiter = ' ') {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(str);

    while (std::getline(stream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

} // namespace spt

#endif