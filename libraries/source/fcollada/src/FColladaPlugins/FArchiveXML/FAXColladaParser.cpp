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

#include "StdAfx.h"
#include "FAXColladaParser.h"
#include "FUtils/FUDaeEnum.h"
#include "FUtils/FUStringConversion.h"

namespace FUDaeParser
{
	// Returns the first child node with a given id
	xmlNode* FindChildById(xmlNode* parent, const fm::string& id)
	{
		if (parent != NULL && !id.empty())
		{
			const char* localId = id.c_str();
			if (localId[0] == '#') ++localId;
			for (xmlNode* child = parent->children; child != NULL; child = child->next)
			{
				if (child->type == XML_ELEMENT_NODE)
				{
					fm::string nodeId = ReadNodeId(child);
					if (nodeId == localId) return child;
				}
			}
		}
		return NULL;
	}

	// Returns the first child node of a given "ref" property
	xmlNode* FindChildByRef(xmlNode* parent, const char* ref)
	{
		return FindChildByProperty(parent, DAE_REF_ATTRIBUTE, ref);
	}

	// Returns the first child with the given 'sid' value within a given XML hierarchy 
	xmlNode* FindHierarchyChildById(xmlNode* hierarchyRoot, const char* id)
	{
		xmlNode* found = NULL;
		for (xmlNode* child = hierarchyRoot->children; child != NULL && found == NULL; child = child->next)
		{
			if (child->type != XML_ELEMENT_NODE) continue;
			if (ReadNodeId(child) == id) return child;
			found = FindHierarchyChildById(child, id);
		}
		return found;
	}

	// Returns the first child with the given 'sid' value within a given XML hierarchy 
	xmlNode* FindHierarchyChildBySid(xmlNode* hierarchyRoot, const char* sid)
	{
		if (hierarchyRoot == NULL) return NULL;
		if (ReadNodeProperty(hierarchyRoot, DAE_SID_ATTRIBUTE) == sid)
			return hierarchyRoot;

		xmlNode* found = NULL;
		for (xmlNode* child = hierarchyRoot->children; child != NULL && found == NULL; child = child->next)
		{
			if (child->type != XML_ELEMENT_NODE) continue;
			//if (ReadNodeProperty(child, DAE_SID_ATTRIBUTE) == sid) return child;
			found = FindHierarchyChildBySid(child, sid);
		}
		return found;
	}

	// Returns the first technique node with a given profile
	xmlNode* FindTechnique(xmlNode* parent, const char* profile)
	{
		if (parent != NULL)
		{
			xmlNodeList techniqueNodes;
			FindChildrenByType(parent, DAE_TECHNIQUE_ELEMENT, techniqueNodes);
			size_t techniqueNodeCount = techniqueNodes.size();
			for (size_t i = 0; i < techniqueNodeCount; ++i)
			{
				xmlNode* techniqueNode = techniqueNodes[i];
				fm::string techniqueProfile = ReadNodeProperty(techniqueNode, DAE_PROFILE_ATTRIBUTE);
				if (techniqueProfile == profile) return techniqueNode;
			}
		}
		return NULL;
	}

	// Returns the accessor node for a given source node
	xmlNode* FindTechniqueAccessor(xmlNode* parent)
	{
		xmlNode* techniqueNode = FindChildByType(parent, DAE_TECHNIQUE_COMMON_ELEMENT);
		return FindChildByType(techniqueNode, DAE_ACCESSOR_ELEMENT);
	}

	// Returns a list of parameter names and nodes held by a given XML node
	void FindParameters(xmlNode* parent, StringList& parameterNames, xmlNodeList& parameterNodes)
	{
		if (parent == NULL || parameterNames.size() != parameterNodes.size()) return;

		size_t originalCount = parameterNodes.size();
		for (xmlNode* child = parent->children; child != NULL; child = child->next)
		{
			if (child->type != XML_ELEMENT_NODE) continue;

			// Drop the technique and exta elements that may be found
			if (IsEquivalent(child->name, DAE_TECHNIQUE_ELEMENT) ||
				IsEquivalent(child->name, DAE_EXTRA_ELEMENT)) continue;

			// Buffer this parameter node
			parameterNodes.push_back(child);
		}

		// Retrieve all the parameter's names
		size_t parameterNodeCount = parameterNodes.size();
		parameterNames.resize(parameterNodeCount);
		for (size_t i = originalCount; i < parameterNodeCount; ++i)
		{
			xmlNode* node = parameterNodes[i];
			parameterNames[i] = (const char*) node->name;
		}
	}

