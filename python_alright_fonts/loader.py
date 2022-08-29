import sys, struct
from . import Glyph, Point, Face

def extract_contours(data):
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
      point.x, point.y = struct.unpack(
        ">bb", 
        data[offset + 0:offset + 2]
      )
      offset += 2
      contour.append(point)

    contours.append(contour)

  return contours

def load_font(file_or_name_or_bytes):
  face = Face()

  data = None
  if isinstance(file_or_name_or_bytes, (bytes, bytearray)):
    data = file_or_name_or_bytes
  elif isinstance(file_or_name_or_bytes, str):
    f = open(file_or_name_or_bytes, "rb")
    data = f.read()
    f.close()
  else:
    data = file_or_name_or_bytes.read()

  # check the header is correct
  if data[:4] != b"af!?":
    print("> invalid Alright Fonts file provided. no matching magic marker in header!")
    sys.exit()

  glyph_count = int.from_bytes(data[4:6], byteorder="big")
  flags = int.from_bytes(data[6:8], byteorder="big")
  
  glyph_entry_length = 9

  # contours start at end of glyph dictionary
  contour_offset = 8 + (glyph_count * glyph_entry_length)

  for i in range(0, glyph_count):        
    glyph = Glyph()    
    glyph_entry_offset = 8 + (i * glyph_entry_length)
    glyph.codepoint, glyph.bbox_x, glyph.bbox_y, glyph.bbox_w, \
      glyph.bbox_h, glyph.advance, contour_data_length = \
      struct.unpack(
        ">HbbBBBH", 
        data[glyph_entry_offset:glyph_entry_offset + glyph_entry_length]
      )

    glyph.contours = extract_contours(
      data[contour_offset:contour_offset + contour_data_length]
    )
    contour_offset += contour_data_length

    face.glyphs[glyph.codepoint] = glyph

  return face