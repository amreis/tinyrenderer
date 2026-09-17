#include "objmodel.h"
#include "tgaimage.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <print>

constexpr TGAColor white = {{255, 255, 255, 255}}; // attention, BGRA order
constexpr TGAColor green = {{0, 255, 0, 255}};
constexpr TGAColor red = {{0, 0, 255, 255}};
constexpr TGAColor blue = {{255, 128, 64, 255}};
constexpr TGAColor yellow = {{0, 200, 255, 255}};

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

void filled_triangle(int ax, int ay, int bx, int by, int cx, int cy, int az, int bz, int cz,
                     TGAImage &framebuffer, TGAColor color) {
    // Draw using a two-part method:
    // 1 - Compute bounding box of the triangle
    // 2 - For each pixel in the bounding box, compute whether it is
    //     inside the triangle. If yes, then draw it.

    TGAColor ac{{255, 0, 0}}, bc{{0, 255, 255}}, cc{{255, 0, 255}};

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
    for (int x = min_x; x <= max_x; ++x) {
        for (int y = min_y; y <= max_y; ++y) {
            int dot_ab = ab_y * (x - ax) - ab_x * (y - ay);
            int dot_bc = bc_y * (x - bx) - bc_x * (y - by);
            int dot_ca = ca_y * (x - cx) - ca_x * (y - cy);

            if (dot_ab < 0 || dot_bc < 0 || dot_ca < 0) {
                continue;
            }

            double sum = dot_ab + dot_bc + dot_ca;
            double alpha = dot_ab / sum, beta = dot_bc / sum, gamma = dot_ca / sum;
            TGAColor color = color_lerp(ac, bc, cc, alpha, beta, gamma);
            framebuffer.set(x, y, color);
        }
    }
}

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer,
              TGAColor color) {
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);
}

Tup3<int> to_pixel_coords(const Vec3 &in, int w, int h) {
    // Obj coordinates go from -1 to +1. We want to map this to [0, w], [0, h]

    int out_x = std::round((in.x + 1.0) * w / 2);
    int out_y = std::round((in.y + 1.0) * h / 2);

    return {out_x, out_y, 0};
}

int main(int argc, char **argv) {
    constexpr int width = 1024;
    constexpr int height = 1024;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    ObjModel model;
    model.Load("./obj/african_head/african_head.obj");

    for (int i = 0; i < model.faces.size(); ++i) {
        auto a = to_pixel_coords(model.vertices[model.faces[i].x], width, height);
        auto b = to_pixel_coords(model.vertices[model.faces[i].y], width, height);
        auto c = to_pixel_coords(model.vertices[model.faces[i].z], width, height);

        TGAColor rnd;
        for (size_t i : {0, 1, 2}) {
            rnd[i] = std::rand() % 256;
        }
        filled_triangle(a.x, a.y, b.x, b.y, c.x, c.y, 255, 127, 0, framebuffer, rnd);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
