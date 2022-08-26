#include <cstdint>
#include <algorithm>
#include <math.h>
#include <string.h>
#include <fstream>
#include <vector>
#include <map>

using namespace std;

/*
  shorthand stream value big endian read functions 
*/
uint16_t  ru16(ifstream &ifs) {uint8_t w[2]; ifs.read((char *)w, 2); return w[0] << 8 | w[1];}
int16_t   rs16(ifstream &ifs) {uint8_t w[2]; ifs.read((char *)w, 2); return w[0] << 8 | w[1];}
uint32_t  ru32(ifstream &ifs) {uint8_t dw[4]; ifs.read((char *)dw, 4); return dw[0] << 24 | dw[1] << 16 | dw[2] << 8 | dw[3];}
uint8_t   ru8(ifstream &ifs) {return ifs.get();}
int8_t    rs8(ifstream &ifs) {return ifs.get();}

namespace paf {

  struct mat3_t;

  struct point_t {
    int16_t x, y;

    point_t(int16_t x, int16_t y) :
      x(x), y(y) {}

    inline point_t& operator-= (const point_t &a) { x -= a.x; y -= a.y; return *this; }
    inline point_t& operator+= (const point_t &a) { x += a.x; y += a.y; return *this; }
    inline point_t& operator*= (const float a) { x *= a;   y *= a;   return *this; }
    inline point_t& operator*= (const mat3_t &a) { this->transform(a); return *this; }
    inline point_t& operator/= (const float a) { x /= a;   y /= a;   return *this; }
    inline point_t& operator/= (const point_t &a) { x /= a.x;   y /= a.y;   return *this; }

    void transform(const mat3_t &m);
  };

  struct rect_t {
    int16_t x, y, w, h;

    rect_t() :
      x(0), y(0), w(0), h(0) {}
    rect_t(int16_t x, int16_t y, int16_t w, int16_t h) :
      x(x), y(y), w(w), h(h) {}
  };

  struct mat3_t {
    float v00, v10, v20, v01, v11, v21, v02, v12, v22 = 0.0f;
    mat3_t() = default;
    mat3_t(const mat3_t &m) = default;
    inline  mat3_t& operator*= (const mat3_t &m) {        
      float r00 = this->v00 * m.v00 + this->v01 * m.v10 + this->v02 * m.v20;
      float r01 = this->v00 * m.v01 + this->v01 * m.v11 + this->v02 * m.v21;
      float r02 = this->v00 * m.v02 + this->v01 * m.v12 + this->v02 * m.v22;
      float r10 = this->v10 * m.v00 + this->v11 * m.v10 + this->v12 * m.v20;
      float r11 = this->v10 * m.v01 + this->v11 * m.v11 + this->v12 * m.v21;
      float r12 = this->v10 * m.v02 + this->v11 * m.v12 + this->v12 * m.v22;
      float r20 = this->v20 * m.v00 + this->v21 * m.v10 + this->v22 * m.v20;
      float r21 = this->v20 * m.v01 + this->v21 * m.v11 + this->v22 * m.v21;
      float r22 = this->v20 * m.v02 + this->v21 * m.v12 + this->v22 * m.v22;    
      this->v00 = r00; this->v01 = r01; this->v02 = r02;
      this->v10 = r10; this->v11 = r11; this->v12 = r12;
      this->v20 = r20; this->v21 = r21; this->v22 = r22;
      return *this;
    }

    static mat3_t identity() {mat3_t m; m.v00 = m.v11 = m.v22 = 1.0f; return m;}
    static mat3_t rotation(float a) {
      float c = cosf(a), s = sinf(a); mat3_t r = mat3_t::identity();
      r.v00 = c; r.v01 = s; r.v10 = -s; r.v11 = c; return r;}
    static mat3_t translation(float x, float y) {
      mat3_t r = mat3_t::identity(); r.v02 = x; r.v12 = y; return r;}
    static mat3_t scale(float x, float y) {
      mat3_t r = mat3_t::identity(); r.v00 = x; r.v11 = y; return r;}
    static mat3_t translation(const point_t &p) {return mat3_t::translation(p.x, p.y);}
    static mat3_t scale(const point_t &p) {return mat3_t::scale(p.x, p.y);}
  };
  inline mat3_t operator* (mat3_t lhs, const mat3_t &m) {lhs *= m; return lhs;}

  void point_t::transform(const mat3_t &m) {
    this->x = (m.v00 * float(this->x) + m.v01 * float(this->y) + m.v02);
    this->y = (m.v10 * float(this->x) + m.v11 * float(this->y) + m.v12);
  }

  struct glyph_t {
    uint16_t codepoint;
    rect_t bounds;
    uint16_t advance;
    uint8_t *contours;    
  };

