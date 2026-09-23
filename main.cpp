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

vec3<double> rot(vec3<double> v) {
    double angle = std::numbers::pi / 6;
    mat<double, 3, 3> rot = {
        {std::cos(angle), 0, std::sin(angle), 0, 1, 0, -std::sin(angle), 0, std::cos(angle)}};
    return rot.matmul(v);
}

void filled_triangle(int ax, int ay, int az, int bx, int by, int bz, int cx, int cy, int cz,
                     TGAImage &framebuffer, TGAImage &depthbuffer, TGAColor color) {
    // Draw using a two-part method:
    // 1 - Compute bounding box of the triangle
    // 2 - For each pixel in the bounding box, compute whether it is
    //     inside the triangle. If yes, then draw it.

    // 1 - Bounding box
    int min_x, min_y, max_x, max_y;
    min_x = std::min({ax, bx, cx});
    max_x = std::max({ax, bx, cx});
    min_y = std::min({ay, by, cy});
    max_y = std::max({ay, by, cy});

    // 2 - check if pixel is inside triangle
    int ab_x = bx - ax, ab_y = by - ay;
    int bc_x = cx - bx, bc_y = cy - by;
    int ca_x = ax - cx, ca_y = ay - cy;
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);

    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            int dot_ab = ab_y * (x - ax) - ab_x * (y - ay);
            int dot_bc = bc_y * (x - bx) - bc_x * (y - by);
            int dot_ca = ca_y * (x - cx) - ca_x * (y - cy);
            // double alpha = signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            // double beta  = signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            // double gamma = signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            
            bool has_neg = dot_ab < 0 || dot_bc < 0 || dot_ca < 0;
            bool has_pos = dot_ab > 0 || dot_bc > 0 || dot_ca > 0;
            if (has_neg && has_pos) continue;

            double sum = dot_ab + dot_bc + dot_ca;
            double alpha = dot_ab / sum, beta = dot_bc / sum, gamma = dot_ca / sum;
            int depth = std::round(alpha * az + beta * bz + gamma * cz);
            if (depth > depthbuffer.get(x, y)[0]) {
                framebuffer.set(x, y, color);
                uint8_t depth_u = depth;
                depthbuffer.set(x, y, TGAColor({{(uint8_t)depth_u, depth_u, depth_u, depth_u}}));
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

Tup3<int> project(const vec3<double> &in, int w, int h) {
    // Obj coordinates go from -1 to +1. We want to map this to [0, w], [0, h]

    int out_x = std::round((in[0] + 1.0) * w / 2);
    int out_y = std::round((in[1] + 1.0) * h / 2);
    int out_z = std::round((in[2] + 1.0) * 256 / 2);

    return {out_x, out_y, out_z};
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " obj/model.obj" << std::endl;
        return 1;
    }

    constexpr int width = 2048;
    constexpr int height = 2048;
    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage depthbuffer(width, height, TGAImage::GRAYSCALE);

    ObjModel model;
    model.Load(argv[1]);

    for (int i = 0; i < model.faces.size(); ++i) {
        auto a = project(rot(model.vertices[model.faces[i].x]), width, height);
        auto b = project(rot(model.vertices[model.faces[i].y]), width, height);
        auto c = project(rot(model.vertices[model.faces[i].z]), width, height);

        TGAColor rnd;
        for (size_t i : {0, 1, 2}) {
            rnd[i] = std::rand() % 256;
        }
        filled_triangle(a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z, framebuffer, depthbuffer, rnd);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    depthbuffer.write_tga_file("depthbuffer.tga");

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
