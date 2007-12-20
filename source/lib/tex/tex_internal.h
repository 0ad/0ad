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

#include "lib/allocators.h"	// DynArray
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
extern LibError tex_validate_plain_format(uint bpp, uint flags);


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
extern bool tex_orientations_match(uint src_flags, uint dst_orientation);


/**
 * decode an in-memory texture file into texture object.
 *
 * split out of tex_load to ease resource cleanup and allow
 * decoding images without needing to write out to disk.
 *
 * @param data input data
 * @param data_size its size [bytes]
 * @param t output texture object.
 * @return LibError.
 **/
extern LibError tex_decode(const u8* data, size_t data_size, Tex* t);

/**
 * encode a texture into a memory buffer in the desired file format.
 *
 * @param t input texture object
 * @param fn filename; only used to determine the desired file format
 * (via extension)
 * @param da output memory array. allocated here; caller must free it
 * when no longer needed. invalid unless function succeeds.
 * @return LibError
 **/
extern LibError tex_encode(Tex* t, const char* fn, DynArray* da);

#endif	// #ifndef INCLUDED_TEX_INTERNAL
