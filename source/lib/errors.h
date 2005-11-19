// X macros: error code, symbolic name in code, user-visible string.
// error code is usually negative; positive denotes warnings.
//   its absolute value must be within [ERR_MIN, ERR_MAX).
#ifdef ERR

// function arguments
ERR(-100000, ERR_INVALID_PARAM, "Invalid function argument")
ERR(-100001, ERR_INVALID_HANDLE, "Invalid Handle (argument)")
ERR(-100002, ERR_BUF_SIZE, "Buffer argument too small")

// system limitations
ERR(-100020, ERR_NO_MEM, "Not enough memory")
ERR(-100021, ERR_AGAIN, "Try again later")
ERR(-100022, ERR_LIMIT, "Fixed limit exceeded")
ERR(-100023, ERR_NO_SYS, "OS doesn't provide a required API")
ERR(-100024, ERR_NOT_IMPLEMENTED, "Feature currently not implemented")
ERR(-100025, ERR_NOT_SUPPORTED, "Feature isn't and won't be supported")

ERR(-1060, ERR_TIMED_OUT, "Timed out")

// file + vfs
ERR(-100200, ERR_FILE_NOT_FOUND, "VFile not found")
ERR(-100201, ERR_PATH_NOT_FOUND, "VDir not found")
ERR(-100202, ERR_PATH_LENGTH, "Path exceeds VFS_MAX_PATH characters")
ERR(-100203, ERR_PATH_INVALID, "Path is invalid")
ERR(-100210, ERR_DIR_END, "End of directory reached (no more files)")
ERR(-100220, ERR_NOT_FILE, "Not a file")
ERR(-100230, ERR_FILE_ACCESS, "Insufficient access rights to open file")
ERR(-100231, ERR_IO, "Error during IO")
ERR(-100232, ERR_EOF, "Reading beyond end of file")

// file format
ERR(-100400, ERR_UNKNOWN_FORMAT, "Unknown file format")
ERR(-100401, ERR_INCOMPLETE_HEADER, "File header not completely read")
ERR(-100402, ERR_CORRUPTED, "File data is corrupted")

// texture
ERR(-100500, ERR_TEX_FMT_INVALID, "Invalid/unsupported texture format")
ERR(-100501, ERR_TEX_INVALID_COLOR_TYPE, "Invalid color type")
ERR(-100502, ERR_TEX_NOT_8BIT_PRECISION, "Not 8-bit channel precision")
ERR(-100503, ERR_TEX_INVALID_LAYOUT, "Unsupported texel layout, e.g. right-to-left")
ERR(-100504, ERR_TEX_COMPRESSED, "Unsupported texture compression")
ERR(+100505, WARN_TEX_INVALID_DATA, "Warning: invalid texel data encountered")
ERR(-100506, ERR_TEX_INVALID_SIZE, "Texture size is incorrect")

ERR(-100600, ERR_CPU_FEATURE_MISSING, "This CPU doesn't support a required feature")

// shaders
ERR(-100700, ERR_SHDR_CREATE, "Shader creation failed")
ERR(-100701, ERR_SHDR_COMPILE, "Shader compile failed")
ERR(-100702, ERR_SHDR_NO_SHADER, "Invalid shader reference")
ERR(-100703, ERR_SHDR_LINK, "Shader linking failed")
ERR(-100704, ERR_SHDR_NO_PROGRAM, "Invalid shader program reference")

#undef ERR
#endif	// #ifdef ERR


//-----------------------------------------------------------------------------

#ifndef ERRORS_H__
#define ERRORS_H__

#define ERR_MIN 100000
#define ERR_MAX 110000

extern const char* error_description(int err);
extern void error_description_r(int err, char* buf, size_t max_chars);

#endif	// #ifndef ERRORS_H__