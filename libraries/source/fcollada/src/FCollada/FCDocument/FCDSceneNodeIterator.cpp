/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FCDocument/FCDSceneNode.h"
#include "FCDocument/FCDSceneNodeIterator.h"
#ifndef __APPLE__
#include "FCDocument/FCDSceneNodeIterator.hpp"
#endif // __APPLE__

extern void TrickLinker3()
{
	FCDSceneNodeIterator it1(NULL);
	FCDSceneNodeConstIterator it2(NULL);
	
	FCDSceneNodeIterator it3(it1.GetNode());
	it1.Next();
	it2.GetNode();
	it2.Next();
	++it1;
	++it2;
	*it1;
	*it2;
	it1.IsDone();
	it2.IsDone();
}
