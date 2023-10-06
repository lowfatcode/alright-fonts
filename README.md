<img src="logo.png" alt="Alright Fonts logo" width="500px">

# Alright Fonts - a font format for embedded and low resource platforms.

## Why?

Historically when drawing text microcontrollers have been limited to bitmapped fonts or very simple stroke based fonts - in part to minimise the space required to store font assets and also to make the rendering process fast enough to be done in realtime.

More recently the power and capacity of cheap, embeddable, microcontrollers has increased to the point where for a dollar and change you can be running at hundreds of megahertz with megabytes of flash storage alongside.

It is now viable to render filled, transformed, anti-aliased, and scalable characters that are comparable in quality to the text that we see on our computer screens every day. 

There is, however, still a sticking point. Existing font formats are complicated beasts that define shapes as collections of curves and control points, include entire state machines for pixel hinting, have character pair specific kerning tables, and digital rights management for.. well, yeah.

> The name Alright Fonts was inspired by the QOI (“Quite OK Image Format”) project. Find out more here: https://qoiformat.org/

## Alright Fonts are here to save the day!

Alright Fonts drops the extra bells and whistles and decomposes curves into small straight line segments.

The format is simple to parse, can be tuned for quality, size, speed, or a combination of all three, contains helper metrics for quickly measuring text, and produces surprisingly good results.

Features:

- support for TTF and OTF fonts
- small file size - Roboto Black printable ASCII set in ~4kB (~40 bytes per glyph)
- libraries for C17 and Python
- tunable settings to trade off file size, path complexity, and visual quality
- glyph dictionary includes advance and bounding box metrics for fast layout
- supports utf-8 codepoints or ascii character code
- support for non ASCII characters such as Kanji (海賊ロボ忍者さる) or Cyrillic
- easy to parse - entire specification is listed below
- coordinates scaled to power of two bounds avoiding slow divide when rendering
- create custom font packs containing only the characters that are needed
- decomposes all glyph paths (strokes) into simple polylines for easy rendering

Alright Fonts includes:

- `afinate` an extraction and encoding tool to create Alright Font (.af) files 
- `python_alright_fonts` a Python library for encoding and loading Alright Fonts
- `alright-fonts.h` a reference C17 implementation

The repository also includes some examples:

- `render-demo` a Python demo of rendering text

> The C17 reference renderer should be straightforward to embed in any project - alternatively it can be used as a guide for implementing your own renderer.

## Creating a font pack with the `afinate` tool

The `afinate` tool creates new font packs from a TTF or OTF font.

```bash
usage: afinate [-h] [--characters CHARACTERS] --font FONTFILE OUTFILE
```

- `--font FILE`: specify the font to use
- `--quiet`: don't output any status or debug messages during processing

Font data can be output either as a binary file or as source files for C(++) and Python.

- `--format`: output format, either `af` (default), `c`, or `python`
  - `af`: generates a binary font file for embedding with your linker or loading at runtime from a filesystem.
  - `c` generates a C(++) code file containing a const array of font data
  - `python` generates a Python code file containing an array of font data
- `--quality`: the quality of decomposed bezier curves, either `low`, `medium`, or `high` (default: `medium` - affects file size)
  
The list of characters to include can be specified in three ways:

  - default: full printable ASCII set (95 characters)
  - `--characters CHARACTERS`: a list of characters to include in the font pack
  - `--corpus FILE`: a text file containing all of the characters to include
  
For example:

```bash
./afinate --characters 'abcdefg' --font 'fonts/Roboto-Black.ttf' roboto-abcdefg.af
```

The file `roboto-abcdefg.af` is now ready to embed into your project.

## The Alright Fonts file format

An Alright Fonts file consists of a 12-byte header, followed by a dictionary of glyphs definitions, followed by the path and point data that define those glyphs.

|size (bytes)|description|count (`n`)|
|--:|---|---|
|`12`|header|1|
|`8` * `n`|glyph dictionary|total number of glyphs|
|`1` * `n`|glyph paths|total number of paths|
|`2` * `n`|glyph path points|total number of points|

### Header

The header contains several key pieces of information: a magic marker for identification, the total number of glyphs in the file, and a collection of flags (not yet used). Additionally, it provides the overall counts for paths and points, simplifying the allocation of buffers before parsing.

|size (bytes)|name|type|notes|
|--:|---|---|---|
|`4`|`"af!?"`|bytes|magic marker bytes|
|`2`|`flags`|unsigned integer|flags (reserved for future use)|
|`2`|`glyphs`|unsigned integer|number of glyphs|
|`2`|`paths`|unsigned integer|total number of paths|
|`2`|`points`|unsigned integer|total number of points|

