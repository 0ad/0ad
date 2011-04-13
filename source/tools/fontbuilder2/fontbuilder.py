import cairo
import codecs
import math

import FontLoader
import Packer

# Representation of a rendered glyph
class Glyph(object):
    def __init__(self, ctx, renderstyle, char, idx):
        self.renderstyle = renderstyle
        self.char = char
        self.idx = idx
        self.glyph = (idx, 0, 0)

        extents = ctx.glyph_extents([self.glyph])

        self.xadvance = round(extents[4])

        # Find the bounding box of strokes and/or fills:

        inf = 1e300 * 1e300
        bb = [inf, inf, -inf, -inf]

        if "stroke" in self.renderstyle:
            for (c, w) in self.renderstyle["stroke"]:
                ctx.set_line_width(w)
                ctx.glyph_path([self.glyph])
                e = ctx.stroke_extents()
                bb = (min(bb[0], e[0]), min(bb[1], e[1]), max(bb[2], e[2]), max(bb[3], e[3]))
                ctx.new_path()

        if "fill" in self.renderstyle:
            ctx.glyph_path([self.glyph])
            e = ctx.fill_extents()
            bb = (min(bb[0], e[0]), min(bb[1], e[1]), max(bb[2], e[2]), max(bb[3], e[3]))
            ctx.new_path()

        bb = (math.floor(bb[0]), math.floor(bb[1]), math.ceil(bb[2]), math.ceil(bb[3]))

        self.x0 = -bb[0]
        self.y0 = -bb[1]
        self.w = bb[2] - bb[0]
        self.h = bb[3] - bb[1]

        # Force multiple of 4, to avoid leakage across S3TC blocks
        # (TODO: is this useful?)
        #self.w += (4 - (self.w % 4)) % 4
        #self.h += (4 - (self.h % 4)) % 4

    def pack(self, packer):
        self.pos = packer.Pack(self.w, self.h)

    def render(self, ctx):
        ctx.save()
        ctx.translate(self.x0, self.y0)
        ctx.translate(self.pos.x, self.pos.y)

        # Render each stroke, and then each fill on top of it

        if "stroke" in self.renderstyle:
            for ((r, g, b, a), w) in self.renderstyle["stroke"]:
                ctx.set_line_width(w)
                ctx.set_source_rgba(r, g, b, a)
                ctx.glyph_path([self.glyph])
                ctx.stroke()

        if "fill" in self.renderstyle:
            for (r, g, b, a) in self.renderstyle["fill"]:
                ctx.set_source_rgba(r, g, b, a)
                ctx.glyph_path([self.glyph])
                ctx.fill()

        ctx.restore()

# Load the set of characters contained in the given text file
def load_char_list(filename):
    f = codecs.open(filename, "r", "utf-8")
    chars = f.read()
    f.close()
    return set(chars)

# Construct a Cairo context and surface for rendering text with the given parameters
def setup_context(width, height, face, size, renderstyle):
    format = (cairo.FORMAT_ARGB32 if "colour" in renderstyle else cairo.FORMAT_A8)
    surface = cairo.ImageSurface(format, width, height)
    ctx = cairo.Context(surface)
    ctx.set_font_face(face)
    ctx.set_font_size(size)
    ctx.set_line_join(cairo.LINE_JOIN_ROUND)
    return ctx, surface

