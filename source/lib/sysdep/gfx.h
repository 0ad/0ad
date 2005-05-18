#ifndef GFX_H__
#define GFX_H__


// useful for choosing a video mode.
// if we fail, outputs are unchanged (assumed initialized to defaults)
extern int get_cur_vmode(int* xres, int* yres, int* bpp, int* freq);

// useful for determining aspect ratio.
// if we fail, outputs are unchanged (assumed initialized to defaults)
extern int get_monitor_size(int& width_mm, int& height_mm);


const size_t GFX_CARD_LEN = 128;
extern char gfx_card[GFX_CARD_LEN];		// default: ""
const size_t GFX_DRV_VER_LEN = 64;
extern char gfx_drv_ver[GFX_DRV_VER_LEN];	// default: ""

// attempt to detect graphics card without OpenGL (in case ogl init fails,
// or we want more detailed info). gfx_card[] is unchanged on failure.
extern void get_gfx_info(void);

#endif	// #ifndef GFX_H__
