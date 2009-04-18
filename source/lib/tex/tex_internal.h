/* Copyright (C) 2009 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * private texture loader helper functions
 */

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
