#ifndef SPT_MAT_HPP
#define SPT_MAT_HPP

#include "Vec.hpp"

namespace spt {

/**
 * @brief RxC dimensional matrix class with column-major storage layout.
 * 
 * Follows GLM conventions: transforms column vectors via right-multiplication (M * v).
 * Composite transformations are applied right-to-left (e.g., T * R * S * v).
 * 
 * @tparam T Component arithmetic type
 * @tparam R Number of rows
 * @tparam C Number of columns
 */
template <arithmetic T, size_t R, size_t C>
struct Mat {
    Vec<T, R> cols[C];

    explicit Mat(T t = static_cast<T>(0)) {
        for (auto& c : cols) { c = Vec<T, R>(t); }
    }
    Mat(std::initializer_list<std::initializer_list<T>> rows) {
        if (rows.size() != R) { throw std::invalid_argument(std::format("Mat: initializer_list row count {} does not match matrix rows {}", rows.size(), R)); }

        for (int r = 0; const auto& row : rows) {
            if (row.size() != C) { throw std::invalid_argument(std::format("Mat: initializer_list col count {} at row {} does not match matrix cols {}", row.size(), r, C)); }
            for (int c = 0; const auto& val : row) {
                cols[c][r] = val;
                ++c;
            }
            ++r;
        }
    }

    static Mat zero() {
        Mat m;
        for (int i = 0; i < C; i++) { m.cols[i] = Vec<T, R>(0); }
        return m;
    }
    static Mat eye()
        requires(R == C)
    {
        Mat m;
        for (int i = 0; i < R; i++) { m.cols[i][i] = T(1); }
        return m;
    }

    Vec<T, R>& operator[](size_t c) {
        if (c >= C) { throw std::out_of_range("Mat::operator[]: c out of range"); }
        return cols[c];
    }
    const Vec<T, R>& operator[](size_t c) const {
        if (c >= C) { throw std::out_of_range("Mat::operator[]: c out of range"); }
        return cols[c];
    }

