/*
GUI base
by Gustav Larsson
gee@pyro.nu
*/

#include "precompiled.h"
#include "GUI.h"


//--------------------------------------------------------
//  Error definitions
//--------------------------------------------------------
// TODO Gee: (2004-09-01) Keeper? The PS_NAME_ABIGUITY for instance doesn't let the user know which objects.
DEFINE_ERROR(PS_NAME_TAKEN,			"Reference name is taken");
DEFINE_ERROR(PS_OBJECT_FAIL,		"Object provided is null");
DEFINE_ERROR(PS_SETTING_FAIL,		"Setting does not exist");
DEFINE_ERROR(PS_VALUE_INVALID,		"Value provided is syntactically incorrect");
DEFINE_ERROR(PS_NEEDS_PGUI,			"m_pGUI is NULL when needed for a requested operation");
DEFINE_ERROR(PS_NAME_AMBIGUITY,		"Two or more objects are sharing name");
DEFINE_ERROR(PS_NEEDS_NAME,			"An object are trying to fit into a GUI without a name");

DEFINE_ERROR(PS_LEXICAL_FAIL,		"PS_LEXICAL_FAIL");
DEFINE_ERROR(PS_SYNTACTICAL_FAIL,	"PS_SYNTACTICAL_FAIL");
