#include "objmodel.h"
#include "tgaimage.h"
#include "vec.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <numbers>
#include <print>
#include <ranges>
#include <sys/types.h>

constexpr TGAColor white = {{255, 255, 255, 255}}; // attention, BGRA order
constexpr TGAColor green = {{0, 255, 0, 255}};
constexpr TGAColor red = {{0, 0, 255, 255}};
constexpr TGAColor blue = {{255, 128, 64, 255}};
constexpr TGAColor yellow = {{0, 200, 255, 255}};

using tinyrenderer::mat;
using tinyrenderer::vec2, tinyrenderer::vec3;

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    const bool steep = std::abs(bx - ax) < std::abs(by - ay);
    if (steep) { // if line is 'more vertical', pretend it's not (swap x and y)
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax > bx) { // always draw left to right, otherwise the 'for' loop below doesn't run
        std::swap(ax, bx);
        std::swap(ay, by);
    }
    bool drawing_down = ay > by;

    const int dx = bx - ax, dy = std::abs(by - ay);
    int ierror = 0;
    int y = ay;
    for (int x = ax; x <= bx; ++x) {
        if (steep) {
            framebuffer.set(y, x, color);
        } else {
            framebuffer.set(x, y, color);
        }

        ierror += 2 * dy;
        if (ierror > dx) {
            y += drawing_down ? -1 : 1; // we might be drawing up -> down instead. This won't work.
                                        // (dy might be < 0)
            ierror -= 2 * dx;
        }
    }
}

void rect(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    for (int x = ax; x <= bx; ++x) {
        for (int y = ay; y <= by; ++y) {
            framebuffer.set(x, y, color);
        }
    }
}

std::tuple<std::pair<int, int>, std::pair<int, int>> bounding_box(int ax, int ay, int bx, int by,
                                                                  int cx, int cy) {
    return {{std::min({ax, bx, cx}), std::min({ay, by, cy})},
            {std::max({ax, bx, cx}), std::max({ay, by, cy})}};
}

double signed_triangle_area(int ax, int ay, int bx, int by, int cx, int cy) {
    return -.5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) + (ay - cy) * (ax + cx));
}

TGAColor color_lerp(const TGAColor &ac, const TGAColor &bc, const TGAColor &cc, double a, double b,
                    double c) {
    uint8_t out_b = ac.bgra[0] * a + bc.bgra[0] * b + cc.bgra[0] * c;
    uint8_t out_g = ac.bgra[1] * a + bc.bgra[1] * b + cc.bgra[1] * c;
    uint8_t out_r = ac.bgra[2] * a + bc.bgra[2] * b + cc.bgra[2] * c;

    return TGAColor{{out_b, out_g, out_r, 0}};
}

void filled_triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz,
                     TGAImage &framebuffer, std::vector<std::vector<double>> &depthbuffer,
                     TGAColor color) {
    // Draw using a two-part method:
    // 1 - Compute bounding box of the triangle
    // 2 - For each pixel in the bounding box, compute whether it is
    //     inside the triangle. If yes, then draw it.

    // 1 - Bounding box
    const auto &[mins, maxs] = bounding_box(ax, ay, bx, by, cx, cy);
    auto [min_x, min_y] = mins;
    auto [max_x, max_y] = maxs;

    // 2 - check if pixel is inside triangle
    int ab_x = bx - ax, ab_y = by - ay;
    int bc_x = cx - bx, bc_y = cy - by;

    int total_area = ab_x * bc_y - ab_y * bc_x; // twice the area of the triangle.
    if (total_area < 1)
        return;
    for (int x = std::max(min_x, 0); x <= std::min(max_x, framebuffer.width() - 1); ++x) {
        for (int y = std::max(min_y, 0); y <= std::min(max_y, framebuffer.height() - 1); ++y) {
            // P = (x, y). All areas below are also twice their true value.
            // Not an issue though, they get divided by `total_area` which also
            // holds a duplicated value.
            int area_opp_a = (y - by) * (cx - bx) - (x - bx) * (cy - by); // cross between BP and BC
            int area_opp_b = (y - cy) * (ax - cx) - (x - cx) * (ay - cy); // cross between CP and CA
            int area_opp_c = (y - ay) * (bx - ax) - (x - ax) * (by - ay); // cross between AP and AB

            // double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            // double beta  = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            // double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;

            bool has_neg = area_opp_a < 0 || area_opp_b < 0 || area_opp_c < 0;
            bool has_pos = true || area_opp_a > 0 || area_opp_a > 0 || area_opp_a > 0;
            if (has_neg && has_pos)
                continue;

            double alpha = static_cast<double>(area_opp_a) / total_area,
                   beta = static_cast<double>(area_opp_b) / total_area,
                   gamma = static_cast<double>(area_opp_c) / total_area;
            double depth = (alpha * az + beta * bz + gamma * cz); // TODO: fix alpha/beta/gamma
            if (depth > depthbuffer[x][y]) {
                framebuffer.set(x, y, color);
                depthbuffer[x][y] = depth;
                // depthbuffer.set(x, y, TGAColor({{(uint8_t)depth_u, depth_u, depth_u, depth_u}}));
            }
        }
    }
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer,
              TGAColor color) {
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);
}

