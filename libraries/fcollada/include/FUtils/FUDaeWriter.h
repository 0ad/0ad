/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FUDaeWriter.h
	This file contains the FUDaeWriter namespace.
*/

#ifndef _FU_DAE_WRITER_H_
#define _FU_DAE_WRITER_H_

#ifdef HAS_LIBXML

#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_
#ifndef _DAE_SYNTAX_H_
#include "FUtils/FUDaeSyntax.h"
#endif // _DAE_SYNTAX_H_
#ifndef _FU_URI_H_
#include "FUtils/FUUri.h"
#endif // _FU_URI_H_
#ifndef _FU_XML_WRITER_H_
#include "FUtils/FUXmlWriter.h"
#endif // _FU_XML_WRITER_H_

/**
	Common COLLADA XML writing functions.
	Based on top of the FUXmlWriter namespace and the LibXML2 library.
	This whole namespace is considered external and should only be used
	by the FCollada library.
	
	@ingroup FUtils
*/
namespace FUDaeWriter
{
	using namespace FUXmlWriter;

	/** Common accessor type string arrays.
		These are NULL-terminated and can be used with the AddAccessor function. */
	struct FCOLLADA_EXPORT FUDaeAccessor
	{
		static const char* XY[]; /**< Use for tangents and other 2D values. */
		static const char* XYZW[]; /**< Use for vector and position sources. */
		static const char* RGBA[]; /**< Use for color sources. */
		static const char* STPQ[]; /**< Use for texture coordinate sources. */
	};

	/** Writes out the \<extra\>\<technique\> element unto the given parent XML tree node.
		This function ensures that only one \<extra\> element exists and that only
		one \<technique\> element exists for the given profile.
		@param parent The parent XML tree node.
		@param profile The application-specific profile name.
		@return The \<technique\> XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddExtraTechniqueChild(xmlNode* parent, const char* profile);

	/** Writes out the \<technique\> element unto the given parent XML tree node.
		This function ensures that only one \<technique\> element exists for the given profile.
		@param parent The parent XML tree node.
		@param profile The application-specific profile name.
		@return The \<technique\> XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddTechniqueChild(xmlNode* parent, const char* profile);

	/** Writes out a COLLADA parameter element.
		This is used for the source accessors.
		A COLLADA parameter has the form: \<param name='' type=''\>value\</param\>.
		@param parent The parent XML tree node.
		@param name The name attribute value.
		@param type The type attribute value.
		@return The created \<param\> XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddParameter(xmlNode* parent, const char* name, const char* type);

	/** Writes out a COLLADA input element.
		This is a very common element. For example, it is used in
		the \<polygons\>, \<sampler\> and \<joints\> elements.
		A COLLADA input has the form: \<input source='\#source_id' semantic='' offset='' set=''/\>.
		@param parent The parent XML tree node.
		@param sourceId The source attribute value.
			This is the COLLADA id of a valid \<source\> element.
		@param semantic The semantic attribute value.
			This is a valid COLLADA semantic.
			For example: POSITION, TEXCOORD, WEIGHT, IN_TANGENT.
		@param offset The optional offset attribute value.
			When used in conjunction with the \<v\> or the \<p\> elements, this is
			the offset for the input data indices within the interleaved indices.
		@param set The optional set attribute value.
			This unsigned integer is used to tied together multiple inputs.
		@return The created \<input\> XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddInput(xmlNode* parent, const char* sourceId, const char* semantic, int32 offset=-1, int32 set=-1);
	inline xmlNode* AddInput(xmlNode* parent, const fm::string& sourceId, const char* semantic, int32 offset=-1, int32 set=-1) { return AddInput(parent, sourceId.c_str(), semantic, offset, set); } /**< See above. */

