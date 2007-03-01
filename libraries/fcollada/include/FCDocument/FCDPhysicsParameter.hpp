/*
    Copyright (C) 2005-2007 Feeling Software Inc.
    MIT License: http://www.opensource.org/licenses/mit-license.php
*/

#ifndef _FC_DOCUMENT_H_
#include "FCDocument/FCDocument.h"
#endif // _FC_DOCUMENT_H_
#ifndef _DAE_SYNTAX_H_
#include "FUtils/FUDaeSyntax.h"
#endif // _DAE_SYNTAX_H_
#ifndef _FU_XML_WRITER_H_
#include "FUtils/FUXmlWriter.h"
#endif // _FU_XML_WRITER_H_

template <class T>
FCDPhysicsParameter<T>::FCDPhysicsParameter(FCDocument* document, const fm::string& ref) : FCDPhysicsParameterGeneric(document, ref)
{
	value = NULL;
}

template <class T>
FCDPhysicsParameter<T>::~FCDPhysicsParameter()
{
	SAFE_DELETE(value);
}

// Clone
template <class T>
FCDPhysicsParameterGeneric* FCDPhysicsParameter<T>::Clone(FCDPhysicsParameterGeneric* _clone) const
{
	FCDPhysicsParameter<T>* clone = NULL;
	if (_clone == NULL) _clone = clone = new FCDPhysicsParameter<T>(const_cast<FCDocument*>(GetDocument()), reference);
	else /* Only one up-cast and no way to verify it.. */ clone = (FCDPhysicsParameter<T>*) _clone;

	if (clone != NULL)
	{
		clone->value = new T(*value);
	}

	return _clone;
}

template <class T>
void FCDPhysicsParameter<T>::SetValue(T val)
{
	SAFE_DELETE(value);
	value = new T();
	*value = val;
	SetDirtyFlag();
}


template <class T>
void FCDPhysicsParameter<T>::SetValue(T* val)
{
	SAFE_DELETE(value);
	value = val;
	SetDirtyFlag();
}

// Flattening: overwrite the target parameter with this parameter
template <class T>
void FCDPhysicsParameter<T>::Overwrite(FCDPhysicsParameterGeneric* target)
{
	((FCDPhysicsParameter<T>*) target)->SetValue(value);
}

// Write out this ColladaFX parameter to the XML node tree
template <class T>
xmlNode* FCDPhysicsParameter<T>::WriteToXML(xmlNode* parentNode) const
{
	xmlNode* parameterNode = FUXmlWriter::AddChild(parentNode, reference.c_str());
	AddContent(parameterNode, FUStringConversion::ToString(*value));
	
	//the translate and rotate elements need to be in a <mass_frame> tag.
	if ((reference == DAE_TRANSLATE_ELEMENT) || (reference == DAE_ROTATE_ELEMENT))
	{
		xmlNode* massFrameNode = FUXmlWriter::AddChild(parentNode, DAE_MASS_FRAME_ELEMENT);
		FUXmlWriter::ReParentNode(parameterNode, massFrameNode);
	}
	return parameterNode;
}
