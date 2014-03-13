/* Copyright (c) 2014 Wildfire Games
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * read/write 2d texture files; allows conversion between pixel formats
 * and automatic orientation correction.
 */

/**

Introduction
------------

This module allows reading/writing 2d images in various file formats and
encapsulates them in Tex objects.
It supports converting between pixel formats; this is to an extent done
automatically when reading/writing. Provision is also made for flipping
all images to a default orientation.


Format Conversion
-----------------

Image file formats have major differences in their native pixel format:
some store in BGR order, or have rows arranged bottom-up.
We must balance runtime cost/complexity and convenience for the
application (not dumping the entire problem on its lap).
That means rejecting really obscure formats (e.g. right-to-left pixels),
but converting everything else to uncompressed RGB "plain" format
except where noted in enum TexFlags (1).

Note: conversion is implemented as a pipeline: e.g. "DDS decompress +
vertical flip" would be done by decompressing to RGB (DDS codec) and then
flipping (generic transform). This is in contrast to all<->all
conversion paths: that would be much more complex, if more efficient.

Since any kind of preprocessing at runtime is undesirable (the absolute
priority is minimizing load time), prefer file formats that are
close to the final pixel format.

1) one of the exceptions is S3TC compressed textures. glCompressedTexImage2D
   requires these be passed in their original format; decompressing would be
   counterproductive. In this and similar cases, TexFlags indicates such
   deviations from the plain format.


Default Orientation
-------------------

After loading, all images (except DDS, because its orientation is
indeterminate) are automatically converted to the global row
orientation: top-down or bottom-up, as specified by
tex_set_global_orientation. If that isn't called, the default is top-down
to match Photoshop's DDS output (since this is meant to be the
no-preprocessing-required optimized format).
Reasons to change it might be to speed up loading bottom-up
BMP or TGA images, or to match OpenGL's convention for convenience;
however, be aware of the abovementioned issues with DDS.

Rationale: it is not expected that this will happen at the renderer layer
(a 'flip all texcoords' flag is too much trouble), so the
application would have to do the same anyway. By taking care of it here,
we unburden the app and save time, since some codecs (e.g. PNG) can
flip for free when loading.


Codecs / IO Implementation
--------------------------

To ease adding support for new formats, they are organized as codecs.
The interface aims to minimize code duplication, so it's organized
following the principle of "Template Method" - this module both
calls into codecs, and provides helper functions that they use.

IO is done via VFS, but the codecs are decoupled from this and
work with memory buffers. Access to them is endian-safe.

When "writing", the image is put into an expandable memory region.
This supports external libraries like libpng that do not know the
output size beforehand, but avoids the need for a buffer between
library and IO layer. Read and write are zero-copy.

**/

#ifndef INCLUDED_TEX
#define INCLUDED_TEX

#include "lib/res/handle.h"
#include "lib/os_path.h"
#include "lib/file/vfs/vfs_path.h"
#include "lib/allocators/dynarray.h"


namespace ERR
{
	const Status TEX_UNKNOWN_FORMAT      = -120100;
	const Status TEX_INCOMPLETE_HEADER   = -120101;
	const Status TEX_FMT_INVALID         = -120102;
	const Status TEX_INVALID_COLOR_TYPE  = -120103;
	const Status TEX_NOT_8BIT_PRECISION  = -120104;
	const Status TEX_INVALID_LAYOUT      = -120105;
	const Status TEX_COMPRESSED          = -120106;
	const Status TEX_INVALID_SIZE        = -120107;
}

namespace WARN
{
	const Status TEX_INVALID_DATA        = +120108;
}

namespace INFO
{
	const Status TEX_CODEC_CANNOT_HANDLE = +120109;
}


/**
 * flags describing the pixel format. these are to be interpreted as
 * deviations from "plain" format, i.e. uncompressed RGB.
 **/
enum TexFlags
{
	/**
	 * flags & TEX_DXT is a field indicating compression.
	 * if 0, the texture is uncompressed;
	 * otherwise, it holds the S3TC type: 1,3,5 or DXT1A.
	 * not converted by default - glCompressedTexImage2D receives
	 * the compressed data.
	 **/
	TEX_DXT = 0x7,	 // mask

	/**
	 * we need a special value for DXT1a to avoid having to consider
	 * flags & TEX_ALPHA to determine S3TC type.
	 * the value is arbitrary; do not rely on it!
	 **/
	DXT1A = 7,

	/**
	 * indicates B and R pixel components are exchanged. depending on
	 * flags & TEX_ALPHA or bpp, this means either BGR or BGRA.
	 * not converted by default - it's an acceptable format for OpenGL.
	 **/
	TEX_BGR = 0x08,

