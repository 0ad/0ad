/*
GUI Inclusion file
by Gustav Larsson
gee@pyro.nu

--Overview--

	Include this file and it will include the whole GUI

--More info--

	http://gee.pyro.nu/wfg/GUI/

*/

#ifndef GUI_H
#define GUI_H

//--------------------------------------------------------
//  Compiler specific
//--------------------------------------------------------
// Important that we do this before <map> is included
#ifdef _MSC_VER
# pragma warning(disable:4786)
#endif

//--------------------------------------------------------
//  Includes
//--------------------------------------------------------
#include <map>
#include <string>
#include <stddef.h>

//// janwas: these are very sloppy added, I don't know the etiquette
#include "../ps/Prometheus.h"
#include "../ps/CStr.h"
#include "types.h"
#include "ogl.h"
//// This is what I need from these includes
/*
	- OGL
	- #define DEFINE_ERROR(x, y)  PS_RESULT x=y; 
	- #define DECLARE_ERROR(x)  extern PS_RESULT x; 
	- PS_RESULT
	- u16
*/

#include "GUIbase.h"
#include "GUIutil.h"
#include "CGUIObject.h"
#include "CGUISettingsObject.h"
#include "CGUIButtonBehavior.h"
#include "CButton.h"
#include "CGUISprite.h"
#include "CGUI.h"


#endif