	// Retrieves a list of floats from a source node
	// Returns the data's stride.
	uint32 ReadSource(xmlNode* sourceNode, FloatList& array)
	{
		uint32 stride = 0;
		if (sourceNode != NULL)
		{
			// Get the accessor's count
			xmlNode* accessorNode = FindTechniqueAccessor(sourceNode);
			stride = ReadNodeStride(accessorNode);
			array.resize(ReadNodeCount(accessorNode) * stride);

			xmlNode* arrayNode = FindChildByType(sourceNode, DAE_FLOAT_ARRAY_ELEMENT);
			const char* arrayContent = ReadNodeContentDirect(arrayNode);
			FUStringConversion::ToFloatList(arrayContent, array);
		}
		return stride;
	}

	// Retrieves a list of signed integers from a source node
	void ReadSource(xmlNode* sourceNode, Int32List& array)
	{
		if (sourceNode != NULL)
		{
			// Get the accessor's count
			xmlNode* accessorNode = FindTechniqueAccessor(sourceNode);
			array.resize(ReadNodeCount(accessorNode));

			xmlNode* arrayNode = FindChildByType(sourceNode, DAE_FLOAT_ARRAY_ELEMENT);
			const char* arrayContent = ReadNodeContentDirect(arrayNode);
			FUStringConversion::ToInt32List(arrayContent, array);
		}
	}

	// Retrieves a list of strings from a source node
	void ReadSource(xmlNode* sourceNode, StringList& array)
	{
		if (sourceNode != NULL)
		{
			// Get the accessor's count
			xmlNode* accessorNode = FindTechniqueAccessor(sourceNode);
			array.resize(ReadNodeCount(accessorNode));

			xmlNode* arrayNode = FindChildByType(sourceNode, DAE_NAME_ARRAY_ELEMENT);
			if (arrayNode == NULL) arrayNode = FindChildByType(sourceNode, DAE_IDREF_ARRAY_ELEMENT);
			const char* arrayContent = ReadNodeContentDirect(arrayNode);
			FUStringConversion::ToStringList(arrayContent, array);
		}
	}

	// Retrieves a list of points from a source node
	void ReadSource(xmlNode* sourceNode, FMVector3List& array)
	{
		if (sourceNode != NULL)
		{
			// Get the accessor's count
			xmlNode* accessorNode = FindTechniqueAccessor(sourceNode);
			array.resize(ReadNodeCount(accessorNode));

			xmlNode* arrayNode = FindChildByType(sourceNode, DAE_FLOAT_ARRAY_ELEMENT);
			const char* arrayContent = ReadNodeContentDirect(arrayNode);
			FUStringConversion::ToPointList(arrayContent, array);
		}
	}

	// Retrieves a list of matrices from a source node
	void ReadSource(xmlNode* sourceNode, FMMatrix44List& array)
	{
		if (sourceNode != NULL)
		{
			// Get the accessor's count
			xmlNode* accessorNode = FindTechniqueAccessor(sourceNode);
			array.resize(ReadNodeCount(accessorNode));

			xmlNode* arrayNode = FindChildByType(sourceNode, DAE_FLOAT_ARRAY_ELEMENT);
			const char* arrayContent = ReadNodeContentDirect(arrayNode);
			FUStringConversion::ToMatrixList(arrayContent, array);
		}
	}

