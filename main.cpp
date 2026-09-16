#include <cmath>
#include "tgaimage.h"

constexpr TGAColor white = {255, 255, 255, 255}; // attention, BGRA order
constexpr TGAColor green = {0, 255, 0, 255};
constexpr TGAColor red = {0, 0, 255, 255};
constexpr TGAColor blue = {255, 128, 64, 255};
constexpr TGAColor yellow = {0, 200, 255, 255};

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color)
{
    float dx, dy;
    dx = bx - ax;
    dy = by - ay;

    float cur_x = ax, cur_y = ay;

    for (float t = 0.0; t <= 1.0; t += 0.02)
    {
        int x = std::round(ax + dx * t);
        int y = std::round(ay + dy * t);
        framebuffer.set(x, y, color);
    }
}

int main(int argc, char **argv)
{
    constexpr int width = 64;
    constexpr int height = 64;
    TGAImage framebuffer(width, height, TGAImage::RGB);

    int ax = 7, ay = 3;
    int bx = 12, by = 37;
    int cx = 62, cy = 53;

    line(ax, ay, bx, by, framebuffer, blue);
    line(cx, cy, bx, by, framebuffer, green);
    line(cx, cy, ax, ay, framebuffer, yellow);
    line(ax, ay, cx, cy, framebuffer, red);

    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}
