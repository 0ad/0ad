/* Copyright (c) 2010 Wildfire Games
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
 * support routines and interface for texture codecs.
 */

#ifndef INCLUDED_TEX_CODEC
#define INCLUDED_TEX_CODEC

#include "tex.h"
#include "tex_internal.h"	// for codec's convenience

/**
 * virtual method table for TexCodecs.
 * rationale: this works in C and also allows storing name and next in vtbl.
 * 'template method'-style interface to increase code reuse and
 * simplify writing new codecs.
 **/
class ITexCodec
{
public:
	/**
	 * decode the file into a Tex structure.
	 *
	 * @param data input data array (non-const, because the texture
	 * may have to be flipped in-place - see "texture orientation").
	 * @param size [bytes] of data, always >= 4
	 *   (this is usually enough to compare the header's "magic" field,
	 *    and no legitimate file will be smaller)
	 * @param t output texture object
	 * @return Status
	 **/
	virtual Status decode(u8* data, size_t size, Tex* RESTRICT t) const = 0;

	/**
	 * encode the texture data into the codec's file format (in memory).
	 *
	 * @param t input texture object. note: non-const because encoding may
	 * require a Tex::transform.
	 * @param da output data array, allocated by codec.
	 * rationale: some codecs cannot calculate the output size beforehand
	 * (e.g. PNG output via libpng), so the output memory cannot be allocated
	 * by the caller.
	 * @return Status
	 **/
	virtual Status encode(Tex* RESTRICT t, DynArray* RESTRICT da) const = 0;

	/**
	 * transform the texture's pixel format.
	 *
	 * @param t texture object
	 * @param transforms: OR-ed combination of TEX_* flags that are to
	 * be changed. note: the codec needs only handle situations specific
	 * to its format; generic pixel format transforms are handled by
	 * the caller.
	 **/
	virtual Status transform(Tex* t, size_t transforms) const = 0;

	/**
	 * indicate if the data appears to be an instance of this codec's header,
	 * i.e. can this codec decode it?
	 *
	 * @param file input data; only guaranteed to be 4 bytes!
	 * (this should be enough to examine the header's 'magic' field)
	 * @return bool
	 **/
	virtual bool is_hdr(const u8* file) const = 0;

	/**
	 * is the extension that of a file format supported by this codec?
	 *
	 * rationale: cannot just return the extension string and have
	 * caller compare it (-> smaller code) because a codec's file format
	 * may have several valid extensions (e.g. jpg and jpeg).
	 *
	 * @param extension (including '.')
	 * @return bool
	 **/
	virtual bool is_ext(const OsPath& extension) const = 0;

	/**
	 * return size of the file header supported by this codec.
	 *
	 * @param file the specific header to return length of (taking its
	 * variable-length fields into account). if NULL, return minimum
	 * guaranteed header size, i.e. the header without any
	 * variable-length fields.
	 * @return size [bytes]
	 **/
	virtual size_t hdr_size(const u8* file) const = 0;

	/**
	 * name of codec for debug purposes. typically set via TEX_CODEC_REGISTER.
	 **/
	virtual const wchar_t* get_name() const = 0;

	virtual ~ITexCodec() {}
};

class TexCodecPng:ITexCodec {
public:
	virtual Status decode(u8* data, size_t size, Tex* RESTRICT t) const;
	virtual Status encode(Tex* RESTRICT t, DynArray* RESTRICT da) const;
	virtual Status transform(Tex* t, size_t transforms) const;
	virtual bool is_hdr(const u8* file) const;
	virtual bool is_ext(const OsPath& extension) const;
	virtual size_t hdr_size(const u8* file) const;
	virtual const wchar_t* get_name() const {
		static const wchar_t *name = L"png";
		return name;
	};
};

class TexCodecJpg:ITexCodec {
public:
	virtual Status decode(u8* data, size_t size, Tex* RESTRICT t) const;
	virtual Status encode(Tex* RESTRICT t, DynArray* RESTRICT da) const;
	virtual Status transform(Tex* t, size_t transforms) const;
	virtual bool is_hdr(const u8* file) const;
	virtual bool is_ext(const OsPath& extension) const;
	virtual size_t hdr_size(const u8* file) const;
	virtual const wchar_t* get_name() const {
		static const wchar_t *name = L"jpg";
		return name;
	};
};

