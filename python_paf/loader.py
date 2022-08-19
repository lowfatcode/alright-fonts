import sys, struct
from . import Glyph, Point, Face

def extract_contours(data, scale):
  contours = []

  offset = 0
  while True:
    point_count = struct.unpack(">H", data[offset + 0:offset + 2])[0]
    offset += 2
    if point_count == 0: # at end we have a "zero" contour
      break

    contour = []

    # load points of contour
    for j in range(0, point_count):
      point = Point()           
      point_size = 2 if scale <= 7 else 4
      point.x, point.y = struct.unpack(
        ">bb" if scale <= 7 else ">hh", 
        data[offset + 0:offset + point_size]
      )
      offset += point_size
      contour.append(point)

    contours.append(contour)

  return contours

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
  
  glyph_entry_length = 9 if scale <= 7 else 14  

  # contours start at end of glyph dictionary
  contour_offset = 8 + (glyph_count * glyph_entry_length)

  for i in range(0, glyph_count):        
    glyph = Glyph()    
    glyph_entry_offset = 8 + (i * glyph_entry_length)
    glyph.codepoint, glyph.bbox_x, glyph.bbox_y, glyph.bbox_w, \
      glyph.bbox_h, glyph.advance, contour_data_length = \
      struct.unpack(
        ">HbbBBBH" if scale <= 7 else ">HhhHHHH", 
        data[glyph_entry_offset:glyph_entry_offset + glyph_entry_length]
      )

    glyph.contours = extract_contours(
      data[contour_offset:contour_offset + contour_data_length],
      scale
    )
    contour_offset += contour_data_length

    

    face.glyphs[glyph.codepoint] = glyph

  return face