#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <print>
#include <ranges>
#include <type_traits>
#include <valarray>

namespace tinyrenderer {

struct none_t {};

template <bool Enable, typename T>
using conditional_member_t = std::conditional_t<Enable, T, none_t>;

template <typename T, std::size_t R, std::size_t C,
          typename = std::enable_if_t<std::is_arithmetic_v<T>>>
class mat;

template <typename T, std::size_t N, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
class vec {
    using container = std::valarray<T>;
    using Self = vec<T, N>;

    container _data;
    friend mat<T, N, N>;
    friend mat<T, N, 1>;

    // vec(const std::valarray<T> &array) : _data(array) {};

  public:
    vec() { _data.resize(N, 0); };
    template <typename... Ts>
    explicit vec(Ts... args)
        requires(sizeof...(Ts) == N && (std::is_same_v<T, Ts> && ...))
        : vec({args...}) {} // enables emplace_back with bare scalars
    vec(const vec &other) : _data(other._data) {};
    vec(vec &&other) : _data(other._data) {};
    vec(std::array<T, N> list) : _data(list.begin(), N) {}
    vec(mat<T, N, 1> mat) : _data(mat._data) {}
    const std::valarray<T> &data() const { return _data; }

    template <typename Self> auto &&operator[](this Self &&self, std::size_t i) {
        assert(i >= 0 && i <= N);
        return std::forward<Self>(self)._data[i];
    }

    T dot(const Self &other) const {
        return std::ranges::fold_left(std::views::zip(_data, other._data) |
                                          std::views::transform([](auto &&tuple) {
                                              const auto &[fst, snd] = tuple;
                                              return fst * snd;
                                          }),
                                      (T)0, std::plus<T>());
    }

    Self cross(const Self &other) const
        requires(N == 3)
    {
        const auto &[a_x, a_y, a_z] = this->_data;
        const auto &[b_x, b_y, b_z] = other._data;

        return {a_y * b_z - a_z * b_y, a_z * b_x - a_x * b_z, a_x * b_y - a_y * b_x};
    }

    // In-place addition with another vecN.
    Self &operator+=(const Self &rhs) {
        _data += rhs._data;
        return *this;
    }
    // Addition with another vecN
    friend Self operator+(Self lhs, const Self &rhs) {
        lhs += rhs;
        return lhs;
    }
    // In-place addition with scalar.
    Self &operator+=(T rhs) {
        _data += rhs;
        return *this;
    }
    // Addition with scalar.
    friend Self operator+(Self lhs, const T &rhs) {
        lhs += rhs;
        return lhs;
    }
    Self &operator*=(const Self &rhs) {
        _data += rhs._data;
        return *this;
    }
    friend Self operator*(Self lhs, const Self &rhs) {
        lhs *= rhs;
        return lhs;
    }
    Self &operator*=(T rhs) {
        _data *= rhs;
        return *this;
    }
    friend Self operator*(Self lhs, T rhs) {
        lhs *= rhs;
        return lhs;
    }
    Self &operator/=(const Self &rhs) {
        _data /= rhs._data;
        return *this;
    }
    friend Self operator/(Self lhs, const Self &rhs) {
        lhs /= rhs;
        return lhs;
    }
    Self &operator/=(T rhs) {
        _data /= rhs;
        return *this;
    }
    friend Self operator/(Self lhs, T rhs) {
        lhs /= rhs;
        return lhs;
    }
    Self operator-() const {
        Self other{-this->_data};
        return other;
    }
    Self operator+() const { return *this; }
};

template <typename T> using vec2 = vec<T, 2>;
template <typename T> using vec3 = vec<T, 3>;
template <typename T> using vec4 = vec<T, 4>;

template <typename T, std::size_t N>
mat<T, N, N> outer(const vec<T, N> &lhs, const vec<T, N> &rhs) {
    std::array<T, N * N> mat_data = {};
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            mat_data[N * i + j] = lhs.data()[i] * rhs.data()[j];
        }
    }
    return {mat_data};
}

template <std::size_t R, std::size_t C, std::size_t N>
concept square = (R == C && C == N);

