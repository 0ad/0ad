#ifndef INCLUDED_KEYNAME
#define INCLUDED_KEYNAME

extern void InitKeyNameMap();
extern CStr FindKeyName( int keycode );
extern int FindKeyCode( const CStr& keyname );

#endif	// #ifndef INCLUDED_KEYNAME
