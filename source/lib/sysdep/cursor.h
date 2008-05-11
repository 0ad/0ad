
// note: these do not warn on error; that is left to the caller.

// creates a cursor from the given image.
// w, h specify image dimensions [pixels]. limit is implementation-
//   dependent; 32x32 is typical and safe.
// bgra_img is the cursor image (BGRA format, bottom-up).
//   it is no longer needed and can be freed after this call returns.
// hotspot (hx,hy) is the offset from its upper-left corner to the
//   position where mouse clicks are registered.
// cursor is only valid when INFO::OK is returned; in that case, it must be
//   sys_cursor_free-ed when no longer needed.
extern LibError sys_cursor_create(int w, int h, void* bgra_img, int hx, int hy, void** cursor);

// create a fully transparent cursor (i.e. one that when passed to set hides
// the system cursor)
extern LibError sys_cursor_create_empty(void **cursor);

// replaces the current system cursor with the one indicated. need only be
// called once per cursor; pass 0 to restore the default.
extern LibError sys_cursor_set(void* cursor);

// destroys the indicated cursor and frees its resources. if it is
// currently the system cursor, the default cursor is restored first.
extern LibError sys_cursor_free(void* cursor);
