lib_LTLIBRARIES = libIL.la

libIL_la_SOURCES = il_alloc.c     il_doom.c      il_stack.c    il_pcx.c        il_rle.c \
                   il_bits.c      il_endian.c    il_jpeg.c     il_pic.c        il_sgi.c \
                   il_bmp.c       il_error.c     il_lif.c      il_png.c        il_states.c \
                   il_convbuff.c  il_files.c     il_main.c     il_pnm.c        il_targa.c \
                   il_convert.c   il_gif.c       il_manip.c    il_profiles.c   il_tiff.c \
                   il_cut.c       il_header.c    il_mdl.c      il_quantizer.c  il_utility.c \
                   il_dcx.c       il_icon.c      il_mng.c      il_raw.c        il_wal.c \
                   il_dds.c       il_internal.c  il_pal.c      il_rawdata.c    il_devil.c \
                   il_io.c        il_pcd.c       il_register.c il_psd.c        il_fastconv.c \
                   il_neuquant.c  il_dds-save.c  il_psp.c      il_pix.c        il_pxr.c \
				   il_xpm.c

CFLAGS = @LIBIL_CFLAGS@
libIL_la_LDFLAGS = -no-undefined @VERSIONINFO@
libIL_la_LIBADD = @LIBPNG@ @LIBJPG@ @LIBTIFF@ @LIBGIF@ @LIBMNG@ @LIBLCMS@

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src-IL/include

