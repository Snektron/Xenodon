#ifndef _XENODON_MATH_MAT_H
#define _XENODON_MATH_MAT_H

#include <array>
#include <algorithm>
#include "math/Vec.h"

template <typename T, size_t M, size_t N>
struct Mat;

template <typename T>
using Mat2 = Mat<T, 2, 2>;
using Mat2F = Mat2<float>;
using Mat2D = Mat2<double>;
using Mat2I = Mat2<int>;

template <typename T>
using Mat3 = Mat<T, 3, 3>;
using Mat3F = Mat3<float>;
using Mat3D = Mat3<double>;
using Mat3I = Mat3<int>;

template <typename T>
using Mat4 = Mat<T, 4, 4>;
using Mat4F = Mat4<float>;
using Mat4D = Mat4<double>;
using Mat4I = Mat4<int>;

template <typename T, size_t M, size_t N>
struct BaseMat {
    union {
        std::array<T, M * N> elements;
        std::array<Vec<T, M>, N> columns;
    };

    constexpr BaseMat():
        elements{0} {
    }

    constexpr BaseMat(const T& diag):
        elements{0} {
        for (size_t i = 0; i < std::min(M, N); ++i)
            this->columns[i](i) = diag;
    }

    template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == N && (M > 1)>>
    constexpr BaseMat(const Vec<Args, M>&... args):
        columns{static_cast<Vec<Args, M>>(args)...} {
    }

    template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == M * N>>
    constexpr BaseMat(Args&&... args):
        elements{static_cast<T>(std::forward<Args>(args))...} {
    }
};

template <typename T, size_t N>
struct BaseMat<T, 1, N> {
    union {
        std::array<T, N> elements;
    };

    constexpr BaseMat():
        elements{0} {
    }

    constexpr BaseMat(const T& diag):
        elements{0} {
        this->elements[0] = diag;
    }

    template <typename... Args, typename = std::enable_if_t<sizeof...(Args) == N>>
    constexpr BaseMat(Args&&... args):
        elements{static_cast<T>(std::forward<Args>(args))...} {
    }
};

template <typename T, size_t M, size_t N>
struct Mat: BaseMat<T, M, N> {
    // static_assert(N > 1, "Cannot create column matrix, use a vector instead");

    constexpr const static size_t Rows = M;
    constexpr const static size_t Cols = N;

    using Base = BaseMat<T, M, N>;
    using Base::Base;

    constexpr Mat():
        Base() {
    }

    template <typename U>
    constexpr Mat(const Mat<U, M, N>& other) {
        std::transform(other.begin(), other.end(), this->begin(), [](const auto& elem){
            return static_cast<T>(elem);
        });
    }

    constexpr static auto translation(const Vec3<T>& translation);

    constexpr static auto translation(const T& x, const T& y, const T& z);

    constexpr static auto scaling(const Vec3<T>& translation);

    constexpr static auto scaling(const T& x, const T& y, const T& z);

    constexpr static auto axis_angle(const Vec3<T>& axis, const T& angle);

    constexpr static auto axis_angle(const T& x, const T& y, const T& z, const T& angle);

    constexpr static auto orthographic(const T& left, const T& right, const T& top, const T& bottom, const T& znear, const T& zfar);

    constexpr static auto perspective(const T& aspect, const T& fov, const T& znear, const T& zfar);

    constexpr static auto look(const Vec3<T>& eye, const Vec3<T>& dir, const Vec3<T>& up);

    constexpr static auto look_at(const Vec3<T>& eye, const Vec3<T>& target, const Vec3<T>& up);

    template <typename F>
    constexpr static Mat<T, M, N> generate(F f) {
        Mat<T, M, N> m;
        for (size_t j = 0; j < N; ++j)
            for (size_t i = 0; i < M; ++i)
                m(i, j) = f(i, j);
        return m;
    }

    constexpr static Mat<T, M, N> identity() {
        return Mat<T, M, N>(1);
    }

    constexpr static Mat<T, M, N> zero() {
        return Mat<T, M, N>(0);
    }

    constexpr Vec<T, M>& column(size_t i) {
        return this->columns[i];
    }

    constexpr const Vec<T, M>& column(size_t i) const {
        return this->columns[i];
    }

    constexpr T& operator()(size_t i, size_t j) {
        return this->columns[j](i);
    }

    constexpr const T& operator()(size_t i, size_t j) const {
        return this->columns[j](i);
    }

    constexpr T& operator[](size_t i) {
        return this->elements[i];
    }

    constexpr const T& operator[](size_t i) const {
        return this->elements[i];
    }

