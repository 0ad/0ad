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

#ifndef _FU_XML_NODE_ID_PAIR_H_
#define _FU_XML_NODE_ID_PAIR_H_

class FUXmlNodeIdPair
{
public:
	FUCrc32::crc32 id;
	xmlNode* node;
};

typedef fm::vector<FUXmlNodeIdPair> FUXmlNodeIdPairList;

#endif // _FU_XML_NODE_ID_PAIR_H_

