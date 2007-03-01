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

/**
	@file FCDMaterialInstance.h
	This file contains the FCDMaterialInstance and the FCDMaterialInstanceBind classes.
*/

#ifndef _FCD_MATERIAL_BIND_H_
#define	_FCD_MATERIAL_BIND_H_

#ifndef _FCD_ENTITY_INSTANCE_H_
#include "FCDocument/FCDEntityInstance.h"
#endif // _FCD_ENTITY_INSTANCE_H_
#ifndef _FU_DAE_ENUM_H_
#include "FUtils/FUDaeEnum.h"
#endif // _FU_DAE_ENUM_H_

class FCDocument;
class FCDGeometryPolygons;

/**
	A ColladaFX per-instance binding.
*/
class FCOLLADA_EXPORT FCDMaterialInstanceBind
{
public:
	fm::string semantic;
	fm::string target;
};

/**
	A ColladaFX per-instance vertex input binding.
*/
class FCOLLADA_EXPORT FCDMaterialInstanceBindVertexInput
{
public:
	fm::string semantic;
	FUDaeGeometryInput::Semantic inputSemantic;
	int32 inputSet;
};

/**
	A ColladaFX per-instance texture surface binding.
*/
class FCOLLADA_EXPORT FCDMaterialInstanceBindTextureSurface
{
public:
	fm::string semantic;
	int32 surfaceIndex;
	int32 textureIndex;
};

typedef fm::vector<FCDMaterialInstanceBind> FCDMaterialInstanceBindList; /**< A dynamically-sized array of per-instance binding. */
typedef fm::vector<FCDMaterialInstanceBindVertexInput> FCDMaterialInstanceBindVertexInputList; /**< A dynamically-sized array of per-instance vertex input binding. */
typedef fm::vector<FCDMaterialInstanceBindTextureSurface> FCDMaterialInstanceBindTextureSurfaceList; /**< A dynamically-sized array of per-instance texture surface binding. */

/**
	A COLLADA material instance.
	A material instance is used to given polygon sets with a COLLADA material entity.
	It is also used to bind data sources with the inputs of an effect.

	@ingroup FCDocument
*/
class FCOLLADA_EXPORT FCDMaterialInstance : public FCDEntityInstance
{
private:
	DeclareObjectType(FCDEntityInstance);
	FCDEntityInstance* parent;
	fstring semantic;
	FCDMaterialInstanceBindList bindings;
	FCDMaterialInstanceBindVertexInputList vertexBindings;

public:
	FCDMaterialInstance(FCDEntityInstance* parent);
	virtual ~FCDMaterialInstance();

	// Accessors
	virtual Type GetType() const { return MATERIAL; }
	inline FCDEntityInstance* GetParent() { return parent; }
	inline const FCDEntityInstance* GetParent() const { return parent; }

	inline const fstring& GetSemantic() const { return semantic; }
	inline void SetSemantic(const fchar* _semantic) { semantic = _semantic; SetDirtyFlag(); }
	inline void SetSemantic(const fstring& _semantic) { semantic = _semantic; SetDirtyFlag(); }

	inline FCDMaterial* GetMaterial() { return (FCDMaterial*) GetEntity(); }
	inline const FCDMaterial* GetMaterial() const { return (FCDMaterial*) GetEntity(); }
	inline void SetMaterial(FCDMaterial* _material) { SetEntity((FCDEntity*) _material); }

	inline FCDMaterialInstanceBindList& GetBindings() { return bindings; }
	inline const FCDMaterialInstanceBindList& GetBindings() const { return bindings; }
	inline size_t GetBindingCount() const { return bindings.size(); }
	inline FCDMaterialInstanceBind* GetBinding(size_t index) { FUAssert(index < bindings.size(), return NULL); return &bindings.at(index); }
	inline const FCDMaterialInstanceBind* GetBinding(size_t index) const { FUAssert(index < bindings.size(), return NULL); return &bindings.at(index); }

	// NOTE: this function uses the parent geometry instance and searches for the polygon set:
	// The application should therefore buffer the retrieved pointer.
	FCDGeometryPolygons* GetPolygons();

