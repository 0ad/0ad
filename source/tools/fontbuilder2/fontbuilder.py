import cairo
import codecs
import math

import FontLoader
import Packer

# Representation of a rendered glyph
class Glyph(object):
    def __init__(self, ctx, renderstyle, char, idx, face, size):
        self.renderstyle = renderstyle
        self.char = char
        self.idx = idx
        self.face = face
        self.size = size
        self.glyph = (idx, 0, 0)

        if not ctx.get_font_face() == self.face:
            ctx.set_font_face(self.face)
            ctx.set_font_size(self.size)
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
        if not ctx.get_font_face() == self.face:
            ctx.set_font_face(self.face)
            ctx.set_font_size(self.size)
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
def setup_context(width, height, renderstyle):
    format = (cairo.FORMAT_ARGB32 if "colour" in renderstyle else cairo.FORMAT_A8)
    surface = cairo.ImageSurface(format, width, height)
    ctx = cairo.Context(surface)
    ctx.set_line_join(cairo.LINE_JOIN_ROUND)
    return ctx, surface

def generate_font(outname, ttfNames, loadopts, size, renderstyle, dsizes):

    faceList = []
    indexList = []
    for i in range(len(ttfNames)):
        (face, indices) = FontLoader.create_cairo_font_face_for_file("../../../binaries/data/tools/fontbuilder/fonts/%s" % ttfNames[i], 0, loadopts)
        faceList.append(face)
        if not ttfNames[i] in dsizes:
            dsizes[ttfNames[i]] = 0
        indexList.append(indices)

    (ctx, _) = setup_context(1, 1, renderstyle)

    # TODO this gets the line height from the default font
    # while entire texts can be in the fallback font
    ctx.set_font_face(faceList[0]);
    ctx.set_font_size(size + dsizes[ttfNames[0]])
    (_, _, linespacing, _, _) = ctx.font_extents()

    # Estimate the 'average' height of text, for vertical center alignment
    charheight = round(ctx.glyph_extents([(indexList[0]("I"), 0.0, 0.0)])[3])

    # Translate all the characters into glyphs
    # (This is inefficient if multiple characters have the same glyph)
    glyphs = []
    #for c in chars:
    for c in range(0x20, 0xFFFD):
        for i in range(len(indexList)):
            idx = indexList[i](unichr(c))
            if c == 0xFFFD and idx == 0: # use "?" if the missing-glyph glyph is missing
                idx = indexList[i]("?")
            if idx:
                glyphs.append(Glyph(ctx, renderstyle, unichr(c), idx, faceList[i], size + dsizes[ttfNames[i]]))
                break

    # Sort by decreasing height (tie-break on decreasing width)
    glyphs.sort(key = lambda g: (-g.h, -g.w))

    # Try various sizes to pack the glyphs into
    sizes = []
    for h in [32, 64, 128, 256, 512, 1024, 2048, 4096]:
        sizes.append((h, h))
        sizes.append((h*2, h))
    sizes.sort(key = lambda (w, h): (w*h, max(w, h))) # prefer smaller and squarer

    for w, h in sizes:
        try:
            # Using the dump pacher usually creates bigger textures, but runs faster
            # In practice the size difference is so small it always ends up in the same size
            packer = Packer.DumbRectanglePacker(w, h)
            #packer = Packer.CygonRectanglePacker(w, h)
            for g in glyphs:
                g.pack(packer)
        except Packer.OutOfSpaceError:
            continue

        ctx, surface = setup_context(w, h, renderstyle)
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
        # sorting unneeded, as glyphs are added in increasing order
        #glyphs.sort(key = lambda g: ord(g.char))
        for g in glyphs:
            x0 = g.x0
            y0 = g.y0
            # UGLY HACK: see http://trac.wildfiregames.com/ticket/1039 ;
            # to handle a-macron-acute characters without the hassle of
            # doing proper OpenType GPOS layout (which the  font
            # doesn't support anyway), we'll just shift the combining acute
            # glyph by an arbitrary amount to make it roughly the right
            # place when used after an a-macron glyph.
            if ord(g.char) == 0x0301:
                y0 += charheight/3

            fnt.write("%d %d %d %d %d %d %d %d\n" % (ord(g.char), g.pos.x, h-g.pos.y, g.w, g.h, -x0, y0, g.xadvance))

        fnt.close()

        return
    print "Failed to fit glyphs in texture"

