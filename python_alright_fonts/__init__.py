import math

class Point():
  def __init__(self, x=None, y=None):
    if x != None:
      if isinstance(x, tuple):
        self.x = x[0]
        self.y = x[1]
      else:
        self.x = x
        self.y = y
    else:
      self.x = 0
      self.y = 0
    
  def scale(self, scale_x, scale_y=None):
    if not scale_y:
      scale_y = scale_x
    return Point(self.x * scale_x, self.y * scale_y)
  
  def round(self):
    return Point(round(self.x), round(self.y))

  def distance(self, other):
    dx = abs(self.x - other.x)
    dy = abs(self.y - other.y)
    return math.sqrt(dx * dx + dy * dy)

  def delta(self, p):
    if p:
      return Point(self.x - p.x, self.y - p.y)
    return Point(self.x, self.y)

  def __repr__(self):
    return "Point({}, {})".format(self.x, self.y)


class Glyph():
  def __init__(self):
    self.codepoint = None
    self.advance = None
    self.bbox_x = None
    self.bbox_y = None
    self.bbox_w = None
    self.bbox_h = None
    self.contours = []
  
  def __repr__(self):
    return "{} ({},{}: {}x{}) [{}]".format(self.codepoint, self.bbox_x, self.bbox_y, self.bbox_w, self.bbox_h, self.advance)

class Face():
  def __init__(self):
    self.glyphs = {}
    pass

  def get_glyph(self, codepoint):
    if codepoint not in self.glyphs:
      return None

    return self.glyphs[codepoint]    

from python_alright_fonts.encoder import Encoder
from python_alright_fonts.loader import load_font
