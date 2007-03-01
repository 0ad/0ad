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
	@file FCDLibrary.h
	This file contains the FCDLibrary template class.
	See the FCDLibrary.hpp file for the template implementation.
*/

#ifndef _FCD_LIBRARY_
#define _FCD_LIBRARY_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDocument;
class FCDEntity;

/**
	A COLLADA library.

	A COLLADA library holds a list of entities. There are libraries for the following entities:
	animations (FCDAnimation), animation clips (FCDAnimationClip), meshes and splines (FCDGeometry),
	materials (FCDMaterial), effects (FCDEffect), images (FCDImage), skins and morphers (FCDController),
	cameras (FCDCamera), lights (FCDLight), physics models (FCDPhysicsModel), physics materials
	(FCDPhysicsMaterial), physics scenes (FCDPhysicsScene) and visual scenes (FCDSceneNode).

	The COLLADA libraries are contained within the FCDocument object.

	@ingroup FCDocument
*/	
template <class T>
class FCDLibrary : public FCDObject
{
protected:
	/** The containment dynamically-sized array for the entities. */
	typedef FUObjectContainer<T> FCDEntityContainer;

	/** Entities list. This list should contain all the root entities of the correct type.
		Note that the following entity types are tree-based, rather than list-based: FCDAnimation,
		FCDSceneNode and FCDPhysicsScene. */
	FCDEntityContainer entities;

public:
	/** Constructor: do not use directly.
		All the necessary libraries are created by the FCDocument object during its creation.
		@param document The parent document. */
	FCDLibrary(FCDocument* document);

	/** Destructor. */
	virtual ~FCDLibrary();

	/** Create a new entity within this library.
		@return The newly created entity. */
	inline T* AddEntity() { return entities.Add(GetDocument()); SetDirtyFlag(); }

	/** Insert a new entity to this library.
		This function is useful if you are adding cloned entites
		back inside the library.
		@return The entity to insert. */
	inline void AddEntity(T* entity) { return entities.push_back(entity); SetDirtyFlag(); }

	/** Retrieve the library entity with the given COLLADA id.
		@param daeId The COLLADA id of the entity.
		@return The library entity which matches the COLLADA id.
			This pointer will be NULL if no matching entity was found. */
	T* FindDaeId(const fm::string& daeId);

	/** Returns whether the library contains no entities.
		@return Whether the library is empty. */
	inline bool IsEmpty() const { return entities.empty(); }

	/** [INTERNAL] Reads in the contents of the library from the COLLADA XML document.
		@param node The COLLADA XML tree node to parse into entities.
		@return The status of the import. If the status is 'false', it may be dangerous to
			extract information from the library. */
	bool LoadFromXML(xmlNode* node);

	/** [INTERNAL] Writes out the library entities to the COLLADA XML document.
		@param node The COLLADA XML tree node in which to write the library entities. */
	void WriteToXML(xmlNode* node) const;

	/** Retrieve the number of entities within the library.
		@return the number of entities contained within the library. */
	inline size_t GetEntityCount() const { return entities.size(); }

	/** Retrieve an indexed entity from the library.
		@param index The index of the entity to retrieve.
			Should be within the range [0, GetEntityCount()[.
		@return The indexed entity. */
	inline T* GetEntity(size_t index) { FUAssert(index < GetEntityCount(), return NULL); return entities.at(index); }

	/** Retrieve an indexed entity from the library.
		@param index The index of the entity to retrieve.
			Should be within the range [0, GetEntityCount()[.
		@return The indexed entity. */
	inline const T* GetEntity(size_t index) const { FUAssert(index < GetEntityCount(), return NULL); return entities.at(index); }
};

#endif // _FCD_LIBRARY_