    constexpr const T* data() const {
        return this->elements.data();
    }

    constexpr auto begin() {
        return this->elements.begin();
    }

    constexpr auto end() {
        return this->elements.end();
    }

    constexpr auto begin() const {
        return this->elements.begin();
    }

    constexpr auto end() const {
        return this->elements.end();
    }

    template <typename U>
    constexpr Mat<T, M, N>& operator+=(const U& other);

    template <typename U>
    constexpr Mat<T, M, N>& operator-=(const U& other);

    template <typename U>
    constexpr Mat<T, M, N>& operator*=(const U& other);
};

template <size_t M, size_t N, typename F>
constexpr auto generate_mat(F f) {
    using Result = std::result_of_t<F(size_t, size_t)>;
    return Mat<Result, M, N>::generate(f);
}

template <typename Os, typename T, size_t M, size_t N>
Os& operator<<(Os& os, const Mat<T, M, N>& mat) {
    os.precision(3);
    for (size_t i = 0; i < M; ++i) {
        for (size_t j = 0; j < N; ++j) {
            os << mat(i, j);
            if (j != N - 1)
                os << '\t';
        }
        if (i != M - 1)
            os << '\n';
    }

    return os;
}

template <typename T, typename U, size_t M, size_t N>
constexpr auto operator+(const Mat<T, M, N>& lhs, const Mat<U, M, N>& rhs) {
    return generate_mat<M, N>([&](size_t i, size_t j) {
        return lhs(i, j) + rhs(i, j);
    });
}

template <typename T, typename U, size_t M, size_t N>
constexpr auto operator+(const T& lhs, const Mat<U, M, N>& rhs) {
    return generate_mat<M, N>([&](size_t i, size_t j) {
        return lhs + rhs(i, j);
    });
}

template <typename T, typename U, size_t M, size_t N>
constexpr auto operator+(const Mat<T, M, N>& lhs, const U& rhs) {
    return generate_mat<M, N>([&](size_t i, size_t j) {
        return lhs(i, j) + rhs;
    });
}

template <typename T, typename U, size_t M, size_t N>
constexpr auto operator-(const Mat<T, M, N>& lhs, const Mat<U, M, N>& rhs) {
    return generate_mat<M, N>([&](size_t i, size_t j) {
        return lhs(i, j) - rhs(i, j);
    });
}

template <typename T, typename U, size_t M, size_t N>
constexpr auto operator-(const T& lhs, const Mat<U, M, N>& rhs) {
    return generate_mat<M, N>([&](size_t i, size_t j) {
        return lhs - rhs(i, j);
    });
}

template <typename T, typename U, size_t M, size_t N>
constexpr auto operator-(const Mat<T, M, N>& lhs, const U& rhs) {
    return generate_mat<M, N>([&](size_t i, size_t j) {
        return lhs(i, j) - rhs;
    });
}

template <typename T, typename U, size_t M, size_t N, size_t O>
constexpr auto operator*(const Mat<T, M, N>& lhs, const Mat<U, N, O>& rhs) {
    using Result = decltype(std::declval<T>() * std::declval<U>() + std::declval<T>() * std::declval<U>());
    Mat<Result, M, O> result;

    for (size_t k = 0; k < N; ++k) {
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < O; ++j)
                result(i, j) += lhs(i, k) * rhs(k, j);
        }
    }

    return result;
}

template <typename T, typename U, size_t M, size_t N>
constexpr auto operator*(const T& lhs, const Mat<U, M, N>& rhs) {
    return generate_mat<M, N>([&](size_t i, size_t j) {
        return lhs * rhs(i, j);
    });
}

template <typename T, typename U, size_t M, size_t N>
constexpr auto operator*(const Mat<T, M, N>& lhs, const U& rhs) {
    return generate_mat<M, N>([&](size_t i, size_t j) {
        return lhs(i, j) * rhs;
    });
}

template <typename T, size_t M, size_t N>
constexpr auto operator-(const Mat<T, M, N>& m) {
    return generate_mat<M, N>([&](size_t i, size_t j) {
        return -m(i, j);
    });
}

template <typename T, size_t M, size_t N>
constexpr auto transpose(const Mat<T, M, N>& m) {
    return generate_mat<N, M>([&](size_t i, size_t j) {
        return m(j, i);
    });
}

template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::translation(const Vec3<T>& translation) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 translation matrix");
    return Mat<T, M, N>::translation(translation.x, translation.y, translation.z);
}