	FCDMaterialInstanceBind* AddBinding();
	FCDMaterialInstanceBind* AddBinding(const char* semantic, const char* target);
	inline FCDMaterialInstanceBind* AddBinding(const fm::string& semantic, const char* target) { return AddBinding(semantic.c_str(), target); }
	inline FCDMaterialInstanceBind* AddBinding(const char* semantic, const fm::string& target) { return AddBinding(semantic, target.c_str()); }
	inline FCDMaterialInstanceBind* AddBinding(const fm::string& semantic, const fm::string& target) { return AddBinding(semantic.c_str(), target.c_str()); }
	void ReleaseBinding(size_t index);

	/** Vertex input binding functions. */
	
	inline FCDMaterialInstanceBindVertexInputList& GetVertexInputBindings() { return vertexBindings; }
	inline const FCDMaterialInstanceBindVertexInputList& GetVertexInputBindings() const { return vertexBindings; }
	inline size_t GetVertexInputBindingCount() const { return vertexBindings.size(); }
	inline FCDMaterialInstanceBindVertexInput* GetVertexInputBinding(size_t index) { FUAssert(index < vertexBindings.size(), return NULL); return &vertexBindings.at(index); }
	inline const FCDMaterialInstanceBindVertexInput* GetVertexInputBinding(size_t index) const { FUAssert(index < vertexBindings.size(), return NULL); return &vertexBindings.at(index); }

	/** Retrieves a given vertex input binding.
		This is useful when trying to match textures with the texture coordinate sets.
		@param semantic A given vertex input binding semantic.
		@return The vertex input binding information structure. This pointer will be
			NULL if the given semantic is not bound within the material instance. */
	inline FCDMaterialInstanceBindVertexInput* FindVertexInputBinding(const char* semantic) { return const_cast<FCDMaterialInstanceBindVertexInput*>(const_cast<const FCDMaterialInstance*>(this)->FindVertexInputBinding(semantic)); }
	inline FCDMaterialInstanceBindVertexInput* FindVertexInputBinding(const fm::string& semantic) { return const_cast<FCDMaterialInstanceBindVertexInput*>(const_cast<const FCDMaterialInstance*>(this)->FindVertexInputBinding(semantic.c_str())); } /**< See above. */
	const FCDMaterialInstanceBindVertexInput* FindVertexInputBinding(const char* semantic) const; /**< See above. */
	inline const FCDMaterialInstanceBindVertexInput* FindVertexInputBinding(const fm::string& semantic) const { return FindVertexInputBinding(semantic.c_str()); } /**< See above. */

	FCDMaterialInstanceBindVertexInput* AddVertexInputBinding();
	FCDMaterialInstanceBindVertexInput* AddVertexInputBinding(const char* semantic, FUDaeGeometryInput::Semantic inputSemantic, int32 inputSet);


	/** Creates a flattened version of the instantiated material. This is the
		preferred way to generate viewer materials from a COLLADA document.
		@return The flattened version of the instantiated material. You
			will need to delete this pointer manually. This pointer will
			be NULL when there is no material attached to this instance. */
	FCDMaterial* FlattenMaterial();

	/** Clones the material instance.
		@param clone The material instance to become the clone.
		@return The cloned material instance. */
	virtual FCDEntityInstance* Clone(FCDEntityInstance* clone = NULL) const;

	/** [INTERNAL] Reads in the material instance from a given extra tree node.
		@param instanceNode The extra tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the instance.*/
	virtual bool LoadFromExtra(FCDENode* instanceNode);

	/** [INTERNAL] Reads in the entity instance from a given XML tree node.
		@param instanceNode The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the instance.*/
	virtual bool LoadFromXML(xmlNode* instanceNode);

	/** [INTERNAL] Writes out the emitter instance to the given extra tree node.
		@param parentNode The extra tree parent node in which to insert the instance.
		@return The create extra tree node. */
	virtual FCDENode* WriteToExtra(FCDENode* parentNode) const;

	/** [INTERNAL] Writes out the emitter instance to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the instance.
		@return The created XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_MATERIAL_BIND_H_