vec3<double> rot(vec3<double> v) {
    double angle = std::numbers::pi / 6;
    mat<double, 3, 3> rot = {
        {std::cos(angle), 0, std::sin(angle), 0, 1, 0, -std::sin(angle), 0, std::cos(angle)}};
    return rot.matmul(v);
}

vec3<double> persp(const vec3<double> &in) {
    constexpr double c = 3.0;
    return in / (1 - in[2] / c);
}

Tup3<int> project(const vec3<double> &in, int w, int h) {
    // Obj coordinates go from -1 to +1. We want to map this to [0, w], [0, h]

    int out_x = std::round((in[0] + 1.0) * w / 2);
    int out_y = std::round((in[1] + 1.0) * h / 2);
    int out_z = std::round((in[2] + 1.0) * 256 / 2);

    return {out_x, out_y, out_z};
}

void convert_to_tga(const std::vector<std::vector<double>> &depthbuffer, TGAImage &out) {
    auto flat_view = depthbuffer | std::views::join;
    double max = std::ranges::max(flat_view);
    std::println("{}", max);
    for (int y = 0; y < out.height(); ++y) {
        for (int x = 0; x < out.width(); ++x) {
            out.set(x, y, {{(uint8_t)std::round(255 * depthbuffer[x][y] / max)}});
        }
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }

    constexpr int width = 2048;
    constexpr int height = 2048;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    std::vector<std::vector<double>> depthbuffer(width, std::vector<double>(height, 0.0));

    ObjModel model;
    model.Load(argv[1]);

    for (int i = 0; i < model.faces.size(); ++i) {
        auto a = project(persp(rot(model.vertices[model.faces[i].x])), width, height);
        auto b = project(persp(rot(model.vertices[model.faces[i].y])), width, height);
        auto c = project(persp(rot(model.vertices[model.faces[i].z])), width, height);

        TGAColor rnd;
        for (size_t i : {0, 1, 2}) {
            rnd[i] = std::rand() % 256;
        }
        filled_triangle(a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z, framebuffer, depthbuffer, rnd);
    }
    std::println("Done drawing");
    framebuffer.write_tga_file("framebuffer.tga");
    TGAImage depthbuffer_tga(width, height, TGAImage::GRAYSCALE);
    std::println("converting to TGA");
    convert_to_tga(depthbuffer, depthbuffer_tga);
    depthbuffer_tga.write_tga_file("depthbuffer.tga");

    tinyrenderer::vec2<double> var({1, 2});
    std::println("{}", var.data());
    tinyrenderer::vec2<double> out = var + 1;
    std::println("{}", out.data());
    std::println("a . b = {}", var.dot(out));

    auto mat = tinyrenderer::outer(var, out);
    std::println("{}", mat.data());
    tinyrenderer::mat<double, 3, 3> mat2 = {{1.0, 2.0, 3.0, 4.0, 5.0, 6.0, -7., 8.0, 9.0}};
    auto mat2_inv = mat2.inv();
    std::println(
        "{}",
        mat2.matmul(mat2_inv).matmul(tinyrenderer::mat<double, 3, 1>({1.0, 2.0, 3.0})).t().data());
    std::println("{}", (-tinyrenderer::mat<int, 3, 2>{{1, 2, 3, 4, 5, 6}}.t()).data());
    return 0;
}