  struct face_t {
    uint16_t glyph_count;
    uint8_t scale;
    uint8_t flags;

    std::map<uint16_t, glyph_t> glyphs;
    uint8_t* contours = nullptr;

    face_t(ifstream &ifs) {load(ifs);}
    face_t(string path) {load(path);}
    
    bool load(ifstream &ifs);
    bool load(string path);

    ~face_t() {
      delete contours;
    }
  };

  enum antialias_t {
    x1      = 0,
    x2      = 1,
    x4      = 2
  };

  enum alignment_t {
    left    = 0, 
    center  = 1, 
    right   = 2,
    justify = 4,
    top     = 8,
    bottom  = 16
  };  

  struct text_metrics_t {
    face_t &face;                     // font to write in
    int size;                         // text size in pixels
    uint scroll;                      // vertical scroll offset
    int line_height;                  // spacing between lines (%)
    int letting_spacing;              // spacing between characters    
    int word_spacing;                 // spacing between words    
    alignment_t align;                // horizontal and vertical alignment
    std::optional<mat3_t> transform;  // arbitrary transformation
    antialias_t antialiasing = x1;    // level of antialiasing to apply

    text_metrics_t(face_t &face, int size) : face(face), size(size) {}
  };

  enum bits_per_pixel_t {
    one   = 1,
    two   = 2,
    four  = 4,
    eight = 8
  };

  struct render_tile_t {
    rect_t bounds;
    bits_per_pixel_t bpp;
    void *p;
  };

  /*
    global properties
  */

  // function called when renderer has prepared a tile of data for drawing
  // client code must provide an implementation for this function
  typedef void (*render_tile_callback_t)(const render_tile_t &rt);
  static render_tile_callback_t rtc;

  // clipping rectangle for rendering, usually the full bounds of the 
  // screen 
  static rect_t clip;

  /*
    initialisation
  */
  void init(render_tile_callback_t _rtc, rect_t _clip) {
    rtc = _rtc;
    clip = _clip;
  }

  /*
    helper functions
  */

  // returns a point from a contour based on the point size specified
  inline __attribute__((always_inline)) point_t contour_point(uint8_t *p, uint8_t ps) {    
    if(ps == 2) {return point_t(p[0], p[1]);}
    return point_t(p[0] << 8 | p[1], p[2] << 8 | p[3]);
  }

  // apply face, supersampling, and target size scaling to point
  inline __attribute__((always_inline)) void scale_point(point_t &p, const text_metrics_t &tm) {
    p.x = (p.x * tm.size) >> (tm.face.scale - tm.antialiasing);
    p.y = (p.y * tm.size) >> (tm.face.scale - tm.antialiasing);
  }

  // dy step (returns 1, 0, or -1 if the supplied value is > 0, == 0, < 0)
  inline __attribute__((always_inline)) int dy_step(int dy) {
    // assumes 32-bit int/unsigned
    return ((unsigned) - dy >> 31) - ((unsigned)dy >> 31);
  }

  /* 
    render functions
  */

