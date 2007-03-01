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

#include "FUtils/FUDaeParser.h"

template <class T>
FCDLibrary<T>::FCDLibrary(FCDocument* document) : FCDObject(document)
{
}

template <class T>
FCDLibrary<T>::~FCDLibrary()
{
}

// Read in a list of entities for a library of a COLLADA document
template <class T>
bool FCDLibrary<T>::LoadFromXML(xmlNode* node)
{
	bool status = true;
	for (xmlNode* entityNode = node->children; entityNode != NULL; entityNode = entityNode->next)
	{
		if (entityNode->type == XML_ELEMENT_NODE)
		{
			T* entity = AddEntity();
			status &= (entity->LoadFromXML(entityNode));
		}
	}

	SetDirtyFlag();
	return status;
}

// Write out the library to the COLLADA XML document
template <class T>
void FCDLibrary<T>::WriteToXML(xmlNode* node) const
{
	for (typename FCDEntityContainer::const_iterator itEntity = entities.begin(); itEntity != entities.end(); ++itEntity)
	{
		const T* entity = (const T*) (*itEntity);
		entity->WriteToXML(node);
	}
}

// Search for the entity in this library with a given COLLADA id.
template <class T>
T* FCDLibrary<T>::FindDaeId(const fm::string& _daeId)
{
	const char* daeId = FUDaeParser::SkipPound(_daeId);
	for (typename FCDEntityContainer::iterator itEntity = entities.begin(); itEntity != entities.end(); ++itEntity)
	{
		FCDEntity* found = (*itEntity)->FindDaeId(daeId);
		if (found != NULL && found->GetObjectType() == T::GetClassType())
		{
			return (T*) found;
		}
	}
	return NULL;
}
