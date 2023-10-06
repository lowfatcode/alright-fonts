#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define AF_IMPLEMENTATION
#include "alright-fonts.h"
#include "helpers.h"

const int WIDTH = 1024;
const int HEIGHT = 1024;
colour buffer[1024][1024];

colour pen;
void set_pen(colour c) {
  pen = c;
}

void blend_tile(const pp_tile_t *t) {
  for(int32_t y = t->y; y < t->y + t->h; y++) {
    for(int32_t x = t->x; x < t->x + t->w; x++) {     
      colour alpha_pen = pen;
      alpha_pen.a = alpha(pen.a, pp_tile_get(t, x, y));
      buffer[y][x] = blend(buffer[y][x], alpha_pen);
    }
  }
}

int main() {
  pp_tile_callback(blend_tile);
  pp_antialias(PP_AA_X16);
  pp_clip(0, 0, WIDTH, HEIGHT);

  FILE *file = fopen("sample-fonts/SpecialElite/SpecialElite-Regular.af", "r");
  af_face_t font;
  af_load_font_file(file, &font);
  fclose(file);

  uint64_t start = time_ms();

  wchar_t *text = L"’Twas brillig, and the slithy toves \n\
Did gyre and gimble in the wabe: \n\
All mimsy were the borogoves, \n\
And the mome raths outgrabe. \n\
\n\
“Beware the Jabberwock, my son! \n\
The jaws that bite, the claws that catch! \n\
Beware the Jubjub bird, and shun \n\
The frumious Bandersnatch!” \n\
\n\
He took his vorpal sword in hand; \n\
Long time the manxome foe he sought— \n\
So rested he by the Tumtum tree \n\
And stood awhile in thought.";

  pp_mat3_t transform;
  transform = pp_mat3_identity();
  pp_mat3_translate(&transform, 30, 200);
  pp_mat3_scale(&transform, 1.0f, 1.1f);

  af_text_metrics_t tm;
  tm.transform = &transform;
  tm.line_height = 110;
  tm.letting_spacing = 95;
  tm.word_spacing = 200;
  tm.size = 48;
  tm.align = AF_H_ALIGN_CENTER;

  set_pen(create_colour(0, 0, 0, 255));
  af_render(&font, text, &tm);

  pp_mat3_translate(&transform, -10, -10);

  set_pen(create_colour(200, 220, 240, 255));
  af_render(&font, text, &tm);

  uint64_t end = time_ms();
  printf("render time: %llums", end - start);

  // output the image
  stbi_write_png("/tmp/out.png", WIDTH, HEIGHT, 4, (void *)buffer, WIDTH * sizeof(uint32_t));

  return 0;
}
