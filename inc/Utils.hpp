#ifndef SPT_UTILS_HPP
#define SPT_UTILS_HPP

#include <cassert>
#include <cmath>
#include <ostream>
#include <random>
#include <sstream>
#include <vector>

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

static std::vector<std::string> split(const std::string& str, char delimiter = ' ') {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);

    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }

    return tokens;
}

} // namespace spt

#endif