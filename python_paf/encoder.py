import freetype, struct
from . import Glyph, Point

# contour encoding
# ===========================================================================

# packs the contour coordinates using the 1, 2, 3 byte method depending on
# the delta values from one coordinate to the next
def pack_glyph_contours(glyph):
  result = bytes()
  for contour in glyph.contours:      
    result += struct.pack(">H", len(contour))
    last_point = None
    for point in contour:
      delta = point.delta(last_point)   
      delta.x = int(delta.x)    
      delta.y = int(delta.y)
      
      if delta.y == 0 and delta.x >= -16 and delta.x < 15:
        # 110xxxxx
        # one byte pack dy == 0
        dx = delta.x + 16 # offset by +16
        byte1 = 0b11000000 | dx
        result += struct.pack(">B", byte1)
      elif delta.x == 0 and delta.y >= -16 and delta.y < 15:
        # 111yyyyy
        # one byte pack dx == 0
        dy = delta.y + 16 # offset by +16
        byte1 = 0b11100000 | dy
        result += struct.pack(">B", byte1)
      elif delta.x >= -4 and delta.x < 4 and delta.y >= -4 and delta.y < 4:
        # 00xxxyyy
        # one byte pack -4 <= x < 4 and -4 <= y < 4
        dx = delta.x + 4 # offset by +4
        dy = delta.y + 4 # offset by +4
        byte1 = 0b00000000 | (dx << 3) | dy
        result += struct.pack(">B", byte1)
      elif delta.x >= -64 and delta.x < 64 and delta.y >= -64 and delta.y < 64:
        # 10xxxxxx xyyyyyyy
        # two byte pack -64 <= x < 64 and -64 <= y < 64
        dx = delta.x + 64 # values offset by +64
        dy = delta.y + 64 # values offset by +64
        byte1 = 0b10000000 | (dx >> 1)
        byte2 = ((dx << 7) & 0xff) | dy
        result += struct.pack(">BB", byte1, byte2)
      else:
        # three byte pack (0b01xxxxxx xxxxxyyy yyyyyyyy)
        dx = delta.x + 512 # values offset by +512
        dy = delta.y + 512 # values offset by +512
        byte1 = 0b01000000 | (dx >> 5)        
        byte2 = ((dx << 3) & 0xff) | (dy >> 8)
        byte3 = (dy & 0xff)
        result += struct.pack(">BBB", byte1, byte2, byte3)
      last_point = point
  # end of contours marker
  result += struct.pack(">H", 0)
  return result      


class Segment():
  def __init__(self, start):
    self.start = start
    self.control = []
    self.end = None

  def add_control(self, point):
    self.control.append(point)

  def set_end(self, end):
    self.end = end

  def bezier_point(t, points):
    if len(points) == 1:
      return points[0]
    a = Segment.bezier_point(t, points[:-1])
    b = Segment.bezier_point(t, points[1:])
    return Point((1 - t) * a.x + t * b.x, (1 - t) * a.y + t * b.y)
    
  def point_at(self, t):
    return Segment.bezier_point(t, [self.start] + self.control + [self.end])
    
  # returns the list of points that make up a line segment or a 
  # bezier curve, does not return the last point as that will be
  # the start point of the next segment and we don't want duplicate 
  # points
  def decompose(self, quality=1):
    points = []

    if len(self.control) > 0:

      # create an array of interpolated coordinates
      coords = []
      # number of control points on the bezier is a fair proxy for 
      # its complexity, we'll use that to determine how many 
      # straight line segments to generate initially
      count = len(self.control) * 3
      for i in range(0, count):
        point = self.point_at(i / count)
        coords.append([point.x, point.y])

      # use simplification library to reduce the number of coordinates
      from simplification.cutil import simplify_coords_vwp      
      simplified = simplify_coords_vwp(coords, quality)

      points = []
      for coord in simplified[:-1]:
        points.append(Point(int(coord[0]), int(coord[1])))   
    else:
      # straight line segment
      points.append(self.start)

    return points
  

