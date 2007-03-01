/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_PHYSICS_PARAMETER_GENERIC_H_
#define _FCD_PHYSICS_PARAMETER_GENERIC_H_

#ifndef __FCD_OBJECT_H_
#include "FCDocument/FCDObject.h"
#endif // __FCD_OBJECT_H_

class FCDocument;

class FCOLLADA_EXPORT FCDPhysicsParameterGeneric : public FCDObject
{
private:
	DeclareObjectType(FCDObject);

protected:
	bool isGenerator; // whether this effect parameter structure generates a new value or modifies an existing value (is <newparam>?)
	fm::string reference;

public:
	FCDPhysicsParameterGeneric(FCDocument* document, const fm::string& ref);
	virtual ~FCDPhysicsParameterGeneric();

	bool IsGenerator() const { return isGenerator; }
	bool IsModifier() const { return !isGenerator; }

	const fm::string& GetReference() const {return reference;};
	void SetReference(const fm::string& ref) { reference = ref; SetDirtyFlag(); };
	void SetGenerator(bool val) { isGenerator = val; SetDirtyFlag(); }

	virtual void Overwrite(FCDPhysicsParameterGeneric* target) = 0;

	// Clones the parameter
	virtual FCDPhysicsParameterGeneric* Clone(FCDPhysicsParameterGeneric* clone = NULL) const = 0;

	// Write out this ColladaFX parameter to the XML node tree
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const = 0;
};

#endif // _FCD_PHYSICS_PARAMETER_GENERIC_H_
