#include "stdafx.h"

#include "AtlasClipboard.h"

// TODO: Do this properly, using the native clipboard. (That probably
// requires AtObj to be serialisable, though... Maybe just use XML?)

static AtObj g_Clipboard;

bool AtlasClipboard::SetClipboard(AtObj& in)
{
	g_Clipboard = in;
	return true;
}

bool AtlasClipboard::GetClipboard(AtObj& out)
{
	out = g_Clipboard;
	return true;
}