def load_glyph(face, codepoint, scale_factor, quality = 1):
  # glyph doesn't exist in face
  if face.get_char_index(codepoint) == 0:
    return None

  # glyph isn't printable
  if not chr(codepoint).isprintable():
    return None

  # load the glyph
  face.load_char(codepoint)

  glyph = Glyph() 
  glyph.codepoint = codepoint # utf-8 codepoint or ascii character code
  glyph.advance = round(face.glyph.metrics.horiAdvance * scale_factor)    

  # bounding box
  bbox = face.glyph.outline.get_bbox()
  glyph.bbox_x = round( bbox.xMin * scale_factor)
  glyph.bbox_y = round( bbox.yMin * scale_factor)
  glyph.bbox_w = round((bbox.xMax - bbox.xMin) * scale_factor)
  glyph.bbox_h = round((bbox.yMax - bbox.yMin) * scale_factor)
  
  # extract glyph contours
  outline = face.glyph.outline
  glyph.contours = []
  start = 0

  for end in outline.contours:
    # extract the points for this contour
    points = [Point(p) for p in outline.points[start:end + 1]]
    tags = outline.tags[start:end + 1]

    # attach start point to end to close the loop
    points.append(points[0])
    tags.append(tags[0])

    # invert the y axis, scale, and round the values in the contour
    points = [p.scale(scale_factor, -scale_factor).round() for p in points]

    # create list of segments for this contour
    contour = []
    
    while len(points) > 1:
      # create a new segment with start point
      segment = Segment(points.pop(0))
      tags.pop(0)

      # add any control points that exist
      while tags[0] & 0b1 == 0:
        segment.add_control(points.pop(0))
        tags.pop(0)

      # set end point of segment (do not remove the point
      # from our list as it will be the start point for the
      # next segment)
      segment.set_end(points[0])

      # decompose the segment into points and add to the contour
      # we're building
      contour += segment.decompose(quality)

    # store the contour scaled up to -1024..1024
    glyph.contours.append(contour)

    # the start of the next contour is the end of this one
    start = end + 1
  
    #points = [p.round() for p in points]

  return glyph
    
class Encoder():
  def __init__(self, font, scale, quality = 1):
    self.face = freetype.Face(font)
    self.bbox_l = self.face.bbox.xMin
    self.bbox_t = self.face.bbox.yMin
    self.bbox_r = self.face.bbox.xMax
    self.bbox_b = self.face.bbox.yMax
    self.glyphs = {}
    self.packed_glyph_contours = {}

    self.quality = quality

    normalising_scale_factor = max(
      abs(self.bbox_l), abs(self.bbox_t), 
      abs(self.bbox_r), abs(self.bbox_b))

    self.scale_factor = scale / normalising_scale_factor    

    self.bbox_l *= self.scale_factor
    self.bbox_t *= self.scale_factor
    self.bbox_r *= self.scale_factor
    self.bbox_b *= self.scale_factor

  def __del__(self):
    # consume rogue error when destroying the face object
    try:
      del self.face
    except:
      pass 

  def get_glyph(self, codepoint):
    if codepoint not in self.glyphs:
      glyph = load_glyph(self.face, codepoint, self.scale_factor, self.quality)
      if not glyph:
        return None
      self.glyphs[codepoint] = glyph
      self.packed_glyph_contours[codepoint] = pack_glyph_contours(glyph)

    return self.glyphs[codepoint]

  def get_packed_glyph_contours(self, glyph):
    return self.packed_glyph_contours[glyph.codepoint]

  def get_packed_glyph_dictionary_entry(self, glyph, contour_start):
    return struct.pack(">HhhHHHL", glyph.codepoint, glyph.bbox_x, glyph.bbox_y, glyph.bbox_w, glyph.bbox_h, glyph.advance, contour_start)