	// Retrieves a series of interleaved floats from a source node
	void ReadSourceInterleaved(xmlNode* sourceNode, fm::pvector<FloatList>& arrays)
	{
		if (sourceNode != NULL)
		{
			// Get the accessor's count
			xmlNode* accessorNode = FindTechniqueAccessor(sourceNode);
			uint32 count = ReadNodeCount(accessorNode);
			for (fm::pvector<FloatList>::iterator it = arrays.begin(); it != arrays.end(); ++it)
			{
				(*it)->resize(count);
			}

			// Use the stride to pad the interleaved float lists or remove extra elements
			uint32 stride = ReadNodeStride(accessorNode);
			while (stride < arrays.size()) arrays.pop_back();
			while (stride > arrays.size()) arrays.push_back(NULL);

			// Read and parse the float array
   			xmlNode* arrayNode = FindChildByType(sourceNode, DAE_FLOAT_ARRAY_ELEMENT);
			const char* arrayContent = ReadNodeContentDirect(arrayNode);
			FUStringConversion::ToInterleavedFloatList(arrayContent, arrays);
		}
	}

	uint32 ReadSourceInterleaved(xmlNode* sourceNode, fm::pvector<FMVector2List>& arrays)
	{
		uint32 stride = 1;
		if (sourceNode != NULL)
		{
			// Get the accessor's count
			xmlNode* accessorNode = FindTechniqueAccessor(sourceNode);
			uint32 count = ReadNodeCount(accessorNode);
			for (fm::pvector<FMVector2List>::iterator it = arrays.begin(); it != arrays.end(); ++it)
			{
				(*it)->resize(count);
			}

			// Backward Compatibility: if the stride is exactly half the expected value,
			// then we have the old 1D tangents that we need to parse correctly.
			stride = ReadNodeStride(accessorNode);
			if (stride > 0 && stride == arrays.size())
			{
				// Read and parse the float array
				xmlNode* arrayNode = FindChildByType(sourceNode, DAE_FLOAT_ARRAY_ELEMENT);
				const char* value = ReadNodeContentDirect(arrayNode);
				for (size_t i = 0; i < count && *value != 0; ++i)
				{
					for (size_t j = 0; j < stride && *value != 0; ++j)
					{
						arrays[j]->at(i) = FMVector2(FUStringConversion::ToFloat(&value), 0.0f);
					}
				}

				while (*value != 0)
				{
					for (size_t i = 0; i < stride && *value != 0; ++i)
					{
						arrays[i]->push_back(FMVector2(FUStringConversion::ToFloat(&value), 0.0f));
					}
				}
			}
			else
			{
				// Use the stride to pad the interleaved float lists or remove extra elements
				while (stride < arrays.size() * 2) arrays.pop_back();
				while (stride > arrays.size() * 2) arrays.push_back(NULL);

				// Read and parse the float array
				xmlNode* arrayNode = FindChildByType(sourceNode, DAE_FLOAT_ARRAY_ELEMENT);
				const char* value = ReadNodeContentDirect(arrayNode);
				for (size_t i = 0; i < count && *value != 0; ++i)
				{
					for (size_t j = 0; 2 * j < stride && *value != 0; ++j)
					{
						if (arrays[j] != NULL)
						{
							arrays[j]->at(i).u = FUStringConversion::ToFloat(&value);
							arrays[j]->at(i).v = FUStringConversion::ToFloat(&value);
						}
						else
						{
							FUStringConversion::ToFloat(&value);
							FUStringConversion::ToFloat(&value);
						}
					}
				}

				while (*value != 0)
				{
					for (size_t i = 0; 2 * i < stride && *value != 0; ++i)
					{
						if (arrays[i] != NULL)
						{
							FMVector2 v;
							v.u = FUStringConversion::ToFloat(&value);
							v.v = FUStringConversion::ToFloat(&value);
							arrays[i]->push_back(v);
						}
						else
						{
							FUStringConversion::ToFloat(&value);
							FUStringConversion::ToFloat(&value);
						}
					}
				}
			}
		}
		return stride;
	}