    T& operator()(size_t r, size_t c) {
        if (r >= R || c >= C) { throw std::out_of_range("Mat::operator():: r or c out of range"); }
        return cols[c][r];
    }
    const T& operator()(size_t r, size_t c) const {
        if (r >= R || c >= C) { throw std::out_of_range("Mat::operator():: r or c out of range"); }
        return cols[c][r];
    }
};

template <arithmetic T>
using Mat4x4 = Mat<T, 4, 4>;
template <arithmetic T>
using Mat3x3 = Mat<T, 3, 3>;
template <arithmetic T>
using Mat2x2 = Mat<T, 2, 2>;

using Mat4x4f = Mat4x4<float>;
using Mat3x3f = Mat3x3<float>;
using Mat2x2f = Mat2x2<float>;

template <arithmetic T, size_t M, size_t K, size_t N>
bool operator==(const Mat<T, M, K>& m1, const Mat<T, K, N>& m2) {
    for (int c = 0; c < N; ++c) {
        if (m1[c] != m2[c]) { return false; }
    }
    return true;
}

template <arithmetic T, size_t M, size_t K, size_t N>
bool operator!=(const Mat<T, M, K>& m1, const Mat<T, K, N>& m2) {
    return !(m1 == m2);
}

template <arithmetic T, size_t M, size_t K, size_t N>
Mat<T, M, N> operator*(const Mat<T, M, K>& m1, const Mat<T, K, N>& m2) {
    Mat<T, M, N> m(0);
    for (int c = 0; c < N; ++c) { m[c] = m1 * m2[c]; }
    return m;
}

template <arithmetic T, size_t R, size_t C>
Vec<T, R> operator*(const Mat<T, R, C>& m, const Vec<T, C>& v) {
    Vec<T, R> vm(0);
    for (int c = 0; c < C; c++) { vm += m[c] * v[c]; }
    return vm;
}

template <arithmetic T, size_t R, size_t C>
std::ostream& operator<<(std::ostream& os, const Mat<T, R, C>& m) {
    std::stringstream ss;
    ss << "Mat" << R << "x" << C << "(";
    for (int r = 0; r < R; r++) {
        ss << "(";
        for (int c = 0; c < C; c++) {
            ss << m(r, c);
            if (c < C - 1) { ss << ", "; }
        }
        ss << ")";
        if (r < R - 1) { ss << ", "; }
    }
    ss << ")";
    os << ss.str();
    return os;
}

template <arithmetic T, size_t R, size_t C>
Mat<T, C, R> transpose(const Mat<T, R, C>& m) {
    Mat<T, C, R> mt;
    for (int r = 0; r < R; r++) {
        for (int c = 0; c < C; c++) {
            mt(c, r) = m(r, c);
        }
    }
    return mt;
}

template <arithmetic T, size_t N>
T det(const Mat<T, N, N>& m) {
    // TODO: implement determinant calculation
    if constexpr (N == 1) {
        return m(0, 0);
    } else if constexpr (N == 2) {
        // 2x2 行列式: ad - bc
        return m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0);
    } else if constexpr (N == 3) {
        // 3x3 行列式 (Sarrus 法则 / 余子式展开)
        return m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) - m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) + m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));
    } else if constexpr (N == 4) {
        // 4x4 行列式 (按第一行展开余子式)
        T SubFactor00 = m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3);
        T SubFactor01 = m(2, 1) * m(3, 3) - m(3, 1) * m(2, 3);
        T SubFactor02 = m(2, 1) * m(3, 2) - m(3, 1) * m(2, 2);
        T SubFactor03 = m(2, 0) * m(3, 3) - m(3, 0) * m(2, 3);
        T SubFactor04 = m(2, 0) * m(3, 2) - m(3, 0) * m(2, 2);
        T SubFactor05 = m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1);

        T det2_0 = (m(1, 1) * SubFactor00 - m(1, 2) * SubFactor01 + m(1, 3) * SubFactor02);
        T det2_1 = -(m(1, 0) * SubFactor00 - m(1, 2) * SubFactor03 + m(1, 3) * SubFactor04);
        T det2_2 = (m(1, 0) * SubFactor01 - m(1, 1) * SubFactor03 + m(1, 3) * SubFactor05);
        T det2_3 = -(m(1, 0) * SubFactor02 - m(1, 1) * SubFactor04 + m(1, 2) * SubFactor05);

        return m(0, 0) * det2_0 + m(0, 1) * det2_1 + m(0, 2) * det2_2 + m(0, 3) * det2_3;
    } else {
        // 任意 N > 4 高维矩阵: 高斯消元法求行列式 O(N^3)
        Mat<T, N, N> temp = m;
        T determinant     = static_cast<T>(1);
        int sign          = 1;

        for (size_t i = 0; i < N; ++i) {
            // 选主元 Pivot
            size_t pivot = i;
            for (size_t j = i + 1; j < N; ++j) {
                if (std::abs(temp(j, i)) > std::abs(temp(pivot, i))) {
                    pivot = j;
                }
            }

            if (std::abs(temp(pivot, i)) < static_cast<T>(1e-9)) {
                return static_cast<T>(0); // 奇异矩阵，行列式为 0
            }

            if (pivot != i) {
                // 交换行，行列式变号
                for (size_t k = 0; k < N; ++k) {
                    std::swap(temp(i, k), temp(pivot, k));
                }
                sign = -sign;
            }

            determinant *= temp(i, i);

            for (size_t j = i + 1; j < N; ++j) {
                T factor = temp(j, i) / temp(i, i);
                for (size_t k = i + 1; k < N; ++k) {
                    temp(j, k) -= factor * temp(i, k);
                }
            }
        }
        return determinant * static_cast<T>(sign);
    }
}

