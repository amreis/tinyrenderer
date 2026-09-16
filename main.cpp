#include "objmodel.h"
#include "tgaimage.h"

#include <cmath>
#include <cstdio>
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

    const int dx = bx - ax, dy = std::max(by - ay, ay - by);
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

void triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer,
              TGAColor color) {
    line(ax, ay, bx, by, framebuffer, color);
    line(bx, by, cx, cy, framebuffer, color);
    line(cx, cy, ax, ay, framebuffer, color);
}

Tup3<int> to_pixel_coords(const Vec3 &in, int w, int h) {
    // Obj coordinates go from -1 to +1. We want to map this to [0, w], [0, h]

    int out_x = std::round((in.x + 1.0) * 0.5 * w);
    int out_y = std::round((in.y + 1.0) * 0.5 * h);

    return {out_x, out_y, 0};
}

int main(int argc, char **argv) {
    constexpr int width = 2048;
    constexpr int height = 2048;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    ObjModel wf;

    if (!wf.Load("./obj/african_head/african_head.obj")) {
        std::println(stderr, "Error!");
    }

    for (const Ind3 &face : wf.faces) {
        const Vec3 &a = wf.vertices[face.x];
        const Vec3 &b = wf.vertices[face.y];
        const Vec3 &c = wf.vertices[face.z];

        const Tup3<int> &a_p = to_pixel_coords(a, width, height);
        const Tup3<int> &b_p = to_pixel_coords(b, width, height);
        const Tup3<int> &c_p = to_pixel_coords(c, width, height);

        line(a_p.x, a_p.y, b_p.x, b_p.y, framebuffer, red);
        line(b_p.x, b_p.y, c_p.x, c_p.y, framebuffer, red);
        line(c_p.x, c_p.y, a_p.x, a_p.y, framebuffer, red);
    }

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
