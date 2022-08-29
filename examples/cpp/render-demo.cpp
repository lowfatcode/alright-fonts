#include "alright-fonts.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace paf;

constexpr int WIDTH = 512;
constexpr int HEIGHT = 512;

typedef uint32_t color;
color image[WIDTH][HEIGHT];

color pen(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return r << 24 | g << 16 | b << 8 | a;
}

color pen(uint8_t r, uint8_t g, uint8_t b) {
  return pen(r, g, b, 255);
}

void pixel(int32_t x, int32_t y, color c) {
  if(x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT) {
    return;
  }
  image[x][y] = c;
}

void line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, color c) {
  int32_t	x = x1, y = y1, dx, dy, incx, incy, balance;

  if(x2 >= x1) {dx = x2 - x1; incx = 1;} else {dx = x1 - x2; incx = -1;}
  if(y2 >= y1) {dy = y2 - y1; incy = 1;} else {dy = y1 - y2; incy = -1;}

  if(dx >= dy) {
    dy <<= 1; balance = dy - dx; dx <<= 1;
    while(x != x2) {
      pixel(x, y, c);
      if (balance >= 0) {y += incy; balance -= dx;}
      balance += dy; x += incx;
    }
  } else {
    dx <<= 1; balance = dx - dy; dy <<= 1;
    while(y != y2) {
      pixel(x, y, c);
      if(balance >= 0) {x += incx; balance -= dy;}
      balance += dx; y += incy;
    }
  }
}

int main() {
  std::string font_path = "../sample-fonts/Roboto/Roboto-Black.paf";
/*
  // initialise alright fonts
  init(render_tile_callback, rect_t(0, 0, WIDTH, HEIGHT));

  // load a font
  face_t face(font_path);

  // define text metrics
  text_metrics_t tm(face, 16);

  line(10, 10, 100, 50, pen(255, 0, 0));

  render_character(tm, int('i'), point_t(10, 10));
*/
  // output the image
  stbi_write_png("out.png", WIDTH, HEIGHT, 4, image, WIDTH * 4);

  return 0;
}