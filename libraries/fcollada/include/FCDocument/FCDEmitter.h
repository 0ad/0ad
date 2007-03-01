/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_EMITTER_H_
#define _FCD_EMITTER_H_

class FCDocument;
class FCDExtra;
class FCDForceField;

class FCDEmitterParticle;

typedef FUObjectList<FCDForceField> FCDForceFieldTrackList; /**< A dynamically-sized array of tracked force fields. */

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

/**
	A COLLADA generic emitter.

	This class does not belong to the COLLADA COMMON profile.
	It is used to define both particle and sound emitters.

	@ingroup FCDocument
*/

class FCDEmitter : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);



public:

	/** Constructor.
		@param document The COLLADA document that owns the emitter. */
	FCDEmitter(FCDocument* document);

	/** Destructor. */
	~FCDEmitter();

	/** Retrieves the entity class type.
		@return The entity class type: FORCE_FIELD */
	virtual Type GetType() const { return EMITTER; }

	/** Clones the emitter information.
		@param clone The cloned emitter.
			If this pointer is NULL, a new emitter is created and
			you will need to release it manually.
		@param cloneChildren Whether to recursively clone this entity's children.
		@return The clone. */
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;


	/** Link particle systems with any particle/emitter node instances once the document is loaded. */
	bool Link();

	/** [INTERNAL] Reads in the emitter from a given COLLADA XML tree node.
		@param node The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the emitter.*/
	virtual bool LoadFromXML(xmlNode* node);

	/** [INTERNAL] Writes out the \<emitter\> element to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the geometry information.
		@return The created \<emitter\> element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};


#endif //_FCD_EMITTER_H
