/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FU_URI_H_
#define _FU_URI_H_

/**
	A simple URI structure.
	This structure is quite incomplete but covers all the necessary cases for now.
	Possible upgrades to support all five parts:
	SCHEME://HOSTNAME/FILENAME@ARGUMENTS#DAE_ID

	@ingroup FUtils
*/
class FUUri
{
public:
	/** The URI prefix represent the name of the filename. */
	fstring prefix;
	/** The URI suffix represent the COLLADA id of the element targeted. */
	fm::string suffix;

	/** Constructor. */
	FUUri();

	/** Constructor.
		@param uri The string value for the URI. */
	FUUri(const fm::string& uri);
#ifdef UNICODE
	FUUri(const fstring& uri); /**< See above. */
#endif // UNICODE
};

typedef fm::vector<FUUri> FUUriList; /**< A dynamically-sized array of URIs. */

#endif // _FU_URI_H_