  void render_character(text_metrics_t &tm, uint16_t codepoint, point_t point) {
/*    // tile rendering buffer
    static uint32_t tile_buffer[32];

    // polygon node list buffer - 4kB
    // handles at most 32 line intersections - is this enough for cjk/emoji?
    static int32_t nodes[32][32];
    static uint32_t node_counts[32];

    // if character not in face then draw nothing
    if(tm.face.glyphs.find(codepoint) == tm.face.glyphs.end()) {
      return;
    }

    // get the glyph contours and point size
    uint8_t ps = tm.face.scale <= 7 ? 2 : 4;
    uint8_t *p = tm.face.glyphs[codepoint].contours;

    // reset the node counts and tile buffer
    memset(node_counts, 0, sizeof(node_counts));
    memset(tile_buffer, 0, sizeof(tile_buffer));
    
    while(true) {
      // get point count for this contour
      uint16_t c = *p++ << 8; c |= *p++;      
      if(c == 0) {break;} // empty contour means end of contours


      // start with the last point of the contour to close the loop
      point_t start = contour_point(p + ((c - 1) * ps), ps);  
      scale_point(start, tm);    
//      printf("> contour start - point count %d\n", c);
//      printf("  - %d, %d\n", start.x, start.y);

      // for each point in contour
      for(auto i = 0; i < c; i++) {
        point_t end = contour_point(p, ps);
        scale_point(end, tm);
        printf("  - %d, %d\n", end.x, end.y);
        p += ps;

        // if line segment is purely horizontal we can skip it, adjoining
        // edges will define spans that cover the same area
        if(start.y == end.y && start.y >= 0 && start.y < 32) {
          continue;
        }

        int dy = end.y - start.y;
        int x = start.x << 8;
        int y = start.y;

        // step_y is 1, 0, or -1 (when dy is >0, ==0, <0 respectively)
        int step_y = dy < 0 ? -1 : 1;
        int step_x = ((end.x - start.x) << 8) / abs(dy);
        while(y != end.y) {
          if(y >= 0 && y < 32) {
            int cx = x >> 8;
            cx = cx < 0 ? 0 : cx > 31 ? 31 : cx;
            nodes[y][node_counts[y]++] = cx;
//            printf("    : add node %d, %d\n", y, x >> 8);
          }
          y += step_y;
          x += step_x;
        }

        start = end;
      }

//      printf("here");

      // sort the nodes for each scanline
      for(auto y = 0; y < 32; y++) {
        std::sort(nodes[y], nodes[y] + node_counts[y]);


        // render the spans for this line
        for (uint32_t i = 0; i < node_counts[y]; i += 2) {
          uint32_t start = nodes[y][i];
          uint32_t end = nodes[y][i + 1];
          //printf("span on %d from %d to %d\n", y, start, end);

          uint32_t span_width = end - start;          
          if(span_width == 0) {
            continue;
          }

          // create a mask with `width` bits set to 1
          uint32_t span_mask = ((1 << span_width) - 1);          
          // write the mask into the tile buffer at `start` position
          tile_buffer[y] |= span_mask << (32 - start - span_width);
        }

        for(auto i = 0; i < 32; i++) {
          printf(((1 << (31 - i)) & tile_buffer[y]) != 0 ? "O" : " ");
        }
        printf("\n");
      }
    }


    render_tile_t rt;
    rt.bounds = rect_t(0, 0, 128, 128);
    rt.bpp = one;
    rt.p = tile_buffer;
    rtc(rt);*/
  }

  void render(const text_metrics_t &tm, rect_t bounds) {
  }

  void render(const text_metrics_t &tm, point_t point) {
  }


  /*
    load functions
  */
  bool face_t::load(ifstream &ifs) {
    char marker[4];
    ifs.read(marker, sizeof(marker));

    // check header magic bytes are present
    if(memcmp(marker, "paf!", 4) != 0) {
      // doesn't start with magic marker
      return false;
    }

    // number of glyphs embedded in font file
    this->glyph_count = ru16(ifs);

    // extract and check scale value
    this->scale = ru8(ifs);
    if(this->scale > 10) {
      // scale out of bounds
      return false;
    }

    // extract flags and ensure none set
    this->flags = ru8(ifs);
    if(this->flags != 0) {
      // unknown flags set
      return false;
    }

    // extract glyph dictionary
    uint16_t glyph_entry_size = this->scale <= 7 ? 9 : 14;
    uint32_t contour_data_offset = 8 + this->glyph_count * glyph_entry_size;
    for(auto i = 0; i < this->glyph_count; i++) {
      glyph_t g;
      if(this->scale <= 7) {
        g.codepoint = ru16(ifs);
        g.bounds.x  = rs8(ifs);
        g.bounds.y  = rs8(ifs);
        g.bounds.w  = ru8(ifs);
        g.bounds.h  = ru8(ifs);
        g.advance   = ru8(ifs);
      }else{
        g.codepoint = ru16(ifs);
        g.bounds.x  = rs16(ifs);
        g.bounds.y  = rs16(ifs);
        g.bounds.w  = ru16(ifs);
        g.bounds.h  = ru16(ifs);
        g.advance   = ru16(ifs);
      }

/*      printf("- g.codepoint %d\n", g.codepoint);
      printf("- g.bounds.x %d\n", g.bounds.x);
      printf("- g.bounds.y %d\n", g.bounds.y);
      printf("- g.bounds.w %d\n", g.bounds.w);
      printf("- g.bounds.h %d\n", g.bounds.h);
      printf("- g.advance %d\n", g.advance);*/

      if(ifs.fail()) {
        // could not read glyph dictionary entry
        return false;
      }

      // allocate space for the contour data and read it from the font file
      uint16_t contour_data_length = ru16(ifs);
/*      printf("- contour_data_length %d\n", contour_data_length);
      printf("- contour_data_offset %d\n", contour_data_offset);*/
      g.contours = new uint8_t[contour_data_length];
      int pos = ifs.tellg();
      ifs.seekg(contour_data_offset, ios::beg);
      ifs.read((char *)g.contours, contour_data_length);
      ifs.seekg(pos);
      contour_data_offset += contour_data_length;

      if(ifs.fail()) {
        // could not read glyph contour data
        return false;
      }

      this->glyphs[g.codepoint] = g;
    }
    
    return true;
  }

  bool face_t::load(string path) {
    ifstream ifs(path, ios::binary);
    return load(ifs);
  }

}