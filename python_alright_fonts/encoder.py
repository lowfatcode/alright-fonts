import freetype, struct
from . import Glyph, Point
from simplification.cutil import simplify_coords_vwp


def load_glyph(face, codepoint, scale_factor, quality=1, complexity=3):
  # glyph doesn't exist in face
  if face.get_char_index(codepoint) == 0:
    return None

  # glyph isn't printable
  if not chr(codepoint).isprintable():
    return None

  # load the glyph
  face.load_char(codepoint, freetype.FT_LOAD_PEDANTIC)

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
  #start = 0

  def move_to(p, ctx):
    # Move to always starts a new contour
    ctx.contours.append([[p.x, p.y]])

  def line_to(a, ctx):
    ctx.contours[-1].append([a.x, a.y])

  def quadratic_bezier(t, src, c1, dst):
    return [
      (1 - t) * (1 - t) * src.x + 2 * (1 - t) * t * c1.x + t * t * dst.x,
      (1 - t) * (1 - t) * src.y + 2 * (1 - t) * t * c1.y + t * t * dst.y
    ]

  def conic_to(c1, dst, ctx):
    # Draw a quadratic bezier from the previous point to dst, with control c1

    # Get the source point (last point in this contour)
    src = Point(ctx.contours[-1][-1][0], ctx.contours[-1][-1][1])
    c1 = Point(c1.x, c1.y)
    dst = Point(dst.x, dst.y)

    # simplify_coords_vwp will discard overlapping/proximal/redundant coords
    for i in range(complexity):
      t = i / complexity
      ctx.contours[-1].append(quadratic_bezier(t, src, c1, dst))

  def cubic_bezier(t, src, c1, c2, dst):
    return [
      ((1 - t) ** 3) * src.x + 3 * ((1 - t) ** 2) * t * c1.x + 3 * (1 - t) * (t ** 2) * c2.x + (t ** 3) * dst.x,
      ((1 - t) ** 3) * src.y + 3 * ((1 - t) ** 2) * t * c1.y + 3 * (1 - t) * (t ** 2) * c2.y + (t ** 3) * dst.y
    ]

  def cubic_to(c1, c2, dst, ctx):
    # Draw a cubic bezier from the previous point to dst, with controls c1 and c2

    # Get the source point (last point in this contour)
    src = Point(ctx.contours[-1][-1][0], ctx.contours[-1][-1][1])
    c1 = Point(c1.x, c1.y)
    c2 = Point(c2.x, c2.y)
    dst = Point(dst.x, dst.y)

    # simplify_coords_vwp will discard overlapping/proximal/redundant coords
    for i in range(complexity):
      t = i / complexity
      ctx.contours[-1].append(cubic_bezier(t, src, c1, c2, dst))

  outline.decompose(glyph, move_to=move_to, line_to=line_to, conic_to=conic_to, cubic_to=cubic_to)

  # Simplify, scale and round the final contours
  for i, c in enumerate(glyph.contours):
    glyph.contours[i] = [Point(p[0], p[1]).scale(scale_factor, -scale_factor).round() for p in simplify_coords_vwp(c, quality)]

  return glyph
    
class Encoder():
  def __init__(self, font, quality = 1, complexity = 3):
    self.face = freetype.Face(font)
    print(self.face.get_format())
    self.bbox_l = self.face.bbox.xMin
    self.bbox_t = self.face.bbox.yMin
    self.bbox_r = self.face.bbox.xMax
    self.bbox_b = self.face.bbox.yMax
    self.glyphs = {}
    self.packed_glyph_contours = {}

    self.quality = quality
    self.complexity = complexity

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
      glyph = load_glyph(self.face, codepoint, self.scale_factor, self.quality, self.complexity)
      if not glyph:
        return None
      self.glyphs[codepoint] = glyph
    return self.glyphs[codepoint]

  def get_packed_glyph(self, glyph):
    pack_format = ">HbbBBBB"
    return struct.pack(
      pack_format, 
      glyph.codepoint, 
      glyph.bbox_x, 
      glyph.bbox_y, 
      glyph.bbox_w, 
      glyph.bbox_h, 
      glyph.advance, 
      len(glyph.contours)
    )

  def get_packed_glyph_paths(self, glyph):
    result = bytes()
    for contour in glyph.contours:
      if len(contour) > 255:
        raise RuntimeError(f"Fatal: Contour too big! {len(contour)}")
      result += struct.pack(">B", len(contour))
    return result      

  def get_packed_glyph_path_points(self, glyph):
    result = bytes()
    for contour in glyph.contours:      
      for point in contour:
        result += struct.pack(">bb", point.x, point.y)
    return result      

  def total_path_count(self):
    total = 0
    for glyph in self.glyphs:
      total += len(self.glyphs[glyph].contours)
    return total
  
  def total_point_count(self):
    total = 0
    for glyph in self.glyphs:
      for contour in self.glyphs[glyph].contours:
        total += len(contour)
    return total  