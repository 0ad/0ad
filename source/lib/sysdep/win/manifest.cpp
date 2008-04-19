#include "precompiled.h"

/*
To use XP-style themed controls, we need to use the manifest to specify the
desired version. (This must be set in the game's .exe in order to affect Atlas.)

For VC7.1, we use manifest.rc to include a complete manifest file.
For VC8.0, which already generates its own manifest, we use the line below
to add the necessary parts to that generated manifest.

ICC 10.1 IPO considers this string to be an input file, hence this
is currently disabled there.
*/
#if MSC_VERSION >= 1400 && !ICC_VERSION
# if ARCH_IA32
#  pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df'\"")
# elif ARCH_AMD64
#  pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x64' publicKeyToken='6595b64144ccf1df'\"")
# endif
#endif
