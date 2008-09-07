/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/
/*
	Based on the FS Import classes:
	Copyright (C) 2005-2006 Feeling Software Inc
	Copyright (C) 2005-2006 Autodesk Media Entertainment
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FU_DAE_PARSER_
#define _FU_DAE_PARSER_

#ifdef HAS_LIBXML

#ifndef _DAE_SYNTAX_H_
#include "FUtils/FUDaeSyntax.h"
#endif // _DAE_SYNTAX_H_
#ifndef _FU_URI_H_
#include "FUtils/FUUri.h"
#endif // _FU_URI_H_
#ifndef _FU_XML_PARSER_H_
#include "FUtils/FUXmlParser.h"
#endif // _FU_XML_PARSER_H_

typedef fm::pair<xmlNode*, uint32> FAXNodeIdPair;
typedef fm::vector<FAXNodeIdPair> FAXNodeIdPairList;
namespace FUDaeParser
{
	using namespace FUXmlParser;

	// Retrieve specific child nodes
	xmlNode* FindChildById(xmlNode* parent, const fm::string& id);
	xmlNode* FindChildByRef(xmlNode* parent, const char* ref);
	xmlNode* FindHierarchyChildById(xmlNode* hierarchyRoot, const char* id);
	xmlNode* FindHierarchyChildBySid(xmlNode* hierarchyRoot, const char* sid);
	xmlNode* FindTechnique(xmlNode* parent, const char* profile);
	xmlNode* FindTechniqueAccessor(xmlNode* parent);
	void FindParameters(xmlNode* parent, StringList& parameterNames, xmlNodeList& parameterNodes);

	// Import source arrays
	uint32 ReadSource(xmlNode* sourceNode, FloatList& array);
	void ReadSource(xmlNode* sourceNode, Int32List& array);
	void ReadSource(xmlNode* sourceNode, StringList& array);
	void ReadSource(xmlNode* sourceNode, FMVector3List& array);
	void ReadSource(xmlNode* sourceNode, FMMatrix44List& array);
	void ReadSourceInterleaved(xmlNode* sourceNode, fm::pvector<FloatList>& arrays);
	uint32 ReadSourceInterleaved(xmlNode* sourceNode, fm::pvector<FMVector2List>& arrays);
	uint32 ReadSourceInterleaved(xmlNode* sourceNode, fm::pvector<FMVector3List>& arrays);
	void ReadSourceInterpolation(xmlNode* sourceNode, UInt32List& array);

	// Target support
	void ReadNodeTargetProperty(xmlNode* targetingNode, fm::string& pointer, fm::string& qualifier);
	void CalculateNodeTargetPointer(xmlNode* targetedNode, fm::string& pointer);

	// Retrieve common node properties
	inline fm::string ReadNodeId(xmlNode* node) { return ReadNodeProperty(node, DAE_ID_ATTRIBUTE); }
	inline fm::string ReadNodeSid(xmlNode* node) { return ReadNodeProperty(node, DAE_SID_ATTRIBUTE); }
	inline fm::string ReadNodeSemantic(xmlNode* node) { return ReadNodeProperty(node, DAE_SEMANTIC_ATTRIBUTE); }
	inline fm::string ReadNodeName(xmlNode* node) { return ReadNodeProperty(node, DAE_NAME_ATTRIBUTE); }
	inline fm::string ReadNodeSource(xmlNode* node) { return ReadNodeProperty(node, DAE_SOURCE_ATTRIBUTE); }
	inline fm::string ReadNodeStage(xmlNode* node) { return ReadNodeProperty(node, DAE_STAGE_ATTRIBUTE); }
	FUUri ReadNodeUrl(xmlNode* node, const char* attribute=DAE_URL_ATTRIBUTE);
	uint32 ReadNodeCount(xmlNode* node);
	uint32 ReadNodeStride(xmlNode* node);

	// Pre-buffer the children of a node, with their ids, for performance optimization
	void ReadChildrenIds(xmlNode* node, FAXNodeIdPairList& pairs);

	// Skip the pound(#) character from a COLLADA id string
	const char* SkipPound(const fm::string& id);
};

#endif // HAS_LIBXML

#endif // _FU_DAE_PARSER_
