/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_FORCE_FIELD_H_
#define _FCD_FORCE_FIELD_H_

class FCDocument;
class FCDExtra;
class FCDForce;
#include "FCDocument/FCDForceTyped.h"

#ifndef _FCD_ENTITY_H_
#include "FCDocument/FCDEntity.h"
#endif // _FCD_ENTITY_H_

/**
	A COLLADA physics force field.

	This class does not have any parameters in the COMMON profile.
	You can use the custom extra tree to enter/retrieve your
	specific customized information.

	@ingroup FCDocument
*/

class FCDForceField : public FCDEntity
{
private:
	DeclareObjectType(FCDEntity);
	FUObjectRef<FCDExtra> information;


public:
	/** Constructor.
		@param document The COLLADA document that owns the force field. */
	FCDForceField(FCDocument* document);

	/** Destructor. */
	virtual ~FCDForceField();


	/** Retrieves the extra tree for all the force field information.
		@return The extra tree. */
	inline FCDExtra* GetInformation() { return information; }
	inline const FCDExtra* GetInformation() const { return information; } /**< See above. */

	/** Retrieves the entity class type.
		@return The entity class type: FORCE_FIELD */
	virtual Type GetType() const { return FORCE_FIELD; }

	/** Clones the force field information.
		@param clone The cloned force field.
			If this pointer is NULL, a new force field is created and
			you will need to release it manually.
		@param cloneChildren Whether to recursively clone this entity's children.
		@return The clone. */
	virtual FCDEntity* Clone(FCDEntity* clone = NULL, bool cloneChildren = false) const;

	/** [INTERNAL] Reads in the force field from a given COLLADA XML tree node.
		@param node The COLLADA XML tree node.
		@return The status of the import. If the status is 'false',
			it may be dangerous to extract information from the force field.*/
	virtual bool LoadFromXML(xmlNode* node);

	/** [INTERNAL] Writes out the force field to the given COLLADA XML tree node.
		@param parentNode The COLLADA XML parent node in which to insert the geometry information.
		@return The created \<force_field\> element XML tree node. */
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;
};

#endif // _FCD_FORCE_FIELD_H_