	/**
	 * indicates the image contains an alpha channel. this is set for
	 * your convenience - there are many formats containing alpha and
	 * divining this information from them is hard.
	 * (conversion is not applicable here)
	 **/
	TEX_ALPHA = 0x10,

	/**
	 * indicates the image is 8bpp greyscale. this is required to
	 * differentiate between alpha-only and intensity formats.
	 * not converted by default - it's an acceptable format for OpenGL.
	 **/
	TEX_GREY = 0x20,

	/**
	 * flags & TEX_ORIENTATION is a field indicating orientation,
	 * i.e. in what order the pixel rows are stored.
	 *
	 * tex_load always sets this to the global orientation
	 * (and flips the image accordingly to match).
	 * texture codecs may in intermediate steps during loading set this
	 * to 0 if they don't know which way around they are (e.g. DDS),
	 * or to whatever their file contains.
	 **/
	TEX_BOTTOM_UP = 0x40,
	TEX_TOP_DOWN  = 0x80,
	TEX_ORIENTATION = TEX_BOTTOM_UP|TEX_TOP_DOWN,	 /// mask

	/**
	 * indicates the image data includes mipmaps. they are stored from lowest
	 * to highest (1x1), one after the other.
	 * (conversion is not applicable here)
	 **/
	TEX_MIPMAPS = 0x100,

	TEX_UNDEFINED_FLAGS = ~0x1FF
};

/**
 * stores all data describing an image.
 * we try to minimize size, since this is stored in OglTex resources
 * (which are big and pushing the h_mgr limit).
 **/
struct Tex
{
	/**
	 * file buffer or image data. note: during the course of transforms
	 * (which may occur when being loaded), this may be replaced with
	 * a new buffer (e.g. if decompressing file contents).
	 **/
	shared_ptr<u8> m_Data;

	size_t m_DataSize;

	/**
	 * offset to image data in file. this is required since
	 * tex_get_data needs to return the pixels, but data
	 * returns the actual file buffer. zero-copy load and
	 * write-back to file is also made possible.
	 **/
	size_t m_Ofs;

	size_t m_Width;
	size_t m_Height;
	size_t m_Bpp;

	/// see TexFlags and "Format Conversion" in docs.
	size_t m_Flags;

	~Tex()
	{
		free();
	}

	/**
	 * Is the texture object valid and self-consistent?
	 * 
	 * @return Status
	 **/
	Status validate() const;

	/**
	 * free all resources associated with the image and make further
	 * use of it impossible.
	 *
	 * @return Status
	 **/
	void free();

	/**
	 * decode an in-memory texture file into texture object.
	 *
	 * FYI, currently BMP, TGA, JPG, JP2, PNG, DDS are supported - but don't
	 * rely on this (not all codecs may be included).
	 *
	 * @param data Input data.
	 * @param data_size Its size [bytes].
	 * @return Status.
	 **/
	Status decode(const shared_ptr<u8>& data, size_t data_size);

	/**
	 * encode a texture into a memory buffer in the desired file format.
	 *
	 * @param extension (including '.').
	 * @param da Output memory array. Allocated here; caller must free it
	 *		  when no longer needed. Invalid unless function succeeds.
	 * @return Status
	 **/
	Status encode(const OsPath& extension, DynArray* da);
	
	/**
	 * store the given image data into a Tex object; this will be as if
	 * it had been loaded via tex_load.
	 *
	 * rationale: support for in-memory images is necessary for
	 *   emulation of glCompressedTexImage2D and useful overall.
	 *   however, we don't want to provide an alternate interface for each API;
	 *   these would have to be changed whenever fields are added to Tex.
	 *   instead, provide one entry point for specifying images.
	 * note: since we do not know how \<img\> was allocated, the caller must free
	 *   it themselves (after calling tex_free, which is required regardless of
	 *   alloc type).
	 *
	 * we need only add bookkeeping information and "wrap" it in
	 * our Tex struct, hence the name.
	 *
	 * @param w,h Pixel dimensions.
	 * @param bpp Bits per pixel.
	 * @param flags TexFlags.
	 * @param data Img texture data. note: size is calculated from other params.
	 * @param ofs
	 * @return Status
	 **/
	Status wrap(size_t w, size_t h, size_t bpp, size_t flags, const shared_ptr<u8>& data, size_t ofs);
	
	//
	// modify image
	//

	/**
	 * Change \<t\>'s pixel format.
	 *
	 * @param transforms TexFlags that are to be flipped.
	 * @return Status
	 **/
	Status transform(size_t transforms);