def generate_font(chars, outname, ttf, loadopts, size, renderstyle):

    (face, indexes) = FontLoader.create_cairo_font_face_for_file(ttf, 0, loadopts)

    (ctx, _) = setup_context(1, 1, face, size, renderstyle)

    (ascent, descent, linespacing, _, _) = ctx.font_extents()

    # Estimate the 'average' height of text, for vertical center alignment
    charheight = round(ctx.glyph_extents([(indexes("I"), 0.0, 0.0)])[3])

    # Translate all the characters into glyphs
    # (This is inefficient if multiple characters have the same glyph)
    glyphs = []
    for c in chars:
        idx = indexes(c)
        if ord(c) == 0xFFFD and idx == 0: # use "?" if the missing-glyph glyph is missing
            idx = indexes("?")
        if idx:
            glyphs.append(Glyph(ctx, renderstyle, c, idx))

    # Sort by decreasing height (tie-break on decreasing width)
    glyphs.sort(key = lambda g: (-g.h, -g.w))

    # Try various sizes to pack the glyphs into
    sizes = []
    for h in [32, 64, 128, 256, 512, 1024]:
        for w in [32, 64, 128, 256, 512, 1024]:
            sizes.append((w, h))
    sizes.sort(key = lambda (w, h): (w*h, max(w, h))) # prefer smaller and squarer

    for w, h in sizes:
        try:
            #packer = Packer.DumbRectanglePacker(w, h)
            packer = Packer.CygonRectanglePacker(w, h)
            for g in glyphs:
                g.pack(packer)
        except Packer.OutOfSpaceError:
            continue

        ctx, surface = setup_context(w, h, face, size, renderstyle)
        for g in glyphs:
            g.render(ctx)
        surface.write_to_png("%s.png" % outname)

        # Output the .fnt file with all the glyph positions etc
        fnt = open("%s.fnt" % outname, "w")
        fnt.write("101\n")
        fnt.write("%d %d\n" % (w, h))
        fnt.write("%s\n" % ("rgba" if "colour" in renderstyle else "a"))
        fnt.write("%d\n" % len(glyphs))
        fnt.write("%d\n" % linespacing)
        fnt.write("%d\n" % charheight)
        glyphs.sort(key = lambda g: ord(g.char))
        for g in glyphs:
            fnt.write("%d %d %d %d %d %d %d %d\n" % (ord(g.char), g.pos.x, h-g.pos.y, g.w, g.h, -g.x0, g.y0, g.xadvance))
        fnt.close()

        return
    print "Failed to fit glyphs in texture"

filled = { "fill": [(1, 1, 1, 1)] }
stroked1 = { "colour": True, "stroke": [((0, 0, 0, 1), 2.0), ((0, 0, 0, 1), 2.0)], "fill": [(1, 1, 1, 1)] }
stroked2 = { "colour": True, "stroke": [((0, 0, 0, 1), 2.0)], "fill": [(1, 1, 1, 1), (1, 1, 1, 1)] }
stroked3 = { "colour": True, "stroke": [((0, 0, 0, 1), 2.5)], "fill": [(1, 1, 1, 1), (1, 1, 1, 1)] }

chars = load_char_list("charset.txt")

DejaVuSansMono = ("DejaVuSansMono.ttf", FontLoader.FT_LOAD_DEFAULT)
DejaVuSans = ("DejaVuSans.ttf", FontLoader.FT_LOAD_DEFAULT)
PagellaRegular = ("texgyrepagella-regular.otf", FontLoader.FT_LOAD_NO_HINTING)
PagellaBold = ("texgyrepagella-bold.otf", FontLoader.FT_LOAD_NO_HINTING)

fonts = (
    ("mono-10", DejaVuSansMono, 10, filled),
    ("mono-stroke-10", DejaVuSansMono, 10, stroked2),
    ("sans-10", DejaVuSans, 10, filled),
    ("serif-9", PagellaRegular, 9, filled),
    ("serif-12", PagellaRegular, 12, filled),
    ("serif-13", PagellaRegular, 13, filled),
    ("serif-14", PagellaRegular, 14, filled),
    ("serif-16", PagellaRegular, 16, filled),
    ("serif-bold-12", PagellaBold, 12, filled),
    ("serif-bold-13", PagellaBold, 13, filled),
    ("serif-bold-14", PagellaBold, 14, filled),
    ("serif-bold-16", PagellaBold, 16, filled),
    ("serif-bold-18", PagellaBold, 18, filled),
    ("serif-bold-20", PagellaBold, 20, filled),
    ("serif-bold-22", PagellaBold, 22, filled),
    ("serif-bold-24", PagellaBold, 24, filled),
    ("serif-stroke-14", PagellaRegular, 14, stroked2),
    ("serif-bold-stroke-14", PagellaBold, 14, stroked3),
)

for (name, (fontname, loadopts), size, style) in fonts:
    print "%s..." % name
    generate_font(chars, "../../../binaries/data/mods/public/fonts/%s" % name, "../../../binaries/data/tools/fontbuilder/fonts/%s" % fontname, loadopts, size, style)