template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::translation(const T& x, const T& y, const T& z) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 translation matrix");
    auto result = Mat4<T>::identity();
    result(0, 3) = x;
    result(1, 3) = y;
    result(2, 3) = z;
    return result;
}

template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::scaling(const Vec3<T>& scaling) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 scaling matrix");
    return Mat<T, M, N>::scaling(scaling.x, scaling.y, scaling.z);
}

template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::scaling(const T& x, const T& y, const T& z) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 scaling matrix");
    Mat4<T> result;
    result(0, 0) = x;
    result(1, 1) = y;
    result(2, 2) = z;
    result(3, 3) = 1;
    return result;
}

template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::axis_angle(const Vec3<T>& axis, const T& angle) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 rotation matrix");
    return Mat<T, M, N>::axis_angle(axis.x, axis.y, axis.z, angle);
}

// See https://en.wikipedia.org/wiki/Rotation_matrix
template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::axis_angle(const T& x, const T& y, const T& z, const T& angle) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 rotation matrix");
    Mat4<T> result;
    auto c = std::cos(angle);
    auto ci = 1 - c;
    auto s = std::sin(angle);

    result(0, 0) = x * x * ci + c;
    result(1, 0) = x * y * ci + z * s;
    result(2, 0) = x * z * ci - y * s;

    result(0, 1) = x * y * ci - z * s;
    result(1, 1) = y * y * ci + c;
    result(2, 1) = y * z * ci + x * s;

    result(0, 2) = x * z * ci + y * s;
    result(1, 2) = y * z * ci - x * s;
    result(2, 2) = z * z * ci + c;

    result(3, 3) = 1;
    return result;
}

// See https://en.wikipedia.org/wiki/Orthographic_projection
template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::orthographic(const T& left, const T& right, const T& top, const T& bottom, const T& znear, const T& zfar) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 orthographic projection matrix");
    Mat4<T> result;

    auto rl = right - left;
    auto tb = top - bottom;
    auto nf = znear - zfar;

    result(0, 0) = 2 / rl;
    result(1, 1) = 2 / tb;
    result(2, 2) = 2 / nf;
    result(0, 3) = -(right + left) / rl;
    result(1, 3) = -(top + bottom) / tb;
    result(2, 3) = -(znear + zfar) / nf;
    result(3, 3) = 1;

    return result;
}

// See http://ogldev.atspace.co.uk/www/tutorial12/tutorial12.html
template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::perspective(const T& aspect, const T& fov, const T& znear, const T& zfar) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 perspective projection matrix");
    Mat4<T> result;

    auto nf = znear - zfar;
    auto tan_fov_2 = std::tan(fov / 2);

    result(0, 0) = 1 / (aspect * tan_fov_2);
    result(1, 1) = 1 / tan_fov_2;
    result(2, 2) = (-znear - zfar) / nf;
    result(2, 3) = (2 * zfar * znear) / nf;
    result(3, 2) = 1;

    return result;
}

// Code taken from glm lookAt function
template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::look(const Vec3<T>& eye, const Vec3<T>& dir, const Vec3<T>& up) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 view matrix");
    auto s = normalize(cross(dir, up));
    auto u = cross(s, dir);

    Mat4<T> result(1.0);

    result(0, 0) = s.x;
    result(0, 1) = s.y;
    result(0, 2) = s.z;
    result(1, 0) = u.x;
    result(1, 1) = u.y;
    result(1, 2) = u.z;
    result(2, 0) = -dir.x;
    result(2, 1) = -dir.y;
    result(2, 2) = -dir.z;
    result(0, 3) = -dot(s, eye);
    result(1, 3) = -dot(u, eye);
    result(2, 3) = dot(dir, eye);

    return result;
}

template <typename T, size_t M, size_t N>
constexpr auto Mat<T, M, N>::look_at(const Vec3<T>& eye, const Vec3<T>& target, const Vec3<T>& up) {
    static_assert(M == 4 && N == 4, "Can only create a 4x4 view matrix");
    return look(eye, normalize(target - eye), up);
}

template <typename T, size_t M, size_t N>
template <typename U>
constexpr Mat<T, M, N>& Mat<T, M, N>::operator+=(const U& other) {
    return *this = *this + other;
}

template <typename T, size_t M, size_t N>
template <typename U>
constexpr Mat<T, M, N>& Mat<T, M, N>::operator-=(const U& other) {
    return *this = *this - other;
}

template <typename T, size_t M, size_t N>
template <typename U>
constexpr Mat<T, M, N>& Mat<T, M, N>::operator*=(const U& other) {
    return *this = *this * other;
}

#endif
