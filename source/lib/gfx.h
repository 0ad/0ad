
// useful for choosing a video mode. not called by detect().
// if we fail, outputs are unchanged (assumed initialized to defaults)
extern int get_cur_vmode(int* xres, int* yres, int* bpp, int* freq);

// useful for determining aspect ratio. not called by detect().
// if we fail, outputs are unchanged (assumed initialized to defaults)
extern int get_monitor_size(int& width_mm, int& height_mm);



extern char gfx_card[64];		// default: ""
extern char gfx_drv_ver[64];	// default: ""

// attempt to detect graphics card without OpenGL (in case ogl init fails,
// or we want more detailed info). gfx_card[] is unchanged on failure.
extern void get_gfx_info();