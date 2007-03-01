/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FCD_PHYSICS_PARAMETER_H_
#define _FCD_PHYSICS_PARAMETER_H_

#ifndef _FCD_PHYSICS_PARAMETER_GENERIC_H_
#include "FCDocument/FCDPhysicsParameterGeneric.h"
#endif // _FCD_PHYSICS_PARAMETER_GENERIC_H_

class FCDocument;

template <class T>
class FCOLLADA_EXPORT FCDPhysicsParameter : public FCDPhysicsParameterGeneric
{
public:
	FCDPhysicsParameter(FCDocument* document, const fm::string& ref);
	virtual ~FCDPhysicsParameter();

	// Clone
	virtual FCDPhysicsParameterGeneric* Clone(FCDPhysicsParameterGeneric* clone = NULL) const;

	void SetValue(T val);
	void SetValue(T* val);

	T* GetValue() const {return value;}

	// Flattening: overwrite the target parameter with this parameter
	virtual void Overwrite(FCDPhysicsParameterGeneric* target);

	// Write out this ColladaFX parameter to the XML node tree
	virtual xmlNode* WriteToXML(xmlNode* parentNode) const;

protected:
	T* value;
};

#endif // _FCD_PHYSICS_PARAMETER_H_