template <typename T, std::size_t R, std::size_t C, typename> class mat {
    using container = std::valarray<T>;
    using Self = mat<T, R, C>;

    container _data;
    // mat(const std::valarray<T> &array) : _data(array) {};

    friend vec<T, R>;
    friend mat<T, C, R>;

  public:
    mat() { _data.resize(R * C, 0); };
    mat(const mat &other) : _data(other._data) {};
    mat(mat &&other) : _data(other._data) {};
    mat(std::array<T, R * C> list) : _data(list.begin(), R * C) {
        // for (std::size_t i = 0; i < list.size(); ++i) {
        //     _data[i] = list[i];
        // }
    }
    template <std::size_t N> mat<T, N, 1>(const vec<T, N> &v) : _data(v._data) {}

    T &operator[](std::size_t linear_idx) { return _data[linear_idx]; }
    const T &operator[](std::size_t linear_idx) const { return _data[linear_idx]; }
    T &operator[](std::size_t row, std::size_t col) { return _data[C * row + col]; }
    const T &operator[](std::size_t row, std::size_t col) const { return _data[C * row + col]; }

    mat<T, C, R> t() const {
        std::valarray<T> new_data((T)0, _data.size());
        for (int i = 0; i < R; ++i) {
            new_data[std::slice(i, C, R)] = _data[std::slice(i * C, C, 1)];
        }
        mat<T, C, R> new_mat;
        new_mat._data = new_data;
        return new_mat;
    }

    template <typename T2, std::size_t C2>
    mat<std::common_type_t<T, T2>, R, C2> matmul(const mat<T2, C, C2> &other) {
        using TOut = std::common_type_t<T, T2>;

        mat<TOut, R, C2> output;
        for (int i = 0; i < R; ++i) {
            for (int j = 0; j < C2; ++j) {
                for (int k = 0; k < C; ++k) {
                    output[i, j] +=
                        static_cast<TOut>((*this)[i, k]) * static_cast<TOut>(other[k, j]);
                }
            }
        }
        return output;
    }
    template <typename T2> mat<std::common_type_t<T, T2>, R, 1> matmul(const vec<T2, C> &other) {
        return this->matmul(mat<T2, C, 1>(other));
    }

    const std::valarray<T> &data() const { return _data; }

    vec<T, R> as_vec() const
        requires(C == 1)
    {
        return vec<T, R>(_data);
    }

    // In-place addition with another vecN.
    Self &operator+=(const Self &rhs) {
        _data += rhs._data;
        return *this;
    }
    // Addition with another vecN
    friend Self operator+(Self lhs, const Self &rhs) {
        lhs += rhs;
        return lhs;
    }
    // In-place addition with scalar.
    Self &operator+=(T rhs) {
        _data += rhs;
        return *this;
    }
    // Addition with scalar.
    friend Self operator+(Self lhs, const T &rhs) {
        lhs += rhs;
        return lhs;
    }
    Self &operator*=(const Self &rhs) {
        _data += rhs._data;
        return *this;
    }
    friend Self operator*(Self lhs, const Self &rhs) {
        lhs *= rhs;
        return lhs;
    }
    Self &operator*=(T rhs) {
        _data *= rhs;
        return *this;
    }
    friend Self operator*(Self lhs, T rhs) {
        lhs *= rhs;
        return lhs;
    }
    friend Self operator*(T lhs, Self rhs) {
        rhs *= lhs;
        return rhs;
    }
    Self &operator/=(const Self &rhs) {
        _data /= rhs._data;
        return *this;
    }
    friend Self operator/(Self lhs, const Self &rhs) {
        lhs /= rhs;
        return lhs;
    }
    Self &operator/=(T rhs) {
        _data /= rhs;
        return *this;
    }
    friend Self operator/(Self lhs, T rhs) {
        lhs /= rhs;
        return lhs;
    }
    Self operator-() const {
        Self other;
        other._data = -this->data();
        return other;
    }
    Self operator+() const { return *this; }

    mat<double, R, C> inv(this Self const &self)
        requires(R == C && C == 2)
    {
        const auto &a = self[0, 0], b = self[0, 1], c = self[1, 0], d = self[1, 1];
        double inv_scaling = self.det();
        return (1.0 / inv_scaling) * mat<double, R, C>({d, -b, -c, a});
    }
    double det() const
        requires(square<R, C, 2>)
    {
        const mat<T, R, C> &self = *this;
        return self[0, 0] * self[1, 1] - self[0, 1] * self[1, 0];
    }
    double det(this Self const &self)
        requires(square<R, C, 3>)
    {
        double positive = self[0, 0] * self[1, 1] * self[2, 2] +
                          self[0, 1] * self[1, 2] * self[2, 0] +
                          self[0, 2] * self[1, 0] * self[2, 1];
        double negative = self[0, 2] * self[1, 1] * self[2, 0] +
                          self[0, 0] * self[1, 2] * self[2, 1] +
                          self[0, 1] * self[1, 0] * self[2, 2];
        return positive - negative;
    }
    mat<double, R, C> inv(this Self const &self)
        requires(square<R, C, 3>)
    {
        mat<double, R, C> adjugate;

        adjugate[0, 0] = self[1, 1] * self[2, 2] - self[2, 1] * self[1, 2];
        adjugate[0, 1] = self[0, 2] * self[2, 1] - self[2, 2] * self[0, 1];
        adjugate[0, 2] = self[0, 1] * self[1, 2] - self[1, 1] * self[0, 2];
        adjugate[1, 0] = self[1, 2] * self[2, 0] - self[2, 2] * self[1, 0];
        adjugate[1, 1] = self[0, 0] * self[2, 2] - self[2, 0] * self[0, 2];
        adjugate[1, 2] = self[0, 2] * self[1, 0] - self[1, 2] * self[0, 0];
        adjugate[2, 0] = self[1, 0] * self[2, 1] - self[2, 0] * self[1, 1];
        adjugate[2, 1] = self[0, 1] * self[2, 0] - self[2, 1] * self[0, 0];
        adjugate[2, 2] = self[0, 0] * self[1, 1] - self[1, 0] * self[0, 1];

        return (1 / self.det()) * adjugate;
    }
};
} // namespace tinyrenderer