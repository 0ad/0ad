#ifndef OGL_TEX_H__
#define OGL_TEX_H__


// load and return a handle to the texture given in <fn>.
// supports RAW, BMP, JP2, PNG, TGA, DDS
extern Handle tex_load(const char* const fn, int scope = 0);

extern int tex_bind(Handle ht);
extern int tex_id(Handle ht);

extern int tex_info(Handle ht, int* w, int* h, int *fmt, int *bpp, void** p);

extern int tex_filter;			// GL values; default: GL_LINEAR
extern unsigned int tex_bpp;	// 16 or 32; default: 32

// upload the specified texture to OpenGL. Texture filter and internal format
// may be specified to override the global defaults.
extern int tex_upload(Handle ht, int filter_override = 0, int internal_fmt_override = 0, int format_override = 0);

extern int tex_free(Handle& ht);


#endif	// #ifndef OGL_TEX_H__
