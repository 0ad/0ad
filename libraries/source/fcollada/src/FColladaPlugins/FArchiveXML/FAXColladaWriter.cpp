/*
	Copyright (C) 2005-2007 Feeling Software Inc.
	Portions of the code are:
	Copyright (C) 2005-2007 Sony Computer Entertainment America
	
	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#include "StdAfx.h"
#include "FAXColladaParser.h"
#include "FAXColladaWriter.h"
using namespace FUDaeParser;

#define FLOAT_STR_ESTIMATE 12

namespace FUDaeWriter
{
	// Write out the <extra><technique> element unto the given parent XML tree node.
	// Check for only one <extra> element for this profile.
	xmlNode* AddExtraTechniqueChild(xmlNode* parent, const char* profile)
	{
		xmlNode* techniqueNode = NULL;
		if (parent != NULL)
		{
			xmlNode* extraNode = AddChildOnce(parent, DAE_EXTRA_ELEMENT);
			techniqueNode = AddTechniqueChild(extraNode, profile);
		}
		return techniqueNode;
	}

	// Write out the <technique> element unto the given parent XML tree node.
	// Check for only one <technique> element for this profile.
	xmlNode* AddTechniqueChild(xmlNode* parent, const char* profile)
	{
		xmlNode* techniqueNode = NULL;
		if (parent != NULL)
		{
			techniqueNode = FindTechnique(parent, profile);
			if (techniqueNode == NULL)
			{
				techniqueNode = AddChild(parent, DAE_TECHNIQUE_ELEMENT);
				AddAttribute(techniqueNode, DAE_PROFILE_ATTRIBUTE, profile);
			}
		}
		return techniqueNode;
	}

	xmlNode* AddParameter(xmlNode* parent, const char* name, const char* type)
	{
		xmlNode* parameterNode = AddChild(parent, DAE_PARAMETER_ELEMENT);
		if (name != NULL && *name != 0) AddAttribute(parameterNode, DAE_NAME_ATTRIBUTE, name);
		if (type == NULL) type = DAE_FLOAT_TYPE;
		AddAttribute(parameterNode, DAE_TYPE_ATTRIBUTE, type);
		return parameterNode;
	}

	xmlNode* AddInput(xmlNode* parent, const char* sourceId, const char* semantic, int32 offset, int32 set)
	{
		if (sourceId == NULL || *sourceId == 0 || semantic == NULL || *semantic == 0) return NULL;
		xmlNode* inputNode = AddChild(parent, DAE_INPUT_ELEMENT);
		AddAttribute(inputNode, DAE_SEMANTIC_ATTRIBUTE, semantic);
		AddAttribute(inputNode, DAE_SOURCE_ATTRIBUTE, fm::string("#") + sourceId);
		if (offset >= 0) AddAttribute(inputNode, DAE_OFFSET_ATTRIBUTE, offset);
		if (set >= 0) AddAttribute(inputNode, DAE_SET_ATTRIBUTE, set);
		return inputNode;
	}

	xmlNode* AddArray(xmlNode* parent, const char* id, const char* arrayType, const char* content, size_t count)
	{
		xmlNode* arrayNode = AddChild(parent, arrayType);
		AddContentUnprocessed(arrayNode, content);
		AddAttribute(arrayNode, DAE_ID_ATTRIBUTE, id);
		AddAttribute(arrayNode, DAE_COUNT_ATTRIBUTE, count);
		return arrayNode;
	}

	xmlNode* AddArray(xmlNode* parent, const char* id, const FMVector2List& values)
	{
		// Reserve the necessary space within the string builder
		FUSStringBuilder builder;
		size_t valueCount = values.size();
		builder.reserve(valueCount * 2 * FLOAT_STR_ESTIMATE);
		if (valueCount > 0)
		{
			// Write out the values
			FMVector2List::const_iterator itP = values.begin();
			FUStringConversion::ToString(builder, *itP);
			for (++itP; itP != values.end(); ++itP) { builder.append(' '); FUStringConversion::ToString(builder, *itP); }
		}

		// Create the typed array node.
		return AddArray(parent, id, DAE_FLOAT_ARRAY_ELEMENT, builder.ToCharPtr(), valueCount * 2);
	}

	xmlNode* AddArray(xmlNode* parent, const char* id, const FMVector3List& values)
	{
		// Reserve the necessary space within the string builder
		FUSStringBuilder builder;
		size_t valueCount = values.size();
		builder.reserve(valueCount * 3 * FLOAT_STR_ESTIMATE);
		if (valueCount > 0)
		{
			// Write out the values
			FMVector3List::const_iterator itP = values.begin();
			FUStringConversion::ToString(builder, *itP);
			for (++itP; itP != values.end(); ++itP) { builder.append(' '); FUStringConversion::ToString(builder, *itP); }
		}

		// Create the typed array node.
		return AddArray(parent, id, DAE_FLOAT_ARRAY_ELEMENT, builder.ToCharPtr(), valueCount * 3);
	}

	xmlNode* AddArray(xmlNode* parent, const char* id, const FMVector4List& values)
	{
		// Reserve the necessary space within the string builder
		FUSStringBuilder builder;
		size_t valueCount = values.size();
		builder.reserve(valueCount * 4 * FLOAT_STR_ESTIMATE);
		if (valueCount > 0)
		{
			// Write out the values
			FMVector4List::const_iterator itP = values.begin();
			FUStringConversion::ToString(builder, *itP);
			for (++itP; itP != values.end(); ++itP) { builder.append(' '); FUStringConversion::ToString(builder, *itP); }
		}

		// Create the typed array node.
		return AddArray(parent, id, DAE_FLOAT_ARRAY_ELEMENT, builder.ToCharPtr(), valueCount * 4);
	}

	xmlNode* AddArray(xmlNode* parent, const char* id, const FMMatrix44List& values)
	{
		FUSStringBuilder builder;
		size_t valueCount = values.size();
		builder.reserve(valueCount * 16 * FLOAT_STR_ESTIMATE);
		if (valueCount > 0)
		{
			FMMatrix44List::const_iterator itM = values.begin();
			FUStringConversion::ToString(builder, *itM);
			for (++itM; itM != values.end(); ++itM) { builder.append(' '); FUStringConversion::ToString(builder, *itM); }
		}
		return AddArray(parent, id, DAE_FLOAT_ARRAY_ELEMENT, builder.ToCharPtr(), valueCount * 16);
	}

	xmlNode* AddArray(xmlNode* parent, const char* id, const FloatList& values)
	{
		size_t valueCount = values.size();
		FUSStringBuilder builder;
		builder.reserve(valueCount * FLOAT_STR_ESTIMATE);
		FUStringConversion::ToString(builder, values);
		return AddArray(parent, id, DAE_FLOAT_ARRAY_ELEMENT, builder.ToCharPtr(), valueCount);
	}

	xmlNode* AddArray(xmlNode* parent, const char* id, const StringList& values, const char* arrayType)
	{
		size_t valueCount = values.size();
		FUSStringBuilder builder;
		builder.reserve(valueCount * 18); // Pulled out of a hat
		if (valueCount > 0)
		{
			StringList::const_iterator itV = values.begin();
			builder.set(*itV);
			for (++itV; itV != values.end(); ++itV) { builder.append(' '); builder.append(*itV); }
		}
		return AddArray(parent, id, arrayType, builder.ToCharPtr(), valueCount);
	}

	xmlNode* AddAccessor(xmlNode* parent, const char* arrayId, size_t count, size_t stride, const char** parameters, const char* type)
	{
		// Create the accessor element and fill its basic properties
		xmlNode* accessorNode = AddChild(parent, DAE_ACCESSOR_ELEMENT);
		AddAttribute(accessorNode, DAE_SOURCE_ATTRIBUTE, fm::string("#") + arrayId);
		AddAttribute(accessorNode, DAE_COUNT_ATTRIBUTE, count);
		AddAttribute(accessorNode, DAE_STRIDE_ATTRIBUTE, stride);

		// Create the stride parameters
		if (type == NULL) type = DAE_FLOAT_TYPE;
		if (stride != 16 && stride != 32)
		{
			size_t p = 0;
			for (size_t i = 0; i < stride; ++i)
			{
				const char* parameter = NULL;
				if (parameters != NULL)
				{
					parameter = parameters[p++];
					if (parameter == NULL) { parameter = parameters[0]; p = 1; }
					while (*parameter != 0 && !((*parameter >= 'a' && *parameter <= 'z') || (*parameter >= 'A' && *parameter <= 'Z'))) ++parameter;			
				}
				AddParameter(accessorNode, parameter, type);
			}
		}
		else if (stride == 16)
		{
			const char* parameter = "TRANSFORM";
			AddParameter(accessorNode, parameter, DAE_MATRIX_TYPE);
		}
		else if (stride == 32)
		{
			const char* parameter = "X_Y";
			AddParameter(accessorNode, parameter, DAE_MATRIX_TYPE);
		}
		return accessorNode;
	}

	xmlNode* AddSourceMatrix(xmlNode* parent, const char* id, const FMMatrix44List& values)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 16, NULL, DAE_MATRIX_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourceColor(xmlNode* parent, const char* id, const FMVector3List& values)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 3, FUDaeAccessor::RGBA, DAE_FLOAT_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourceTexcoord(xmlNode* parent, const char* id, const FMVector3List& values)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 3, FUDaeAccessor::STPQ, DAE_FLOAT_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourcePosition(xmlNode* parent, const char* id, const FMVector2List& values)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 2, FUDaeAccessor::XYZW, DAE_FLOAT_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourcePosition(xmlNode* parent, const char* id, const FMVector3List& values)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 3, FUDaeAccessor::XYZW, DAE_FLOAT_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourcePosition(xmlNode* parent, const char* id, const FMVector4List& values)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 4, FUDaeAccessor::XYZW, DAE_FLOAT_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourceFloat(xmlNode* parent, const char* id, const FloatList& values, const char* parameter)
	{ return AddSourceFloat(parent, id, values, 1, &parameter); }
	xmlNode* AddSourceFloat(xmlNode* parent, const char* id, const FloatList& values, size_t stride, const char** parameters)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		if (stride == 0) stride = 1;
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size() / stride, stride, parameters, (stride != 16) ? DAE_FLOAT_TYPE : DAE_MATRIX_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourceFloat(xmlNode* parent, const char* id, const FMVector3List& values)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 3, NULL, DAE_FLOAT_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourceFloat(xmlNode* parent, const char* id, const FMVector2List& values)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 2, NULL, DAE_FLOAT_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourceTangent(xmlNode* parent, const char* id, const FMVector2List& values)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 2, FUDaeAccessor::XY, DAE_FLOAT_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourceString(xmlNode* parent, const char* id, const StringList& values, const char* parameter)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values, DAE_NAME_ARRAY_ELEMENT);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 1, &parameter, DAE_NAME_TYPE);
		return sourceNode;
	}
	
	xmlNode* AddSourceIDRef(xmlNode* parent, const char* id, const StringList& values, const char* parameter)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");
		AddArray(sourceNode, arrayId.ToCharPtr(), values, DAE_IDREF_ARRAY_ELEMENT);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), values.size(), 1, &parameter, DAE_IDREF_TYPE);
		return sourceNode;
	}

	xmlNode* AddSourceInterpolation(xmlNode* parent, const char* id, const FUDaeInterpolationList& interpolations)
	{
		xmlNode* sourceNode = AddChild(parent, DAE_SOURCE_ELEMENT);
		AddAttribute(sourceNode, DAE_ID_ATTRIBUTE, id);
		FUSStringBuilder arrayId(id); arrayId.append("-array");

		FUSStringBuilder builder;
		size_t valueCount = interpolations.size();
		if (valueCount > 0)
		{
			FUDaeInterpolationList::const_iterator itI = interpolations.begin();
			builder.append(FUDaeInterpolation::ToString(*itI));
			for (++itI; itI != interpolations.end(); ++itI)
			{
				builder.append(' '); builder.append(FUDaeInterpolation::ToString(*itI));
			}
		}
		AddArray(sourceNode, arrayId.ToCharPtr(), DAE_NAME_ARRAY_ELEMENT, builder.ToCharPtr(), valueCount);
		xmlNode* techniqueCommonNode = AddChild(sourceNode, DAE_TECHNIQUE_COMMON_ELEMENT);
		const char* parameter = "INTERPOLATION";
		AddAccessor(techniqueCommonNode, arrayId.ToCharPtr(), valueCount, 1, &parameter, DAE_NAME_TYPE);
		return sourceNode;
	}

	// Add an 'sid' attribute to the given XML node, ensuring unicity. Returns the final 'sid' value.
	fm::string AddNodeSid(xmlNode* node, const char* wantedSid)
	{
		// Find the first parent node with an id or sid. If this node has an id, return right away.
		xmlNode* parentNode = node;
		for (parentNode = node; parentNode != NULL; parentNode = parentNode->parent)
		{
			if (HasNodeProperty(parentNode, DAE_ID_ATTRIBUTE) || HasNodeProperty(parentNode, DAE_SID_ATTRIBUTE)) break;
		}
		if (parentNode == node)
		{
			if (!HasNodeProperty(parentNode, DAE_SID_ATTRIBUTE)) AddAttribute(node, DAE_SID_ATTRIBUTE, wantedSid);
			return wantedSid;
		}
		if (parentNode == NULL)
		{
			// Retrieve the last parent node available
			for (parentNode = node; parentNode->parent != NULL; parentNode = parentNode->parent) {}
		}

		// Check the wanted sid for uniqueness
		xmlNode* existingNode = FindHierarchyChildBySid(parentNode, wantedSid);
		if (existingNode == NULL)
		{
			AddAttribute(node, DAE_SID_ATTRIBUTE, wantedSid);
			return wantedSid;
		}

		// Generate new sids with an incremental counter.
		for (uint32 counter = 2; counter < 100; ++counter)
		{
			FUSStringBuilder builder(wantedSid); builder.append(counter);
			existingNode = FindHierarchyChildBySid(parentNode, builder.ToCharPtr());
			if (existingNode == NULL)
			{
				AddAttribute(node, DAE_SID_ATTRIBUTE, builder);
				return builder.ToString();
			}
		}
		return emptyString;
	}

	void AddNodeSid(xmlNode* node, fm::string& subId)
	{
		subId = AddNodeSid(node, subId.c_str());
	}
#ifdef UNICODE
	void AddNodeSid(xmlNode* node, fstring& subId)
	{
		fm::string _subId = TO_STRING(subId);
		_subId = AddNodeSid(node, _subId.c_str());
		subId = TO_FSTRING(_subId);
	}
#endif
};
