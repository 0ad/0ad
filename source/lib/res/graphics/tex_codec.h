#ifndef TEX_CODEC_H__
#define TEX_CODEC_H__

#include "tex.h"

// rationale: no C++ to allow us to store const char* name in vtbl.

struct TexCodecVTbl
{
	// pointers aren't const, because the textures
	// may have to be flipped in-place - see "texture orientation".
	// size is guaranteed to be >= 4.
	// (usually enough to compare the header's "magic" field;
	// anyway, no legitimate file will be smaller)
	int (*decode)(u8* data, size_t data_size, Tex* t, const char** perr_msg);

	// rationale: some codecs cannot calculate the output size beforehand
	// (e.g. PNG output via libpng); we therefore require each one to
	// allocate memory itself and return the pointer.
	int (*encode)(const char* ext, Tex* t, u8** out, size_t* out_size, const char** perr_msg);

	int (*transform)(Tex* t, int new_flags);

	const char* name;
};


#define TEX_CODEC_REGISTER(name)\
	static const TexCodecVTbl vtbl = { name##_decode, name##_encode, name##_transform, #name};\
	static int dummy = tex_codec_register(&vtbl);


// the given texture cannot be handled by this codec; pass the buck on to the next one
const int TEX_CODEC_CANNOT_HANDLE = 1;


// add this vtbl to the codec list. called at NLSO init time by the
// TEX_CODEC_REGISTER in each codec file. note that call order and therefore
// order in the list is undefined, but since each codec only steps up if it
// can handle the given format, this is not a problem.
extern int tex_codec_register(const TexCodecVTbl* c);

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
extern int tex_codec_alloc_rows(const u8* data, size_t h, size_t pitch,
	int file_orientation, RowArray& rows);

extern int tex_codec_set_orientation(Tex* t, int file_orientation);


struct MemSource
{
	u8* p;
	size_t size;
	size_t pos;
};

#endif	// #ifndef TEX_CODEC_H__