	uint32 ReadSourceInterleaved(xmlNode* sourceNode, fm::pvector<FMVector3List>& arrays)
	{
		uint32 stride = 1;
		if (sourceNode != NULL)
		{
			// Get the accessor's count
			xmlNode* accessorNode = FindTechniqueAccessor(sourceNode);
			uint32 count = ReadNodeCount(accessorNode);
			for (fm::pvector<FMVector3List>::iterator it = arrays.begin(); it != arrays.end(); ++it)
			{
				(*it)->resize(count);
			}

			// Backward Compatibility: if the stride is exactly half the expected value,
			// then we have the old 1D tangents that we need to parse correctly.
			stride = ReadNodeStride(accessorNode);
			if (stride > 0 && stride == arrays.size())
			{
				// Read and parse the float array
				xmlNode* arrayNode = FindChildByType(sourceNode, DAE_FLOAT_ARRAY_ELEMENT);
				const char* value = ReadNodeContentDirect(arrayNode);
				for (size_t i = 0; i < count && *value != 0; ++i)
				{
					for (size_t j = 0; j < stride && *value != 0; ++j)
					{
						arrays[j]->at(i) = FMVector3(FUStringConversion::ToFloat(&value), 0.0f, 0.0f);
					}
				}

				while (*value != 0)
				{
					for (size_t i = 0; i < stride && *value != 0; ++i)
					{
						arrays[i]->push_back(FMVector3(FUStringConversion::ToFloat(&value), 0.0f, 0.0f));
					}
				}
			}
			else
			{
				// Use the stride to pad the interleaved float lists or remove extra elements
				while (stride < arrays.size() * 3) arrays.pop_back();
				while (stride > arrays.size() * 3) arrays.push_back(NULL);

				// Read and parse the float array
				xmlNode* arrayNode = FindChildByType(sourceNode, DAE_FLOAT_ARRAY_ELEMENT);
				const char* value = ReadNodeContentDirect(arrayNode);
				for (size_t i = 0; i < count && *value != 0; ++i)
				{
					for (size_t j = 0; 3 * j < stride && *value != 0; ++j)
					{
						if (arrays[j] != NULL)
						{
							arrays[j]->at(i).x = FUStringConversion::ToFloat(&value);
							arrays[j]->at(i).y = FUStringConversion::ToFloat(&value);
							arrays[j]->at(i).z = FUStringConversion::ToFloat(&value);
						}
						else
						{
							FUStringConversion::ToFloat(&value);
							FUStringConversion::ToFloat(&value);
							FUStringConversion::ToFloat(&value);
						}
					}
				}

				while (*value != 0)
				{
					for (size_t i = 0; 2 * i < stride && *value != 0; ++i)
					{
						if (arrays[i] != NULL)
						{
							FMVector3 v;
							v.x = FUStringConversion::ToFloat(&value);
							v.y = FUStringConversion::ToFloat(&value);
							v.z = FUStringConversion::ToFloat(&value);
							arrays[i]->push_back(v);
						}
						else
						{
							FUStringConversion::ToFloat(&value);
							FUStringConversion::ToFloat(&value);
							FUStringConversion::ToFloat(&value);
						}
					}
				}
			}
		}
		return stride;
	}

	// Retrieves a series of interpolation values from a source node
	void ReadSourceInterpolation(xmlNode* sourceNode, UInt32List& array)
	{
		if (sourceNode != NULL)
		{
			// Get the accessor's count
			xmlNode* accessorNode = FindTechniqueAccessor(sourceNode);
			uint32 count = ReadNodeCount(accessorNode);
			array.resize(count);

			// Backward compatibility: drop the unwanted interpolation values.
			// Before, we exported one interpolation token for each dimension of a merged curve.
			// Now, we export one interpolation token for each key of a merged curve.
			uint32 stride = ReadNodeStride(accessorNode);
			StringList stringArray(count * stride);
			xmlNode* arrayNode = FindChildByType(sourceNode, DAE_NAME_ARRAY_ELEMENT);
			const char* arrayContent = ReadNodeContentDirect(arrayNode);
			FUStringConversion::ToStringList(arrayContent, stringArray);
			for (uint32 i = 0; i < count; ++i)
			{
				array[i] = (uint32) FUDaeInterpolation::FromString(stringArray[i * stride]);
			}
		}
	}

