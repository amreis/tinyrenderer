#pragma once

#include <string_view>
#include <vector>

#include "vec.h"

template <typename T, std::enable_if_t<std::is_arithmetic<T>::value, bool> = true> struct Tup3 {
    T x = 0.0, y = 0.0, z = 0.0;
};

using Ind3 = Tup3<size_t>;

struct ObjModel {
    ObjModel() = default;

    bool Load(std::string_view path);

    std::vector<tinyrenderer::vec3<double>> vertices;
    std::vector<Ind3> faces;
};
