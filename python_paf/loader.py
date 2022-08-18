import sys
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

  # load dictionary entries
  for i in range(0, glyph_count):
    offset = 8 + (i * 16)
    entry = data[offset:offset + 16]

    glyph = Glyph()
    glyph.codepoint = int.from_bytes(entry[ 0: 2], byteorder="big")
    glyph.bbox_x    = int.from_bytes(entry[ 2: 4], byteorder="big")
    glyph.bbox_y    = int.from_bytes(entry[ 4: 6], byteorder="big")
    glyph.bbox_w    = int.from_bytes(entry[ 6: 8], byteorder="big")
    glyph.bbox_h    = int.from_bytes(entry[ 8:10], byteorder="big")
    glyph.advance   = int.from_bytes(entry[10:12], byteorder="big")
    
    # load contours for this glyph
    offset  = int.from_bytes(entry[12:16], byteorder="big")
    while True:
      point_count = int.from_bytes(data[offset:offset+2], byteorder="big")    
      offset += 2
      if point_count == 0: # at the end of contours we have a "zero" contour
        break

      contour = []

      # load points of contour
      last = Point(0, 0)
      for j in range(0, point_count):
        byte1 = data[offset]
        offset += 1
        
        x = 0
        y = 0

        # one byte encoding (y == 0)
        if byte1 & 0b11100000 == 0b11000000:
          x = ((byte1 & 0b00011111)) - 16

        # one byte encoding (x == 0)
        if byte1 & 0b11100000 == 0b11100000:
          y = ((byte1 & 0b00011111)) - 16

        # one byte encoding
        if byte1 & 0b11000000 == 0b00000000:
          x = ((byte1 & 0b111000) >> 3) - 4
          y = ((byte1 & 0b000111) >> 0) - 4

        # two byte encoding
        if byte1 & 0b11000000 == 0b10000000:
          byte2 = data[offset]
          offset += 1
          x = (((byte1 & 0b111111) << 1) | (byte2 >> 7)) - 64
          y = ((byte2 & 0b1111111) >> 0) - 64

        # three byte encoding
        if byte1 & 0b11000000 == 0b01000000:
          byte2 = data[offset]
          offset += 1
          byte3 = data[offset]
          offset += 1
          x = (((byte1 & 0b111111) << 5) | (byte2 >> 3)) - 1024
          y = (((byte2 & 0b111) << 8) | byte3) - 1024

        point = Point(last.x + x, last.y + y)

        contour.append(point)

        last = point

      glyph.contours.append(contour)

    face.glyphs[glyph.codepoint] = glyph

  return face