	/** Writes out a COLLADA strongly-typed data array.
		To write out data values, it is preferable to use the AddSourceX functions.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the array.
			This id is used only by the accessor of a source.
		@param arrayType The strongly-typed name of the array.
			For example: \<float_array\>, \<Name_array\>.
		@param content The array content.
		@param count The number of entries within the content of the array.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddArray(xmlNode* parent, const char* id, const char* arrayType, const char* content, size_t count);

	/** Writes out a COLLADA array of matrices.
		To write out data values, it is preferable to use the AddSourceX functions.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the array.
			This id is used only by the accessor of a source.
		@param values A list of matrices.
		@param lengthFactor An optional scale factor for
			the translation column of the matrices.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddArray(xmlNode* parent, const char* id, const FMMatrix44List& values, float lengthFactor=1.0f);

	/** Writes out a COLLADA array of 3D vectors.
		To write out data values, it is preferable to use the AddSourceX functions.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the array.
			This id is used only by the accessor of a source.
		@param values A list of 3D vectors.
		@param lengthFactor An optional scale factor for all the 3D vectors.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddArray(xmlNode* parent, const char* id, const FMVector3List& values, float lengthFactor=1.0f);

	/** Writes out a COLLADA array of 2D vectors.
		To write out data values, it is preferable to use the AddSourceX functions.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the array.
			This id is used only by the accessor of a source.
		@param values A list of 2D vectors.
		@param lengthFactor An optional scale factor for all the 3D vectors.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddArray(xmlNode* parent, const char* id, const FMVector2List& values, float lengthFactor=1.0f);

	/** Writes out a COLLADA array of floating-point values.
		To write out data values, it is preferable to use the AddSourceX functions.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the array.
			This id is used only by the accessor of a source.
		@param values A list of floating-point values.
		@param lengthFactor An optional scale factor for all the floating-point values.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddArray(xmlNode* parent, const char* id, const FloatList& values, float lengthFactor=1.0f);

	/** Writes out a COLLADA array of UTF-8 tokens.
		To write out data values, it is preferable to use the AddSourceX functions.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the array.
			This id is used only by the accessor of a source.
		@param values A list of UTF-8 tokens. The members of this list will appear
			space-separated within the COLLADA document.
		@param arrayType The COLLADA element name for the output array.
			Defaults to \<Name_array\>. This might also be \<IDRef_array\>.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddArray(xmlNode* parent, const char* id, const StringList& values, const char* arrayType=DAE_NAME_ARRAY_ELEMENT);

	/** Writes out a COLLADA accessor to be used within a source.
		This function should really be called only from within AddSourceX functions.
		@param parent The parent XML tree node.
		@param arrayId The COLLADA id of the array.
		@param count The number of complete elements within the array.
		@param stride The number of values that should be used together to create one array element.
		@param parameters The list of parameter names.
			Some valid parameter names are available in the FUDaeAccessor class.
		@param type The type name of the parameters. Examples: float, float4x4, Name or IDRef.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddAccessor(xmlNode* parent, const char* arrayId, size_t count, size_t stride=1, const char** parameters=NULL, const char* type=NULL);
	inline xmlNode* AddAccessor(xmlNode* parent, const fm::string& arrayId, size_t count, size_t stride=1, const char** parameters=NULL, const char* type=NULL) { return AddAccessor(parent, arrayId.c_str(), count, stride, parameters, type); } /**< See above. */

	/** Writes out a COLLADA multi-dimensional source of floating-point values.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of floating-point values.
		@param stride The number of dimensions. This is the number of
			floating-point values that should be used together to create one element.
		@param parameters The list of accessor parameter names.
			Some valid parameter names are available in the FUDaeAccessor class.
		@param lengthFactor An optional scale factor for all the floating-point values.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceFloat(xmlNode* parent, const char* id, const FloatList& values, size_t stride=1, const char** parameters=NULL, float lengthFactor=1.0f);
	inline xmlNode* AddSourceFloat(xmlNode* parent, const fm::string& id, const FloatList& values, size_t stride=1, const char** parameters=NULL, float lengthFactor=1.0f) { return AddSourceFloat(parent, id.c_str(), values, stride, parameters, lengthFactor); } /**< See above. */