	// Retrieves the target property of a targeting node, split into its pointer and its qualifier(s)
	void ReadNodeTargetProperty(xmlNode* targetingNode, fm::string& pointer, fm::string& qualifier)
	{
		fm::string target = ReadNodeProperty(targetingNode, DAE_TARGET_ATTRIBUTE);
		FUStringConversion::SplitTarget(target, pointer, qualifier);
	}

	// Calculate the target pointer for a targetable node
	void CalculateNodeTargetPointer(xmlNode* target, fm::string& pointer)
	{
		if (target != NULL)
		{
			// The target node should have either a subid or an id
			if (HasNodeProperty(target, DAE_ID_ATTRIBUTE))
			{
				pointer = ReadNodeId(target);
				return;
			}
			else if (!HasNodeProperty(target, DAE_SID_ATTRIBUTE))
			{
				pointer.clear();
				return;
			}
	
			// Generate a list of parent nodes up to the first properly identified parent
			xmlNodeList traversal;
			traversal.reserve(16);
			traversal.push_back(target);
			xmlNode* current = target->parent;
			while (current != NULL)
			{
				traversal.push_back(current);
				if (HasNodeProperty(current, DAE_ID_ATTRIBUTE)) break;
				current = current->parent;
			}
	
			// The top parent should have the ID property
			FUSStringBuilder builder;
			intptr_t nodeCount = (intptr_t) traversal.size();
			builder.append(ReadNodeId(traversal[nodeCount - 1]));
			if (builder.empty()) { pointer.clear(); return; }
	
			// Build up the target string
			for (intptr_t i = nodeCount - 2; i >= 0; --i)
			{
				xmlNode* node = traversal[i];
				fm::string subId = ReadNodeProperty(node, DAE_SID_ATTRIBUTE);
				if (!subId.empty())
				{
					builder.append('/');
					builder.append(subId);
				}
			}
	
			pointer = builder.ToString();
		}
		else pointer.clear();
	}

	// Parse the Url attribute off a node
	FUUri ReadNodeUrl(xmlNode* node, const char* attribute)
	{
		fm::string uriString = ReadNodeProperty(node, attribute);
		return FUUri(TO_FSTRING(uriString));
	}

	// Parse the count attribute off a node
	uint32 ReadNodeCount(xmlNode* node)
	{
		fm::string countString = ReadNodeProperty(node, DAE_COUNT_ATTRIBUTE);
		return FUStringConversion::ToUInt32(countString);
	}

	// Parse the stride attribute off a node
	uint32 ReadNodeStride(xmlNode* node)
	{
		fm::string strideString = ReadNodeProperty(node, DAE_STRIDE_ATTRIBUTE);
		uint32 stride = FUStringConversion::ToUInt32(strideString);
		if (stride == 0) stride = 1;
		return stride;
	}

	// Pre-buffer the children of a node, with their ids for performance optimization
	void ReadChildrenIds(xmlNode* node, FAXNodeIdPairList& pairs)
	{
		// To avoid unnecessary memory copies:
		// Start with calculating the maximum child count
		uint32 nodeCount = 0;
		for (xmlNode* child = node->children; child != NULL; child = child->next)
		{
			if (child->type == XML_ELEMENT_NODE) ++nodeCount;
		}

		// Now, buffer the child nodes and their ids
		pairs.reserve(nodeCount);
		for (xmlNode* child = node->children; child != NULL; child = child->next)
		{
			if (child->type == XML_ELEMENT_NODE)
			{
				FAXNodeIdPair* it = pairs.insert(pairs.end(), FAXNodeIdPair());
				it->first = child;
				it->second = ReadNodePropertyCRC(child, DAE_ID_ATTRIBUTE);
			}
		}
	}

	// Skip the pound(#) character from a COLLADA id string
	const char* SkipPound(const fm::string& id)
	{
		const char* s = id.c_str();
		if (s == NULL) return NULL;
		else if (*s == '#') ++s;
		return s;
	}
}
