#include "alright-fonts.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace alright_fonts;

constexpr int WIDTH = 512;
constexpr int HEIGHT = 512;

typedef uint32_t color;
color image[WIDTH][HEIGHT];

color pen(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
  return a << 24 | b << 16 | g << 8 | r;
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


void callback(const tile_t &tile) {
  static uint8_t alpha_map_X4[5] = {0, 64, 128, 192, 255};
  static uint8_t alpha_map_X16[17] = {0, 16, 32, 48, 64, 80, 96, 112, 128, 144, 160, 176, 192, 208, 224, 240, 255};

  for(int i = 0; i < 5; i++) {
    const float gamma = 1.2f;
    uint8_t value = (uint8_t)(powf((i * 63) / 255.0f, gamma) * 255.0f + 0.5f);
    alpha_map_X4[i] = value;
  }

  for(int i = 0; i < 17; i++) {
    const float gamma = 0.5f;
    uint8_t value = (uint8_t)(powf((i * 15) / 255.0f, gamma) * 255.0f + 0.5f);
    alpha_map_X16[i] = value;
  }
  


  uint8_t *p = tile.data;
  for(auto y = 0; y < tile.bounds.h; y++) {
    for(auto x = 0; x < tile.bounds.w; x++) {    
      uint8_t value = settings::antialias == X4 ? alpha_map_X4[*p++] : alpha_map_X16[*p++];
      image[y + tile.bounds.y][x + tile.bounds.x] = pen(255, 255, 255, value);
    }
    p += tile.stride - tile.bounds.w;
  }
}

int main() {
  //std::string font_path = "sample-fonts/IndieFlower/IndieFlower-Regular.af";
  std::string font_path = "sample-fonts/Roboto/Roboto-Black.af";
  
  set_options(callback, X4, {0, 0, WIDTH, HEIGHT});
  
  face_t face(font_path);
  text_metrics_t tm(face, 16);
  point_t origin(50, 50);
  point_t caret = origin;
  for(int codepoint = 0; codepoint < 128; codepoint++) {
    render_character(tm, codepoint, caret);

    if(tm.face.glyphs.count(codepoint) == 1) {
      glyph_t glyph = tm.face.glyphs[codepoint];
      caret.x += ((glyph.advance * tm.size) / 128) * 1.2;
      if(caret.x > 400) {
        caret.y += tm.size;
        caret.x = origin.x;
      }
    }
  }
  

  // output the image
  stbi_write_png("/tmp/out.png", WIDTH, HEIGHT, 4, image, WIDTH * 4);

  return 0;
}