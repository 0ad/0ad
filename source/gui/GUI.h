/*
GUI Inclusion file
by Gustav Larsson
gee@pyro.nu

--Overview--

	Include this file and it will include the whole GUI.

--More info--

	Check TDD for GUI Engine Documentation

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
#include <vector>
#include <stddef.h>

#include "Pyrogenesis.h"
#include "CStr.h"
#include "types.h"
#include "ogl.h"

#include "GUIbase.h"
#include "GUIutil.h"
#include "GUItext.h"
#include "IGUIObject.h"
#include "IGUIButtonBehavior.h"
#include "IGUIScrollBarOwner.h"
#include "IGUITextOwner.h"
#include "IGUIScrollBar.h"
#include "CGUIScrollBarVertical.h"
#include "CGUISprite.h"
#include "CGUI.h"

#endif