#### Coordinate normalisation and scaling

All glyph coordinates - including bounding boxes, advances, and path points - are normalized to a common `-1` to `1` scale. This is achieved by dividing them by the largest dimension of the bounding box that encompasses all glyphs. Subsequently, these normalized coordinates are scaled up by multiplying them by `127`, bringing them into a range of `-128` to `127`.

This scaling factor was selected for several reasons:

- Encoding path coordinates as single bytes minimizes the file size.
- Offers sufficient resolution to maintain fine details, even in complex glyphs.
- Eliminates the need for costly division operations during rendering, as the scaling can be performed using the shift-right operation: `(size_px * coordinate) >> 8`.
  
#### Flags

Let's hedge our bets.

Being an English software developer ~~it's possible~~ an absolute certainty that I don't fully understand every nuance of every language used globally - heck, I can just barely handle my own. 

It's also likely that we may want, in future, to add a feature or two such as:

- Exclude glyph bounding boxes.
- Add glyph pair kerning data.
- Allow 4-byte character codepoints.
- Support a finer scale for coordinates (i.e. `-65536` to `65535`).
- Alternative packing methods for path data.
- Better support for vertical or left-to-right languages.

The `flags` field is designed for future extensibility, allowing parsers to implement none, some, or all upcoming features. If a parser encounters a `1` bit in the `flags` field for a feature it does not support, the parser should reject the file and return an error.

*There are currently no flags and this feature is reserved for future use.*

### Glyph dictionary

Following the header is the glyph dictionary which contains entries for all of the glyphs sorted by their utf-8 codepoint or ascii character code.

Each entry includes the character codepoint, some basic metrics, and length of its path data.

> The inclusion of bounding box and advance metrics makes it very performant to calculate the bounds of a piece of text (there is no need to interrogate the path data).

The glyphs are laid out one after another in the file:

|size (bytes)|name|type|notes|
|--:|---|---|---|
|`2`|`codepoint`|unsigned integer|utf-8 codepoint or ascii character code|
|`1`|`bbox_x`|signed integer|left edge of bounding box|
|`1`|`bbox_y`|signed integer|top edge of bounding box|
|`1`|`bbox_w`|unsigned integer|width of bounding box|
|`1`|`bbox_h`|unsigned integer|height of bounding box|
|`1`|`advance`|unsigned integer|horizontal or vertical advance|
|`1`|`paths`|unsigned integer|number of paths|

...and repeat for each glyph.

### Glyph paths

Immediately after the glyph dictionary comes the glyph path data. With each glyph in the same order they appear in the dictionary.

A glyph can have multiple paths, each with a different number of points.

|size (bytes)|name|type|notes|
|--:|---|---|---|
|`1`|`points`|unsigned integer|count of points in first path|
|..|..|..|..|
|`1`|`points n`|unsigned integer|count of points in nth path|

...and repeat for each glyph.

### Glyph path points

Immediately after the glyph paths come the glyph path points. With the points for each path in the same order they appear in the glyph paths.

Following on is the point data for the paths stored as `x`, `y` pairs.

|size (bytes)|name|type|notes|
|--:|---|---|---|
|`1`|`point 1 x`|signed integer|first point x component|
|`1`|`point 1 y`|signed integer|first point y component|
||..|..|..|
|`1`|`point n x`|signed integer|nth point x component|
|`1`|`point n y`|signed integer|nth point y component|

...and repeat for each glyph path.

## Examples

### Quality comparison

Here three Alright Fonts files have been generated containing the full set of printable ASCII characters. The font used was Roboto Black and the command line parameters to `afinate` were:

```bash
./afinate --font fonts/Roboto-Black.ttf --quality [low|medium|high]
```

|Low|Medium|High|
|---|---|---|
|3,657 bytes|4,495 bytes|5,681 bytes|
|![roboto-low](https://user-images.githubusercontent.com/297930/184846578-ceafa616-ca17-446b-ae8f-84a64fa79e44.jpeg)|![roboto-medium](https://user-images.githubusercontent.com/297930/184846646-c84d55a2-17d7-486e-9616-a17bada7ea12.jpg)|![roboto-high](https://user-images.githubusercontent.com/297930/184846678-8343fa2d-e128-4516-a7bb-5e0c247fae23.jpg)|

The differences are easier to see when viewing the images at their original size - click to open in a new tab.

### Python `render-demo`

You can pipe the output of `afinate` directly into the `render-demo` example script to product a swatch image.

```bash
./afinate --font fonts/Roboto-Black.ttf --quality high - | ./render-demo
```