	/** Writes out a COLLADA source of floating-point values.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of floating-point values.
		@param parameter The accessor parameter name.
			Some valid parameter names are available in the FUDaeAccessor class.
		@param lengthFactor An optional scale factor for all the floating-point values.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceFloat(xmlNode* parent, const char* id, const FloatList& values, const char* parameter=NULL, float lengthFactor=1.0f);
	inline xmlNode* AddSourceFloat(xmlNode* parent, const fm::string& id, const FloatList& values, const char* parameter=NULL, float lengthFactor=1.0f) { return AddSourceFloat(parent, id.c_str(), values, parameter, lengthFactor); } /**< See above. */

	/** Writes out a COLLADA source of floating-point values.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of FMVector3 values.
		@param parameter The accessor parameter name.
			Some valid parameter names are available in the FUDaeAccessor class.
		@param lengthFactor An optional scale factor for all the floating-point values.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceFloat(xmlNode* parent, const char* id, const FMVector3List& values);
	inline xmlNode* AddSourceFloat(xmlNode* parent, const fm::string& id, const FMVector3List& values) { return AddSourceFloat(parent, id.c_str(), values); } /**< See above. */

	/** Writes out a COLLADA source of floating-point values.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of FMVector2 values.
		@param parameter The accessor parameter name.
			Some valid parameter names are available in the FUDaeAccessor class.
		@param lengthFactor An optional scale factor for all the floating-point values.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceFloat(xmlNode* parent, const char* id, const FMVector2List& values);
	inline xmlNode* AddSourceFloat(xmlNode* parent, const fm::string& id, const FMVector2List& values) { return AddSourceFloat(parent, id.c_str(), values); } /**< See above. */

	/** Writes out a COLLADA source of 2D tangent values.
	@param parent The parent XML tree node.
	@param id The COLLADA id of the source.
	@param values The list of tangent values.
	@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceTangent(xmlNode* parent, const char* id, const FMVector2List& values);
	inline xmlNode* AddSourceTangent(xmlNode* parent, const fm::string& id, const FMVector2List& values) { return AddSourceTangent(parent, id.c_str(), values); } /**< See above. */

	/** Writes out a COLLADA source of matrices.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of matrices.
		@param lengthFactor An optional scale factor for all the floating-point values.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceMatrix(xmlNode* parent, const char* id, const FMMatrix44List& values, float lengthFactor=1.0f);
	inline xmlNode* AddSourceMatrix(xmlNode* parent, const fm::string& id, const FMMatrix44List& values, float lengthFactor=1.0f) { return AddSourceMatrix(parent, id.c_str(), values, lengthFactor); } /**< See above. */

	/** Writes out a COLLADA source of matrices.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of matrices.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceColor(xmlNode* parent, const char* id, const FMVector3List& values);
	inline xmlNode* AddSourceColor(xmlNode* parent, const fm::string& id, const FMVector3List& values) { return AddSourceColor(parent, id.c_str(), values); } /**< See above. */

	/** Writes out a COLLADA source of texture coordinates.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of 3D texture coordinates.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceTexcoord(xmlNode* parent, const char* id, const FMVector3List& values);
	inline xmlNode* AddSourceTexcoord(xmlNode* parent, const fm::string& id, const FMVector3List& values) { return AddSourceTexcoord(parent, id.c_str(), values); } /**< See above. */

	/** Writes out a COLLADA source of 2D positions or vectors.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of 2D vectors.
		@param lengthFactor An optional scale factor for all the 2D vectors.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourcePosition(xmlNode* parent, const char* id, const FMVector2List& values, float lengthFactor=1.0f);
	inline xmlNode* AddSourcePosition(xmlNode* parent, const fm::string& id, const FMVector2List& values, float lengthFactor=1.0f) { return AddSourcePosition(parent, id.c_str(), values, lengthFactor); } /**< See above. */


	/** Writes out a COLLADA source of 3D positions or vectors.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of 3D vectors.
		@param lengthFactor An optional scale factor for all the 3D vectors.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourcePosition(xmlNode* parent, const char* id, const FMVector3List& values, float lengthFactor=1.0f);
	inline xmlNode* AddSourcePosition(xmlNode* parent, const fm::string& id, const FMVector3List& values, float lengthFactor=1.0f) { return AddSourcePosition(parent, id.c_str(), values, lengthFactor); } /**< See above. */

