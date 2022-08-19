import sys, struct
from . import Glyph, Point, Face

def load_paf(file_or_bytes):
  face = Face()

  data = None
  if isinstance(file_or_bytes, (bytes, bytearray)):
    data = file_or_bytes
  else:
    data = file_or_bytes.read()

  # check the header is correct
  if data[:4] != b"paf!":
    print("> invalid paf file provided. no matching magic marker in header!")
    sys.exit()

  glyph_count = int.from_bytes(data[4:6], byteorder="big")
  scale = int.from_bytes(data[6:7], byteorder="big")
  flags = int.from_bytes(data[7:8], byteorder="big")
  
  offset = 8

  # load dictionary entries
  for i in range(0, glyph_count):    
    data = data[offset:]
    offset = 0

    glyph = Glyph()
    glyph_header_length = 7 if scale <= 7 else 12
    glyph.codepoint, glyph.bbox_x, \
      glyph.bbox_y, glyph.bbox_w, \
      glyph.bbox_h, glyph.advance = \
      struct.unpack(">HbbBBB" if scale <= 7 else ">HhhHHH", data[0: glyph_header_length])
    offset += glyph_header_length
    
    # load contours for this glyph    
    while True:
      point_count = int.from_bytes(data[offset + 0:offset + 2], byteorder="big")    
      offset += 2
      if point_count == 0: # at the end of contours we have a "zero" contour
        break

      contour = []

      # load points of contour
      for j in range(0, point_count):
        # extract coordinates
        point = Point()      
        
        point_size = 2 if scale <= 7 else 4
        point.x, point.y = struct.unpack(">bb" if scale <= 7 else ">hh", data[offset + 0:offset + point_size])
        offset += point_size

        contour.append(point)

      glyph.contours.append(contour)

    face.glyphs[glyph.codepoint] = glyph

  return face