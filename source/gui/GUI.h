/*
GUI Inclusion file
by Gustav Larsson
gee@pyro.nu

--Overview--

	Include this file and it will include the whole GUI

--More info--

	http://gee.pyro.nu/GUI/

*/

// Main page for GUI documentation
/**
 * @mainpage
 * Welcome to the Wildfire Games Graphical User Interface Documentation.
 *
 * Additional Downloads can be made from the link below.\n
 * <a href="http://gee.pyro.nu/GUIfiles/">Technical Design Document</a>
 *
 * The GUI uses <a href="http://xml.apache.org/xerces-c/">Xerces C++ Parser</a>, 
 * Current official version (ensured to work): 2.3.0
 * 
 * @dot
 * digraph
 * {
 *	node [shape=record, fontname=Helvetica, fontsize=10];
 *	q [ label="Questions?"];
 *  c [ label="Comments?"];
 *  s [ label="Suggestions?"];
 *	email [label="E-mail Me" URL="mailto:slimgee@bredband.net"];
 *	q -> email;
 *	c -> email;
 *	s -> email;
 * }
 * @enddot
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

#include "Prometheus.h"
#include "CStr.h"
#include "types.h"
#include "ogl.h"

#include "GUIbase.h"
#include "GUIutil.h"
#include "IGUIObject.h"
#include "IGUISettingsObject.h"
#include "IGUIButtonBehavior.h"
#include "CButton.h"
#include "CGUISprite.h"
#include "CGUI.h"


#endif