	/** Writes out a COLLADA source of 4D positions or vectors.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of 4D vectors.
		@param lengthFactor An optional scale factor for all the 4D vectors.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourcePosition(xmlNode* parent, const char* id, const FMVector4List& values, float lengthFactor=1.0f);
	inline xmlNode* AddSourcePosition(xmlNode* parent, const fm::string& id, const FMVector4List& values, float lengthFactor=1.0f) { return AddSourcePosition(parent, id.c_str(), values, lengthFactor); } /**< See above. */

	/** Writes out a COLLADA source of UTF-8 tokens.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of UTF-8 tokens. This list will be space-
			separated within the COLLADA document, so you none of the
			token should have spaces in them.
		@param parameter The name of the accessor parameter.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceString(xmlNode* parent, const char* id, const StringList& values, const char* parameter=NULL);
	inline xmlNode* AddSourceString(xmlNode* parent, const fm::string& id, const StringList& values, const char* parameter=NULL) { return AddSourceString(parent, id.c_str(), values, parameter); } /**< See above. */

	/** Writes out a COLLADA source of COLLADA references.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param values The list of COLLADA references.
		@param parameter The name of the accessor parameter.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceIDRef(xmlNode* parent, const char* id, const StringList& values, const char* parameter=NULL);
	inline xmlNode* AddSourceIDRef(xmlNode* parent, const fm::string& id, const StringList& values, const char* parameter=NULL) { return AddSourceIDRef(parent, id.c_str(), values, parameter); } /**< See above. */

	/** Writes out a COLLADA source of interpolation tokens.
		This function is used within the export of animation curves.
		@param parent The parent XML tree node.
		@param id The COLLADA id of the source.
		@param interpolations The list of interpolation tokens.
		@return The created XML tree node. */
	FCOLLADA_EXPORT xmlNode* AddSourceInterpolation(xmlNode* parent, const char* id, const FUDaeInterpolationList& interpolations);
	inline xmlNode* AddSourceInterpolation(xmlNode* parent, const fm::string& id, const FUDaeInterpolationList& values) { return AddSourceInterpolation(parent, id.c_str(), values); } /**< See above. */

	/** Cleans up a given name into a valid COLLADA id.
		This function does no check for uniqueness.
		@param id A name.
		@return A valid COLLADA id. The returned value is a static variable reference.
			If you want to keep this information, copy it to a local value. */
	FCOLLADA_EXPORT const fm::string& CleanId(const char* id);
	inline const fm::string& CleanId(const fm::string& id) { return CleanId(id.c_str()); } /**< See above. */

	/** Cleans up a given name into a valid COLLADA name.
		@param name A name.
		@return A valid COLLADA name. The returned value is a static variable reference.
			If you want to keep this information, copy it to a local value. */
	FCOLLADA_EXPORT const fstring& CleanName(const fchar* name);
	inline const fstring& CleanName(const fstring& name) { return CleanName(name.c_str()); } /**< See above. */

	/** Adds the 'sid' attribute to a given XML tree node.
		The sub-id is verified to ensure uniqueness within the scope.
		@param node The XML tree node.
		@param wantedSid The wanted sub-id.
		@return The actual sub-id written to the XML tree node.
			The returned value is a static variable reference.
			If you want to keep this information, copy it to a local value. */
	FCOLLADA_EXPORT const fm::string& AddNodeSid(xmlNode* node, const char* wantedSid);

	/** Adds the 'sid' attribute to a given XML tree node.
		The sub-id is verified to ensure uniqueness within the scope.
		@param node The XML tree node.
		@param subId The wanted sub-id. This string is modified
			to hold the actual sub-id written to the XML tree node. */
	FCOLLADA_EXPORT void AddNodeSid(xmlNode* node, fm::string& subId);
#ifdef UNICODE
	FCOLLADA_EXPORT void AddNodeSid(xmlNode* node, fstring& subId); /**< See above. */
#endif
};

#endif // HAS_LIBXML

#endif // _FU_DAE_WRITER_H_