filled = { "fill": [(1, 1, 1, 1)] }
stroked1 = { "colour": True, "stroke": [((0, 0, 0, 1), 2.0), ((0, 0, 0, 1), 2.0)], "fill": [(1, 1, 1, 1)] }
stroked2 = { "colour": True, "stroke": [((0, 0, 0, 1), 2.0)], "fill": [(1, 1, 1, 1), (1, 1, 1, 1)] }
stroked3 = { "colour": True, "stroke": [((0, 0, 0, 1), 2.5)], "fill": [(1, 1, 1, 1), (1, 1, 1, 1)] }

# For extra glyph support, add your preferred font to the font array
Sans = (["LinBiolinum_Rah.ttf","FreeSans.ttf"], FontLoader.FT_LOAD_DEFAULT)
Sans_Bold = (["LinBiolinum_RBah.ttf","FreeSansBold.ttf"], FontLoader.FT_LOAD_DEFAULT)
Sans_Italic = (["LinBiolinum_RIah.ttf","FreeSansOblique.ttf"], FontLoader.FT_LOAD_DEFAULT)
SansMono = (["DejaVuSansMono.ttf","FreeMono.ttf"], FontLoader.FT_LOAD_DEFAULT)
Serif = (["texgyrepagella-regular.otf","FreeSerif.ttf"], FontLoader.FT_LOAD_NO_HINTING)
Serif_Bold = (["texgyrepagella-bold.otf","FreeSerifBold.ttf"], FontLoader.FT_LOAD_NO_HINTING)

# Define the size differences used to render different fallback fonts
# I.e. when adding a fallback font has smaller glyphs than the original, you can bump it
dsizes = {'HanaMinA.ttf': 2} # make the glyphs for the (chinese font 2 pts bigger)

fonts = (
    ("mono-10", SansMono, 10, filled),
    ("mono-stroke-10", SansMono, 10, stroked2),
    ("sans-9", Sans, 9, filled),
    ("sans-10", Sans, 10, filled),
    ("sans-12", Sans, 12, filled),
    ("sans-13", Sans, 13, filled),
    ("sans-14", Sans, 14, filled),
    ("sans-16", Sans, 16, filled),
    ("sans-bold-12", Sans_Bold, 12, filled),
    ("sans-bold-13", Sans_Bold, 13, filled),
    ("sans-bold-14", Sans_Bold, 14, filled),
    ("sans-bold-16", Sans_Bold, 16, filled),
    ("sans-bold-18", Sans_Bold, 18, filled),
    ("sans-bold-20", Sans_Bold, 20, filled),
    ("sans-bold-22", Sans_Bold, 22, filled),
    ("sans-bold-24", Sans_Bold, 24, filled),
    ("sans-stroke-12", Sans, 12, stroked2),
    ("sans-bold-stroke-12", Sans_Bold, 12, stroked3),
    ("sans-stroke-13", Sans, 13, stroked2),
    ("sans-bold-stroke-13", Sans_Bold, 13, stroked3),
    ("sans-stroke-14", Sans, 14, stroked2),
    ("sans-bold-stroke-14", Sans_Bold, 14, stroked3),
    ("sans-stroke-16", Sans, 16, stroked2),

    ("serif-9", Serif, 9, filled),
    ("serif-12", Serif, 12, filled),
    ("serif-13", Serif, 13, filled),
    ("serif-14", Serif, 14, filled),
    ("serif-16", Serif, 16, filled),
    ("serif-bold-12", Serif_Bold, 12, filled),
    ("serif-bold-13", Serif_Bold, 13, filled),
    ("serif-bold-14", Serif_Bold, 14, filled),
    ("serif-bold-16", Serif_Bold, 16, filled),
    ("serif-bold-18", Serif_Bold, 18, filled),
    ("serif-bold-20", Serif_Bold, 20, filled),
    ("serif-bold-22", Serif_Bold, 22, filled),
    ("serif-bold-24", Serif_Bold, 24, filled),
    ("serif-stroke-12", Serif, 12, stroked2),
    ("serif-bold-stroke-12", Serif_Bold, 12, stroked3),
    ("serif-stroke-13", Serif, 13, stroked2),
    ("serif-bold-stroke-13", Serif_Bold, 13, stroked3),
    ("serif-stroke-14", Serif, 14, stroked2),
    ("serif-bold-stroke-14", Serif_Bold, 14, stroked3),
    ("serif-stroke-16", Serif, 16, stroked2),
)

for (name, (fontnames, loadopts), size, style) in fonts:
    print "%s..." % name
    generate_font("../../../binaries/data/mods/public/fonts/%s" % name, fontnames, loadopts, size, style, dsizes)