template <arithmetic T, size_t N>
Mat<T, N, N> inv(const Mat<T, N, N>& m) {
    // TODO: implement inverse calculation
    if constexpr (N == 1) {
        Mat<T, 1, 1> res;
        res(0, 0) = static_cast<T>(1) / m(0, 0);
        return res;
    } else if constexpr (N == 2) {
        T d = det(m);
        assert(std::abs(d) > static_cast<T>(1e-9) && "Matrix is singular and cannot be inverted!");
        T inv_d = static_cast<T>(1) / d;

        Mat<T, 2, 2> res;
        res(0, 0) = m(1, 1) * inv_d;
        res(0, 1) = -m(0, 1) * inv_d;
        res(1, 0) = -m(1, 0) * inv_d;
        res(1, 1) = m(0, 0) * inv_d;
        return res;
    } else if constexpr (N == 3) {
        // 3x3 伴随矩阵法 (Adjugate Matrix)
        T d = det(m);
        assert(std::abs(d) > static_cast<T>(1e-9) && "Matrix is singular and cannot be inverted!");
        T inv_d = static_cast<T>(1) / d;

        Mat<T, 3, 3> res;
        res(0, 0) = (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) * inv_d;
        res(0, 1) = (m(0, 2) * m(2, 1) - m(0, 1) * m(2, 2)) * inv_d;
        res(0, 2) = (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * inv_d;

        res(1, 0) = (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) * inv_d;
        res(1, 1) = (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0)) * inv_d;
        res(1, 2) = (m(0, 2) * m(1, 0) - m(0, 0) * m(1, 2)) * inv_d;

        res(2, 0) = (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0)) * inv_d;
        res(2, 1) = (m(0, 1) * m(2, 0) - m(0, 0) * m(2, 1)) * inv_d;
        res(2, 2) = (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) * inv_d;
        return res;
    } else if constexpr (N == 4) {
        // 4x4 工业级展开 (与 GLM 保持完全相同的算法，大幅减少冗余乘法)
        T coef00 = m(2, 2) * m(3, 3) - m(3, 2) * m(2, 3);
        T coef02 = m(1, 2) * m(3, 3) - m(3, 2) * m(1, 3);
        T coef03 = m(1, 2) * m(2, 3) - m(2, 2) * m(1, 3);

        T coef04 = m(2, 1) * m(3, 3) - m(3, 1) * m(2, 3);
        T coef06 = m(1, 1) * m(3, 3) - m(3, 1) * m(1, 3);
        T coef07 = m(1, 1) * m(2, 3) - m(2, 1) * m(1, 3);

        T coef08 = m(2, 1) * m(3, 2) - m(3, 1) * m(2, 2);
        T coef10 = m(1, 1) * m(3, 2) - m(3, 1) * m(1, 2);
        T coef11 = m(1, 1) * m(2, 2) - m(2, 1) * m(1, 2);

        T coef12 = m(2, 0) * m(3, 3) - m(3, 0) * m(2, 3);
        T coef14 = m(1, 0) * m(3, 3) - m(3, 0) * m(1, 3);
        T coef15 = m(1, 0) * m(2, 3) - m(2, 0) * m(1, 3);

        T coef16 = m(2, 0) * m(3, 2) - m(3, 0) * m(2, 2);
        T coef18 = m(1, 0) * m(3, 2) - m(3, 0) * m(1, 2);
        T coef19 = m(1, 0) * m(2, 2) - m(2, 0) * m(1, 2);

        T coef20 = m(2, 0) * m(3, 1) - m(3, 0) * m(2, 1);
        T coef22 = m(1, 0) * m(3, 1) - m(3, 0) * m(1, 1);
        T coef23 = m(1, 0) * m(2, 1) - m(2, 0) * m(1, 1);

        Vec<T, 4> fac0(coef00, coef00, coef02, coef03);
        Vec<T, 4> fac1(coef04, coef04, coef06, coef07);
        Vec<T, 4> fac2(coef08, coef08, coef10, coef11);
        Vec<T, 4> fac3(coef12, coef12, coef14, coef15);
        Vec<T, 4> fac4(coef16, coef16, coef18, coef19);
        Vec<T, 4> fac5(coef20, coef20, coef22, coef23);

        Vec<T, 4> Vec0(m(1, 0), m(0, 0), m(0, 0), m(0, 0));
        Vec<T, 4> Vec1(m(1, 1), m(0, 1), m(0, 1), m(0, 1));
        Vec<T, 4> Vec2(m(1, 2), m(0, 2), m(0, 2), m(0, 2));
        Vec<T, 4> Vec3(m(1, 3), m(0, 3), m(0, 3), m(0, 3));

        Vec<T, 4> inv0(Vec1 * fac0 - Vec2 * fac1 + Vec3 * fac2);
        Vec<T, 4> inv1(Vec0 * fac0 - Vec2 * fac3 + Vec3 * fac4);
        Vec<T, 4> inv2(Vec0 * fac1 - Vec1 * fac3 + Vec3 * fac5);
        Vec<T, 4> inv3(Vec0 * fac2 - Vec1 * fac4 + Vec2 * fac5);

        Vec<T, 4> signA(+1, -1, +1, -1);
        Vec<T, 4> signB(-1, +1, -1, +1);

        Mat<T, 4, 4> inverse;
        // 假设 Mat 按行/按列下标构建，请注意 match 你的 (row, col) 存储规则
        for (size_t i = 0; i < 4; ++i) {
            inverse(0, i) = inv0[i] * signA[i];
            inverse(1, i) = inv1[i] * signB[i];
            inverse(2, i) = inv2[i] * signA[i];
            inverse(3, i) = inv3[i] * signB[i];
        }

        Vec<T, 4> row0(inverse(0, 0), inverse(1, 0), inverse(2, 0), inverse(3, 0));
        Vec<T, 4> dot0(m(0, 0) * row0[0], m(0, 1) * row0[1], m(0, 2) * row0[2], m(0, 3) * row0[3]);
        T dot1 = (dot0[0] + dot0[1]) + (dot0[2] + dot0[3]);

        assert(std::abs(dot1) > static_cast<T>(1e-9) && "Matrix is singular and cannot be inverted!");
        T oneOverDeterminant = static_cast<T>(1) / dot1;

        for (size_t r = 0; r < 4; ++r) {
            for (size_t c = 0; c < 4; ++c) {
                inverse(r, c) *= oneOverDeterminant;
            }
        }

        return inverse;
    } else {
        // 任意 N > 4 高维矩阵: 全选主元高斯-约当消元法 (Gauss-Jordan Elimination)
        Mat<T, N, N> A = m;
        Mat<T, N, N> I = Mat<T, N, N>::identity(); // 单位阵

        for (size_t i = 0; i < N; ++i) {
            // 找主元 Pivot
            size_t pivot = i;
            for (size_t j = i + 1; j < N; ++j) {
                if (std::abs(A(j, i)) > std::abs(A(pivot, i))) {
                    pivot = j;
                }
            }

            assert(std::abs(A(pivot, i)) > static_cast<T>(1e-9) && "Matrix is singular!");

            if (pivot != i) {
                for (size_t k = 0; k < N; ++k) {
                    std::swap(A(i, k), A(pivot, k));
                    std::swap(I(i, k), I(pivot, k));
                }
            }

            // 归一化主行
            T scale = A(i, i);
            for (size_t k = 0; k < N; ++k) {
                A(i, k) /= scale;
                I(i, k) /= scale;
            }

            // 消去其他行的该列元素
            for (size_t j = 0; j < N; ++j) {
                if (j != i) {
                    T factor = A(j, i);
                    for (size_t k = 0; k < N; ++k) {
                        A(j, k) -= factor * A(i, k);
                        I(j, k) -= factor * I(i, k);
                    }
                }
            }
        }
        return I;
    }
}

