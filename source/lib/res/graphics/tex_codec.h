#ifndef TEX_CODEC_H__
#define TEX_CODEC_H__

#include "tex.h"
#include "dyn_array.h"

// rationale: no C++ to allow us to store const char* name in vtbl.

struct TexCodecVTbl
{
	// 'template method' to increase code reuse and simplify writing new codecs

	// pointers aren't const, because the textures
	// may have to be flipped in-place - see "texture orientation".
	// size is guaranteed to be >= 4.
	// (usually enough to compare the header's "magic" field;
	// anyway, no legitimate file will be smaller)
	int (*decode)(DynArray* restrict da, Tex* restrict t);

	// rationale: some codecs cannot calculate the output size beforehand
	// (e.g. PNG output via libpng); we therefore require each one to
	// allocate memory itself and return the pointer.
	//
	// note: <t> cannot be made const because encoding may require a
	// tex_transform.
	int (*encode)(Tex* restrict t, DynArray* restrict da);

	int (*transform)(Tex* t, uint transforms);

	// only guaranteed 4 bytes!
	bool (*is_hdr)(const u8* file);

	// precondition: ext is valid string
	// ext doesn't include '.'; just compare against e.g. "png"
	bool (*is_ext)(const char* ext);

	size_t (*hdr_size)(const u8* file);

	const char* name;
};


#define TEX_CODEC_REGISTER(name)\
	static const TexCodecVTbl vtbl = { name##_decode, name##_encode, name##_transform, name##_is_hdr, name##_is_ext, name##_hdr_size, #name};\
	static int dummy = tex_codec_register(&vtbl);


// the given texture cannot be handled by this codec; pass the buck on to the next one
const int TEX_CODEC_CANNOT_HANDLE = 1;


// add this vtbl to the codec list. called at NLSO init time by the
// TEX_CODEC_REGISTER in each codec file. note that call order and therefore
// order in the list is undefined, but since each codec only steps up if it
// can handle the given format, this is not a problem.
extern int tex_codec_register(const TexCodecVTbl* c);


// find codec that recognizes the desired output file extension
extern int tex_codec_for_filename(const char* fn, const TexCodecVTbl** c);

// find codec that recognizes the header's magic field
extern int tex_codec_for_header(const u8* file, size_t file_size, const TexCodecVTbl** c);



// allocate an array of row pointers that point into the given texture data.
// <file_orientation> indicates whether the file format is top-down or
// bottom-up; the row array is inverted if necessary to match global
// orienatation. (this is more efficient than "transforming" later)
//
// used by PNG and JPG codecs; caller must free() rows when done.
//
// note: we don't allocate the data param ourselves because this function is
// needed for encoding, too (where data is already present).
typedef const u8* RowPtr;
typedef RowPtr* RowArray;
extern int tex_util_alloc_rows(const u8* data, size_t h, size_t pitch,
	uint src_flags, uint dst_orientation, RowArray& rows);

extern int tex_util_write(Tex* t, uint transforms, const void* hdr, size_t hdr_size, DynArray* da);

#endif	// #ifndef TEX_CODEC_H__
