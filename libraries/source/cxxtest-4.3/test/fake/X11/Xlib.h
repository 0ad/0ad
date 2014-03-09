// Fake Xlib.h

struct Display {};
typedef int Window, Colormap, GC;
typedef const char *XID;
struct XFontStruct { int ascent, descent; };
struct Screen {};
struct XColor { int pixel; };

enum { Success, ExposureMask };

inline Display *XOpenDisplay(void *) { return 0; }
inline Colormap DefaultColormap( Display *, int ) { return 0; }
inline void XParseColor( Display *, Colormap, const char *, XColor * ) {}
inline int XAllocColor( Display *, Colormap, XColor *) { return 0; }
inline Window XCreateSimpleWindow( Display *, Window, int, int, int, int, int, int, int ) { return 0; }
inline Window RootWindow( Display *, int ) { return 0; }
inline GC XCreateGC( Display *, Window, int, int ) { return 0; }
inline XID XLoadFont( Display *, const char * ) { return 0; }
inline int XSetFont( Display *, GC, XID ) { return 0; }
inline XID XGContextFromGC( GC ) { return 0; }
inline XFontStruct *XQueryFont( Display *, const char * ) { return 0; }
inline int XFreeFontInfo( char **, XFontStruct *, int ) { return 0; }
inline int XSelectInput( Display *, Window, int ) { return 0; }
inline int XMapWindow( Display *, Window ) { return 0; }
inline Screen *XDefaultScreenOfDisplay( Display * ) { return 0; }
inline int WidthOfScreen( Screen * ) { return 0; }    
inline int HeightOfScreen( Screen * ) { return 0; }
inline int XMoveResizeWindow( Display *, Window, int, int, int, int ) { return 0; }

struct XEvent {};
inline int XCheckMaskEvent( Display *, int, XEvent * ) { return 0; }
inline int XSetStandardProperties( Display *, Window, const char *, int, int, int, int, int ) { return 0; }

struct XWindowAttributes { int width, height; };
inline int XGetWindowAttributes( Display *, Window, XWindowAttributes * ) { return 0; }
inline int XSetForeground( Display *, GC, unsigned long ) { return 0; }
inline int XSetBackground( Display *, GC, unsigned long ) { return 0; }
inline int XFillRectangle( Display *, Window, GC, int, int, int, int ) { return 0; }
inline int XDrawLine( Display *, Window, GC, int, int, int, int ) { return 0; }
inline int XDrawString( Display *, Window, GC, int, int, const char *, int ) { return 0; }
inline int XFlush( Display * ) { return 0; }
inline int XFreeGC( Display *, GC ) { return 0; }
inline int XDestroyWindow( Display *, Window ) { return 0; }
inline int XCloseDisplay( Display * ) { return 0; }
inline int XTextWidth( XFontStruct *, const char *, int ) { return 0; }
