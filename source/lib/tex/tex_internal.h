/**
 * =========================================================================
 * File        : tex_internal.h
 * Project     : 0 A.D.
 * Description : private texture loader helper functions
 * =========================================================================
 */

// license: GPL; see lib/license.txt

#ifndef INCLUDED_TEX_INTERNAL
#define INCLUDED_TEX_INTERNAL

#include "lib/allocators/dynarray.h"
#include "lib/file/io/io.h"	// io_Allocate

/**
 * check if the given texture format is acceptable: 8bpp grey,
 * 24bpp color or 32bpp color+alpha (BGR / upside down are permitted).
 * basically, this is the "plain" format understood by all codecs and
 * tex_codec_plain_transform.
 * @param bpp bits per pixel
 * @param flags TexFlags
 * @return LibError
 **/
extern LibError tex_validate_plain_format(size_t bpp, size_t flags);


/**
 * indicate if the two vertical orientations match.
 *
 * used by tex_codec.
 * 
 * @param src_flags TexFlags, used to extract the orientation.
 * we ask for this instead of src_orientation so callers don't have to
 * mask off TEX_ORIENTATION.
 * @param dst_orientation orientation to compare against.
 * can be one of TEX_BOTTOM_UP, TEX_TOP_DOWN, or 0 for the
 * "global orientation".
 * @return bool
 **/
extern bool tex_orientations_match(size_t src_flags, size_t dst_orientation);

#endif	// #ifndef INCLUDED_TEX_INTERNAL
