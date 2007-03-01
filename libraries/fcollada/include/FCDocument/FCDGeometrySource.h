/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/**
	@file FCDGeometrySource.h
	This file contains the FCDGeometrySource class.
*/

#ifndef _FCD_GEOMETRY_SOURCE_H_
#define _FCD_GEOMETRY_SOURCE_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_
#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_

class FCDAnimated;
class FCDExtra;

/** A dynamically-sized array of FCDAnimated objects. */
typedef fm::pvector<FCDAnimated> FCDAnimatedList;

/**
	A COLLADA data source for geometric meshes.

	A COLLADA data source for geometric meshes contains a list of floating-point values and the information
	to parse these floating-point values into meaningful content: the stride of the list and the type of data
	that the floating-point values represent. When the floating-point values are split according to the stride,
	you get the individual source values of the given type. A data source may also have a user-generated name to
	identify the data within. The name is optional and is used to keep
	around the user-friendly name for texture coordinate sets or color sets.

	Each source values of the COLLADA data source may be animated individually, or together: as an element.

	@ingroup FCDGeometry
*/
class FCOLLADA_EXPORT FCDGeometrySource : public FCDObjectWithId
{
private:
	DeclareObjectType(FCDObjectWithId);
	fstring name;
	FloatList sourceData;
	uint32 sourceStride;
	xmlNode* sourceNode;
	FUDaeGeometryInput::Semantic sourceType;
	FCDExtra* extra;

	// The animated values held here are contained within the document.
	FCDAnimatedList animatedValues;

public:
	/** Constructor: do not use directly.
		Use FCDGeometryMesh::AddSource or FCDGeometryMesh::AddValueSource instead.
		@param document The COLLADA document which owns the data source.
		@param type The type of data contained within the source. */
	FCDGeometrySource(FCDocument* document, FUDaeGeometryInput::Semantic type);

	/** Destructor. */
	virtual ~FCDGeometrySource();

	/** Copies the data source into a clone.
		The clone may reside in another document.
		@param clone The empty clone. If this pointer is NULL, a new data source
			will be created and you will need to release the returned pointer manually.
		@return The clone. */
	FCDGeometrySource* Clone(FCDGeometrySource* clone = NULL) const;

	/** Retrieves the name of the data source. The name is optional and is used to
		keep around a user-friendly name for texture coordinate sets or color sets.
		@return The name of the data source. */
	inline const fstring& GetName() const { return name; }

	/** Retrieves the pure data of the data source. This is a dynamically-sized array of
		floating-point values that contains all the data of the source.
		@deprecated Use GetData() instead.
		@return The pure data of the data source. */
	inline FloatList& GetSourceData() { return sourceData; }
	inline const FloatList& GetSourceData() const { return sourceData; } /**< See above. */

	/** Retrieves the pure data of the data source. This is a dynamically-sized array of
		floating-point values that contains all the data of the source.
		@return The pure data of the data source. */
	inline FloatList& GetData() { return sourceData; }
	inline const FloatList& GetData() const { return sourceData; } /**< See above. */

	/** Retrieves the amount of data inside the source.
		@return The number of data entries in the source. */
	inline size_t GetDataCount() const { return sourceData.size(); }

	/** Retrieves the stride of the data within the source.
		There is no guarantee that the number of data values within the source is a multiple of the stride,
		yet you should always verify that the stride is at least the wanted dimension. For example, there is
		no guarantee that your vertex position data source has a stride of 3. 3dsMax is known to always
		export 3D texture coordinate positions.
		@deprecated Use GetStride() instead.
		@return The stride of the data. */
	inline uint32 GetSourceStride() const { return sourceStride; }

	/** Retrieves the stride of the data within the source.
		There is no guarantee that the number of data values within the source is a multiple of the stride,
		yet you should always verify that the stride is at least the wanted dimension. For example, there is
		no guarantee that your vertex position data source has a stride of 3. 3dsMax is known to always
		export 3D texture coordinate positions.
		@return The stride of the data. */
	inline uint32 GetStride() const { return sourceStride; }

	/** Retrieves the number of individual source values contained in the source.
		@return The number of source values. */
	inline size_t GetValueCount() const { return sourceData.size() / sourceStride; }

