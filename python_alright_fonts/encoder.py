import freetype, struct
from . import Glyph, Point

# contour encoding
# ===========================================================================

def pack_glyph_contours(glyph):
  result = bytes()
  for contour in glyph.contours:      
    result += struct.pack(">H", len(contour))
    for point in contour:
      result += struct.pack(">bb", point.x, point.y)
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
  

def load_glyph(face, codepoint, scale_factor, quality=1):
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
      if (tags[0]):
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

    # store the contour
    glyph.contours.append(contour)

    # the start of the next contour is the end of this one
    start = end + 1
  
  return glyph
    
class Encoder():
  def __init__(self, font, quality = 1):
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

    self.scale_factor = 127 / normalising_scale_factor    

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
    return self.glyphs[codepoint]

  def get_packed_glyph(self, glyph):
    self.packed_glyph_contours[glyph.codepoint] = pack_glyph_contours(glyph)
    pack_format = ">HbbBBBH"
    return struct.pack(pack_format, glyph.codepoint, 
      glyph.bbox_x, glyph.bbox_y, glyph.bbox_w, glyph.bbox_h, glyph.advance, 
      len(self.packed_glyph_contours[glyph.codepoint]))

  def get_packed_glyph_contours(self, glyph):
    return self.packed_glyph_contours[glyph.codepoint]