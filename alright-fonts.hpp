#include <cstdint>
#include <math.h>
#include <string.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <vector>
#include <optional>
#include <map>

#include "pretty-poly/pretty-poly.hpp"

using namespace pretty_poly;
using namespace std;

namespace alright_fonts {

  struct glyph_t {
    uint16_t codepoint;
    rect_t bounds;
    uint8_t advance;
    vector<contour_t<int8_t>> contours;
  };

  struct face_t {
    uint16_t glyph_count;
    uint16_t flags;
    std::map<uint16_t, glyph_t> glyphs;

    face_t(ifstream &ifs) {load(ifs);}
    face_t(string path) {load(path);}
    
    bool load(ifstream &ifs);
    bool load(string path);
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
    //optional<mat3_t> transform;       // arbitrary transformation
    antialias_t antialiasing = X4;    // level of antialiasing to apply

    text_metrics_t(face_t &face, int size) : face(face), size(size) {}
  };


  /*
    global properties
  */

  /*
    helper functions
  */
/*
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
*/
  /* 
    render functions
  */

  void render_character(text_metrics_t &tm, uint16_t codepoint, point_t<int> origin) {
    if(tm.face.glyphs.count(codepoint) == 1) {
      glyph_t glyph = tm.face.glyphs[codepoint];

      // scale is a fixed point 16:16 value, our font data is already scaled to
      // -128..127 so to get the pixel size we want we can just shift the
      // users requested size up one bit
      unsigned scale = tm.size << 9;

      draw_polygon<int8_t>(glyph.contours, origin, scale);
    }
  }
/*
  void render(const text_metrics_t &tm, rect_t bounds) {
  }

  void render(const text_metrics_t &tm, point_t point) {
  }
*/

  /*
    load functions
  */

  // big endian stream value helpers
  uint16_t  ru16(ifstream &ifs) {uint8_t w[2]; ifs.read((char *)w, 2); return w[0] << 8 | w[1];}
  int16_t   rs16(ifstream &ifs) {uint8_t w[2]; ifs.read((char *)w, 2); return w[0] << 8 | w[1];}
  uint32_t  ru32(ifstream &ifs) {uint8_t dw[4]; ifs.read((char *)dw, 4); return dw[0] << 24 | dw[1] << 16 | dw[2] << 8 | dw[3];}
  uint8_t   ru8(ifstream &ifs) {return ifs.get();}
  int8_t    rs8(ifstream &ifs) {return ifs.get();}

  bool face_t::load(ifstream &ifs) {
    char marker[4];
    ifs.read(marker, sizeof(marker));

    // check header magic bytes are present
    if(memcmp(marker, "af!?", 4) != 0) {
      // doesn't start with magic marker
      return false;
    }

    // number of glyphs embedded in font file
    this->glyph_count = ru16(ifs);

    // extract flags and ensure none set
    this->flags = ru16(ifs);
    if(this->flags != 0) {
      // unknown flags set
      return false;
    }

    // extract glyph dictionary
    uint16_t glyph_entry_size = 9;
    uint32_t contour_data_offset = 8 + this->glyph_count * glyph_entry_size;
    for(auto i = 0; i < this->glyph_count; i++) {
      glyph_t g;
      g.codepoint = ru16(ifs);
      g.bounds.x  = rs8(ifs);
      g.bounds.y  = rs8(ifs);
      g.bounds.w  = ru8(ifs);
      g.bounds.h  = ru8(ifs);
      g.advance   = ru8(ifs);

      if(ifs.fail()) {
        // could not read glyph dictionary entry
        return false;
      }

      // allocate space for the contour data and read it from the font file
      uint16_t contour_data_length = ru16(ifs);

      // remember where we are in the dictionary
      int pos = ifs.tellg();  

      // read contour data
      ifs.seekg(contour_data_offset, ios::beg);
      while(true) {
        // get number of points in contour
        uint16_t count = ru16(ifs);

        // if count is zero then this is the end of contour marker    
        if(count == 0) {
          break;
        }

        // allocate space to store point data for contour and read
        // from file
        point_t<int8_t> *points = new point_t<int8_t>[count];
        ifs.read((char *)points, count * 2);

        g.contours.push_back({points, count});
      }      

      // return back to position in dictionary
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
    if(ifs.fail()) {
      // could not open file
      return false;
    }    
    return load(ifs);
  }

}