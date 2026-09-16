#pragma once

#include <vector>
#include <string_view>

template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true> struct Tup3 {
    T x = 0.0, y = 0.0, z = 0.0;
};

using Vec3 = Tup3<float>;
using Ind3 = Tup3<size_t>;

struct ObjModel {
    ObjModel() = default;

    bool Load(std::string_view path);

    std::vector<Vec3> vertices;
    std::vector<Ind3> faces;
};