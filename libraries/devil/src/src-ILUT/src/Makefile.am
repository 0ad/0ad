lib_LTLIBRARIES = libILUT.la

libILUT_la_SOURCES = ilut_allegro.c     ilut_main.c \
                     ilut_sdlsurface.c  ilut_states.c \
                     ilut_internal.c    ilut_opengl.c \
                     ilut_win32.c       ilut_directx.c \
					 ilut_directx9.c

CFLAGS = @LIBILUT_CFLAGS@

libILUT_la_LDFLAGS = -no-undefined @VERSIONINFO@ \
                     -L$(top_srcdir)/src-IL/src \
                     -L$(top_srcdir)/src-ILU/src \
                     @LIBSDL@ @LIBALLEGRO@ @LIBILUT_WIN32LIBPATH@

libILUT_la_LIBADD = -lIL -lILU @LIBGL@ @LIBGDI32@

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src-ILUT/include \
           @HSDL@ @HALLEGRO@ @X_CFLAGS@
