# Dumps lines containing the name of a font followed by a space-separated
# list of decimal codepoints (from U+0001 to U+FFFF) for which that font
# contains some glyph data.

import FontLoader

def dump_font(ttf):

    (face, indexes) = FontLoader.create_cairo_font_face_for_file("../../../binaries/data/tools/fontbuilder/fonts/%s" % ttf, 0, FontLoader.FT_LOAD_DEFAULT)

    mappings = [ (c, indexes(unichr(c))) for c in range(1, 65535) ]
    print ttf,
    print ' '.join(str(c) for (c, g) in mappings if g != 0)

dump_font("DejaVuSansMono.ttf")
dump_font("DejaVuSans.ttf")
dump_font("texgyrepagella-regular.otf")
dump_font("texgyrepagella-bold.otf")