	/** Retrieves one source value out of this source.
		@param index The index of the source value.
		@return The source value. */
	inline float* GetValue(size_t index) { FUAssert(index < GetValueCount(), return NULL); return &sourceData.at(index * sourceStride); }
	inline const float* GetValue(size_t index) const { FUAssert(index < GetValueCount(), return NULL); return &sourceData.at(index * sourceStride); }

	/** Retrieves the type of data contained within the source.
		Common values for the type of data are POSITION, NORMAL, COLOR and TEXCOORD.
		Please see FUDaeGeometryInput for more information.
		@deprecated Use GetType() instead.
		@see FUDaeGeometryInput.
		@return The type of data contained within the source. */
	inline FUDaeGeometryInput::Semantic GetSourceType() const { return sourceType; }

	/** Retrieves the type of data contained within the source.
		Common values for the type of data are POSITION, NORMAL, COLOR and TEXCOORD.
		Please see FUDaeGeometryInput for more information.
		@see FUDaeGeometryInput.
		@return The type of data contained within the source. */
	inline FUDaeGeometryInput::Semantic GetType() const { return sourceType; }

	/** Retrieves the list of animated values for the data of the source.
		@return The list of animated values. */
	inline FCDAnimatedList& GetAnimatedValues() { return animatedValues; }
	inline const FCDAnimatedList& GetAnimatedValues() const { return animatedValues; } /**< See above. */

	/** Sets the user-friendly name of the data source. The name is optional and is used to
		keep around a user-friendly name for texture coordinate sets or color sets.
		@param _name The user-friendly name of the data source. */		
	inline void SetName(const fstring& _name) { name = _name; SetDirtyFlag(); }

	/** Overwrites the data contained within the data source.
		@param _sourceData The new data for this source.
		@param _sourceStride The stride for the new data.
		@param offset The offset at which to start retrieving the new data.
			This argument defaults at 0 to indicate that the data copy should start from the beginning.
		@param count The number of data entries to copy into the data source.
			This argument defaults at 0 to indicate that the data copy should include everything. */
	void SetSourceData(const FloatList& _sourceData, uint32 _sourceStride, size_t count=0, size_t offset=0);

	/** Sets the stride for the source data.
		@deprecated Use SetStride() instead.
		@param stride The stride for the source data. */
	inline void SetSourceStride(uint32 stride) { sourceStride = stride; SetDirtyFlag(); }

	/** Sets the stride for the source data.
		@param stride The stride for the source data. */
	inline void SetStride(uint32 stride) { sourceStride = stride; SetDirtyFlag(); }

	/** Sets the type of data contained within this data source.
		@deprecated Use SetType instead.
		@param type The new type of data for this data source. */
	void SetSourceType(FUDaeGeometryInput::Semantic type);

	/** Sets the type of data contained within this data source.
		@param type The new type of data for this data source. */
	void SetType(FUDaeGeometryInput::Semantic type);

	/** Retrieves the extra information contained by this data source.
		@return The extra tree. This pointer will be NULL,
			in the const-version of this function, if there is no extra information.
			In the modifiable-version of this function:
			you will always get a valid extra tree that you can fill in. */
	FCDExtra* GetExtra();
	inline const FCDExtra* GetExtra() const { return extra; } /**< See above. */

	/** [INTERNAL] Sets the XML tree node associated with the data source.
		@todo Take the XML tree node out of this class.
		@param _sourceNode A XML tree node. */
	inline void SetSourceNode(xmlNode* _sourceNode) { sourceNode = _sourceNode; SetDirtyFlag(); }

	/** [INTERNAL] Clones this data source. You will need to release the returned pointer manually.
		@return An identical copy of the data source. */
	FCDGeometrySource* Clone() const;

	/** [INTERNAL] Reads in the \<source\> element from a given COLLADA XML tree node.
		@param sourceNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the data source.*/
	bool LoadFromXML(xmlNode* sourceNode);

	/** [INTERNAL] Writes out the \<source\> element to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the data source.
		@return The created \<source\> element XML tree node. */
	xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_GEOMETRY_SOURCE_H_
