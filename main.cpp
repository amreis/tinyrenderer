#include "objmodel.h"
#include "tgaimage.h"

#include <algorithm>
#include <array>
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

enum Hit { NONE, LEFT, RIGHT };

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

void filled_triangle(int ax, int ay, int bx, int by, int cx, int cy, TGAImage &framebuffer,
                     TGAColor color) {
    // Sort by y-value
    // int topx = 0, topy = 0, midx = 0, midy = 0, botx = 0, boty = 0;
    std::array<std::pair<int, int>, 3> ys({{ax, ay}, {bx, by}, {cx, cy}});
    std::sort(ys.begin(), ys.end(), [](std::pair<int, int> lhs, std::pair<int, int> rhs) -> bool {
        return lhs.second < rhs.second;
    });
    auto [bot, mid, top] = ys;
    auto [topx, topy] = top;
    auto [midx, midy] = mid;
    auto [botx, boty] = bot;

    // Key idea: the line from top to bot does not need to be changed when we reach
    // mid with the scan line. It keeps going, therefore it is relative to the
    // total height of the triangle.

    // Another thing: we do not need to worry about skipping pixels and drawing
    // perfect lines here (as in the line() function). That is because we will
    // fill the triangle anyway.

    int total_height = topy - boty;
    if (topy != midy) {
        int height_this_half = topy - midy;
        for (int y = topy; y >= midy; --y) {
            int xb = topx + ((botx - topx) * (topy - y)) / total_height;
            int xa = topx + ((midx - topx) * (topy - y)) / height_this_half;

            for (int x = std::min(xa, xb); x <= std::max(xa, xb); ++x)
                framebuffer.set(x, y, color);
        }
    }

    if (midy != boty) {
        int height_this_half = midy - boty;
        for (int y = midy; y >= boty; --y) {
            int xb = topx + ((botx - topx) * (topy - y)) / total_height;
            int xa = midx + ((botx - midx) * (midy - y)) / height_this_half;

            for (int x = std::min(xa, xb); x <= std::max(xa, xb); ++x)
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

    int out_x = std::round((in.x + 1.0) * 0.5 * w);
    int out_y = std::round((in.y + 1.0) * 0.5 * h);

    return {out_x, out_y, 0};
}

int main(int argc, char **argv) {
    constexpr int width = 128;
    constexpr int height = 128;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    filled_triangle(7, 45, 35, 100, 45, 60, framebuffer, red);
    filled_triangle(120, 35, 90, 5, 45, 110, framebuffer, white);
    filled_triangle(115, 83, 80, 90, 85, 120, framebuffer, green);
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
