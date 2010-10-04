# Adapted from http://cairographics.org/freetypepython/

import ctypes
import cairo
import sys

CAIRO_STATUS_SUCCESS = 0
FT_Err_Ok = 0

# find required libraries (platform specific)
if sys.platform == "win32":
    ft_lib = "freetype6.dll"
    lc_lib = "libcairo-2.dll"
else:
    ft_lib = "libfreetype.so.6"
    lc_lib = "libcairo.so.2"

_freetype_so = ctypes.CDLL (ft_lib)
_cairo_so = ctypes.CDLL (lc_lib)

# initialize freetype
_ft_lib = ctypes.c_void_p ()
if FT_Err_Ok != _freetype_so.FT_Init_FreeType (ctypes.byref (_ft_lib)):
    raise "Error initialising FreeType library."

_surface = cairo.ImageSurface (cairo.FORMAT_A8, 0, 0)

class PycairoContext(ctypes.Structure):
    _fields_ = [("PyObject_HEAD", ctypes.c_byte * object.__basicsize__),
        ("ctx", ctypes.c_void_p),
        ("base", ctypes.c_void_p)]

def create_cairo_font_face_for_file (filename, faceindex=0, loadoptions=0):
    # create freetype face
    ft_face = ctypes.c_void_p()
    cairo_ctx = cairo.Context (_surface)
    cairo_t = PycairoContext.from_address(id(cairo_ctx)).ctx
    _cairo_so.cairo_ft_font_face_create_for_ft_face.restype = ctypes.c_void_p
    if FT_Err_Ok != _freetype_so.FT_New_Face (_ft_lib, filename, faceindex, ctypes.byref(ft_face)):
        raise "Error creating FreeType font face for " + filename

    # create cairo font face for freetype face
    cr_face = _cairo_so.cairo_ft_font_face_create_for_ft_face (ft_face, loadoptions)
    if CAIRO_STATUS_SUCCESS != _cairo_so.cairo_font_face_status (cr_face):
        raise "Error creating cairo font face for " + filename

    _cairo_so.cairo_set_font_face (cairo_t, cr_face)
    if CAIRO_STATUS_SUCCESS != _cairo_so.cairo_status (cairo_t):
        raise "Error creating cairo font face for " + filename

    face = cairo_ctx.get_font_face ()

    indexes = lambda char: _freetype_so.FT_Get_Char_Index(ft_face, ord(char))

    return (face, indexes)