	/**
	 * Change \<t\>'s pixel format (2nd version)
	 * (note: this is equivalent to tex_transform(t, t-\>flags^new_flags).
	 *
	 * @param new_flags desired new value of TexFlags.
	 * @return Status
	 **/
	Status transform_to(size_t new_flags);

	//
	// return image information
	//

	/**
	 * rationale: since Tex is a struct, its fields are accessible to callers.
	 * this is more for C compatibility than convenience; the following should
	 * be used instead of direct access to the corresponding fields because
	 * they take care of some dirty work.
	 **/

	/**
	 * return a pointer to the image data (pixels), taking into account any
	 * header(s) that may come before it.
	 *
	 * @return pointer to data returned by mem_get_ptr (holds reference)!
	 **/
	u8* get_data();

	/**
	 * return the ARGB value of the 1x1 mipmap level of the texture.
	 *
	 * @return ARGB value (or 0 if texture does not have mipmaps)
	 **/
	u32 get_average_colour() const;

	/**
	 * return total byte size of the image pixels. (including mipmaps!)
	 * rationale: this is preferable to calculating manually because it's
	 * less error-prone (e.g. confusing bits_per_pixel with bytes).
	 *
	 * @return size [bytes]
	 **/
	size_t img_size() const;

};


/**
 * Set the orientation to which all loaded images will
 * automatically be converted (excepting file formats that don't specify
 * their orientation, i.e. DDS). See "Default Orientation" in docs.
 * @param orientation Either TEX_BOTTOM_UP or TEX_TOP_DOWN.
 **/
extern void tex_set_global_orientation(int orientation);


/**
 * special value for levels_to_skip: the callback will only be called
 * for the base mipmap level (i.e. 100%)
 **/
const int TEX_BASE_LEVEL_ONLY = -1;

/**
 * callback function for each mipmap level.
 *
 * @param level number; 0 for base level (i.e. 100%), or the first one
 * in case some were skipped.
 * @param level_w, level_h pixel dimensions (powers of 2, never 0)
 * @param level_data the level's texels
 * @param level_data_size [bytes]
 * @param cbData passed through from tex_util_foreach_mipmap.
 **/
typedef void (*MipmapCB)(size_t level, size_t level_w, size_t level_h, const u8* RESTRICT level_data, size_t level_data_size, void* RESTRICT cbData);

/**
 * for a series of mipmaps stored from base to highest, call back for
 * each level.
 *
 * @param w,h Pixel dimensions.
 * @param bpp Bits per pixel.
 * @param data Series of mipmaps.
 * @param levels_to_skip Number of levels (counting from base) to skip, or
 *		  TEX_BASE_LEVEL_ONLY to only call back for the base image.
 *		  Rationale: this avoids needing to special case for images with or
 *		  without mipmaps.
 * @param data_padding Minimum pixel dimensions of mipmap levels.
 *		  This is used in S3TC images, where each level is actually stored in
 *		  4x4 blocks. usually 1 to indicate levels are consecutive.
 * @param cb MipmapCB to call.
 * @param cbData Extra data to pass to cb.
 **/
extern void tex_util_foreach_mipmap(size_t w, size_t h, size_t bpp, const u8* data, int levels_to_skip, size_t data_padding, MipmapCB cb, void* RESTRICT cbData);


//
// image writing
//

/**
 * Is the file's extension that of a texture format supported by tex_load?
 *
 * Rationale: tex_load complains if the given file is of an
 * unsupported type. this API allows users to preempt that warning
 * (by checking the filename themselves), and also provides for e.g.
 * enumerating only images in a file picker.
 * an alternative might be a flag to suppress warning about invalid files,
 * but this is open to misuse.
 *
 * @param pathname Only the extension (starting with '.') is used. case-insensitive.
 * @return bool
 **/
extern bool tex_is_known_extension(const VfsPath& pathname);

/**
 * return the minimum header size (i.e. offset to pixel data) of the
 * file format corresponding to the filename.
 *
 * rationale: this can be used to optimize calls to tex_write: when
 * allocating the buffer that will hold the image, allocate this much
 * extra and pass the pointer as base+hdr_size. this allows writing the
 * header directly into the output buffer and makes for zero-copy IO.
 *
 * @param filename Filename; only the extension (that after '.') is used.
 *		  case-insensitive.
 * @return size [bytes] or 0 on error (i.e. no codec found).
 **/
extern size_t tex_hdr_size(const VfsPath& filename);

#endif	 // INCLUDED_TEX