template <arithmetic T>
Mat<T, 4, 4> inv_affine(const Mat<T, 4, 4>& m) {
    // 1. 提取左上角 3x3 矩阵 A 和右侧平移向量 t
    // A = [ m(0,0) m(0,1) m(0,2) ]
    //     [ m(1,0) m(1,1) m(1,2) ]
    //     [ m(2,0) m(2,1) m(2,2) ]

    // 2. 手动展开计算 3x3 矩阵 A 的行列式 det(A)
    T detA = m(0, 0) * (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) - m(0, 1) * (m(1, 0) * m(2, 2) - m(1, 2) * m(2, 0)) + m(0, 2) * (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0));

    assert(std::abs(detA) > static_cast<T>(1e-9) && "Affine matrix A is singular!");
    T invDetA = static_cast<T>(1) / detA;

    // 3. 计算 3x3 逆矩阵 A_inv (伴随矩阵 / detA)
    Mat<T, 3, 3> A_inv;
    A_inv(0, 0) = (m(1, 1) * m(2, 2) - m(1, 2) * m(2, 1)) * invDetA;
    A_inv(0, 1) = (m(0, 2) * m(2, 1) - m(0, 1) * m(2, 2)) * invDetA;
    A_inv(0, 2) = (m(0, 1) * m(1, 2) - m(0, 2) * m(1, 1)) * invDetA;

    A_inv(1, 0) = (m(1, 2) * m(2, 0) - m(1, 0) * m(2, 2)) * invDetA;
    A_inv(1, 1) = (m(0, 0) * m(2, 2) - m(0, 2) * m(2, 0)) * invDetA;
    A_inv(1, 2) = (m(0, 2) * m(1, 0) - m(0, 0) * m(1, 2)) * invDetA;

    A_inv(2, 0) = (m(1, 0) * m(2, 1) - m(1, 1) * m(2, 0)) * invDetA;
    A_inv(2, 1) = (m(0, 1) * m(2, 0) - m(0, 0) * m(2, 1)) * invDetA;
    A_inv(2, 2) = (m(0, 0) * m(1, 1) - m(0, 1) * m(1, 0)) * invDetA;

    // 4. 计算新的平移向量: t_inv = -A_inv * t
    Vec<T, 3> t(m(0, 3), m(1, 3), m(2, 3));
    Vec<T, 3> t_inv = -Vec<T, 3>(
        A_inv(0, 0) * t.x + A_inv(0, 1) * t.y + A_inv(0, 2) * t.z,
        A_inv(1, 0) * t.x + A_inv(1, 1) * t.y + A_inv(1, 2) * t.z,
        A_inv(2, 0) * t.x + A_inv(2, 1) * t.y + A_inv(2, 2) * t.z);

    // 5. 组装最终的 4x4 逆矩阵
    Mat<T, 4, 4> res;
    // 拷贝 3x3 逆矩阵
    for (size_t r = 0; r < 3; ++r) {
        for (size_t c = 0; c < 3; ++c) {
            res(r, c) = A_inv(r, c);
        }
    }
    // 填入新的平移向量
    res(0, 3) = t_inv.x;
    res(1, 3) = t_inv.y;
    res(2, 3) = t_inv.z;

    // 底行固定为 0, 0, 0, 1
    res(3, 0) = static_cast<T>(0);
    res(3, 1) = static_cast<T>(0);
    res(3, 2) = static_cast<T>(0);
    res(3, 3) = static_cast<T>(1);

    return res;
}

} // namespace spt

#endif