class TexCodecDds:ITexCodec {
public:
	virtual Status decode(u8* data, size_t size, Tex* RESTRICT t) const;
	virtual Status encode(Tex* RESTRICT t, DynArray* RESTRICT da) const;
	virtual Status transform(Tex* t, size_t transforms) const;
	virtual bool is_hdr(const u8* file) const;
	virtual bool is_ext(const OsPath& extension) const;
	virtual size_t hdr_size(const u8* file) const;
	virtual const wchar_t* get_name() const {
		static const wchar_t *name = L"dds";
		return name;
	};
};

class TexCodecTga:ITexCodec {
public:
	virtual Status decode(u8* data, size_t size, Tex* RESTRICT t) const;
	virtual Status encode(Tex* RESTRICT t, DynArray* RESTRICT da) const;
	virtual Status transform(Tex* t, size_t transforms) const;
	virtual bool is_hdr(const u8* file) const;
	virtual bool is_ext(const OsPath& extension) const;
	virtual size_t hdr_size(const u8* file) const;
	virtual const wchar_t* get_name() const {
		static const wchar_t *name = L"tga";
		return name;
	};
};

class TexCodecBmp:ITexCodec {
public:
	virtual Status decode(u8* data, size_t size, Tex* RESTRICT t) const;
	virtual Status encode(Tex* RESTRICT t, DynArray* RESTRICT da) const;
	virtual Status transform(Tex* t, size_t transforms) const;
	virtual bool is_hdr(const u8* file) const;
	virtual bool is_ext(const OsPath& extension) const;
	virtual size_t hdr_size(const u8* file) const;
	virtual const wchar_t* get_name() const {
		static const wchar_t *name = L"bmp";
		return name;
	};
};

/**
 * Find codec that recognizes the desired output file extension.
 *
 * @param extension
 * @param c (out) vtbl of responsible codec
 * @return Status; ERR::RES_UNKNOWN_FORMAT (without warning, because this is
 * called by tex_is_known_extension) if no codec indicates they can
 * handle the given extension.
 **/
extern Status tex_codec_for_filename(const OsPath& extension, const ITexCodec** c);

/**
 * find codec that recognizes the header's magic field.
 *
 * @param data typically contents of file, but need only include the
 * (first 4 bytes of) header.
 * @param data_size [bytes]
 * @param c (out) vtbl of responsible codec
 * @return Status; ERR::RES_UNKNOWN_FORMAT if no codec indicates they can
 * handle the given format (header).
 **/
extern Status tex_codec_for_header(const u8* data, size_t data_size, const ITexCodec** c);

/**
 * transform the texture's pixel format.
 * tries each codec's transform method once, or until one indicates success.
 *
 * @param t texture object
 * @param transforms: OR-ed combination of TEX_* flags that are to
 * be changed.
 * @return Status
 **/
extern Status tex_codec_transform(Tex* t, size_t transforms);

/**
 * allocate an array of row pointers that point into the given texture data.
 * for texture decoders that support output via row pointers (e.g. PNG),
 * this allows flipping the image vertically (useful when matching bottom-up
 * textures to a global orientation) directly, which is much more
 * efficient than transforming later via copying all pixels.
 *
 * @param data the texture data into which row pointers will point.
 * note: we don't allocate it here because this function is
 * needed for encoding, too (where data is already present).
 * @param h height [pixels] of texture.
 * @param pitch size [bytes] of one texture row, i.e. width*bytes_per_pixel.
 * @param src_flags TexFlags of source texture. used to extract its
 * orientation.
 * @param dst_orientation desired orientation of the output data.
 * can be one of TEX_BOTTOM_UP, TEX_TOP_DOWN, or 0 for the
 * "global orientation".
 * depending on src and dst, the row array is flipped if necessary.
 **/
typedef const u8* RowPtr;
extern std::vector<RowPtr> tex_codec_alloc_rows(const u8* data, size_t h, size_t pitch, size_t src_flags, size_t dst_orientation);

/**
 * apply transforms and then copy header and image into output buffer.
 *
 * @param t input texture object
 * @param transforms transformations to be applied to pixel format
 * @param hdr header data
 * @param hdr_size [bytes]
 * @param da output data array (will be expanded as necessary)
 * @return Status
 **/
extern Status tex_codec_write(Tex* t, size_t transforms, const void* hdr, size_t hdr_size, DynArray* da);

#endif	 // #ifndef INCLUDED_TEX_CODEC
