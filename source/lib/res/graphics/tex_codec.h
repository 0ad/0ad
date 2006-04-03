#ifndef TEX_CODEC_H__
#define TEX_CODEC_H__

#include "tex.h"
#include "lib/allocators.h"

// rationale: no C++ to allow us to store const char* name in vtbl.

struct TexCodecVTbl
{
	// 'template method' to increase code reuse and simplify writing new codecs

	// pointers aren't const, because the textures
	// may have to be flipped in-place - see "texture orientation".
	// size is guaranteed to be >= 4.
	// (usually enough to compare the header's "magic" field;
	// anyway, no legitimate file will be smaller)
	LibError (*decode)(DynArray* restrict da, Tex* restrict t);

	// rationale: some codecs cannot calculate the output size beforehand
	// (e.g. PNG output via libpng); we therefore require each one to
	// allocate memory itself and return the pointer.
	//
	// note: <t> cannot be made const because encoding may require a
	// tex_transform.
	LibError (*encode)(Tex* restrict t, DynArray* restrict da);

	LibError (*transform)(Tex* t, uint transforms);

	// only guaranteed 4 bytes!
	bool (*is_hdr)(const u8* file);

	// precondition: ext is valid string
	// ext doesn't include '.'; just compare against e.g. "png"
	// must compare case-insensitive!
	bool (*is_ext)(const char* ext);

	size_t (*hdr_size)(const u8* file);

	const char* name;

	// intrusive linked-list of codecs: more convenient than fixed-size
	// static storage.
	const TexCodecVTbl* next;
};


#define TEX_CODEC_REGISTER(name)\
	static TexCodecVTbl vtbl = { name##_decode, name##_encode, name##_transform, name##_is_hdr, name##_is_ext, name##_hdr_size, #name};\
	static int dummy = tex_codec_register(&vtbl);


// add this vtbl to the codec list. called at NLSO init time by the
// TEX_CODEC_REGISTER in each codec file. note that call order and therefore
// order in the list is undefined, but since each codec only steps up if it
// can handle the given format, this is not a problem.
//
// returns int to alloc calling from a macro at file scope.
extern int tex_codec_register(TexCodecVTbl* c);


// find codec that recognizes the desired output file extension,
// or return ERR_UNKNOWN_FORMAT if unknown.
// note: does not raise a warning because it is used by
// tex_is_known_extension.
extern LibError tex_codec_for_filename(const char* fn, const TexCodecVTbl** c);

// find codec that recognizes the header's magic field
extern LibError tex_codec_for_header(const u8* file, size_t file_size, const TexCodecVTbl** c);

extern LibError tex_codec_transform(Tex* t, uint transforms);


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
extern LibError tex_codec_alloc_rows(const u8* data, size_t h, size_t pitch,
	uint src_flags, uint dst_orientation, RowArray& rows);

extern LibError tex_codec_write(Tex* t, uint transforms, const void* hdr, size_t hdr_size, DynArray* da);

#endif	// #ifndef TEX_CODEC_H__
