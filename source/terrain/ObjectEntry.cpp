#include "ObjectEntry.h"
#include "ObjectManager.h"
#include "terrain/Model.h"
#include "terrain/ModelDef.h"

#include "UnitManager.h"

// xerces XML stuff
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

// Gee's custom error handler
#include <ps/XercesErrorHandler.h>

// automatically use namespace ..
XERCES_CPP_NAMESPACE_USE


CObjectEntry::CObjectEntry(int type) : m_Model(0), m_Type(type)
{
}

CObjectEntry::~CObjectEntry()
{
	delete m_Model;
}

bool CObjectEntry::BuildModel()
{
	// check we've enough data to consider building the object
	if (m_ModelName.Length()==0 || m_TextureName.Length()==0) {
		return false;
	}

	// get the root directory of this object
	CStr dirname=g_ObjMan.m_ObjectTypes[m_Type].m_Name;

	// remember the old model so we can replace any models using it later on
	CModelDef* oldmodel=m_Model ? m_Model->GetModelDef() : 0;

	// build filename
	CStr modelfilename("mods\\official\\");
	modelfilename+=m_ModelName;	
	
	// try and create a model
	CModelDef* modeldef;
	
	try {
		modeldef=CModelDef::Load((const char*) modelfilename);
	} catch (...) {
		return false;
	}

	// create new Model 
	m_Model=new CModel;
	m_Model->InitModel(modeldef);

	CStr texturefilename(m_TextureName);	

	m_Model->SetTexture(CTexture((const char*) texturefilename));

	for( uint t = 0; t < m_Animations.size(); t++ )
	{
		if( m_Animations[t].m_FileName.Length() > 0 )
		{
			CStr animfilename( "mods\\official\\" );
			animfilename += m_Animations[t].m_FileName;
			try
			{
				m_Animations[t].m_AnimData = CSkeletonAnim::Load( animfilename );
			}
			catch( ... )
			{
				m_Animations[t].m_AnimData = NULL;
			}
			if( m_Animations[t].m_AnimName.LowerCase() == CStr( "idle" ) )
				m_IdleAnim = m_Animations[t].m_AnimData;
			if( m_Animations[t].m_AnimName.LowerCase() == CStr( "walk" ) )
				m_WalkAnim = m_Animations[t].m_AnimData;
		}
	}
	m_Model->SetAnimation( m_IdleAnim );
	
	// rebuild model bounds	
	m_Model->CalcBounds();

	// replace any units using old model to now use new model
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	for (uint i=0;i<units.size();++i) {
		if (units[i]->m_Model->GetModelDef()==oldmodel) {
			units[i]->m_Model->InitModel(m_Model->GetModelDef());

		}
	}

	// and were done with the old model ..
	delete oldmodel;
	
	return true;
}

CSkeletonAnim* CObjectEntry::GetNamedAnimation( CStr animationName )
{
	for( uint t = 0; t < m_Animations.size(); t++ )
	{
		if( m_Animations[t].m_AnimName == animationName )
			return( m_Animations[t].m_AnimData );
	}
	return( NULL );
}

bool CObjectEntry::Load(const char* filename)
{
	bool parseOK = false;

	// Initialize XML library
	XMLPlatformUtils::Initialize();
	{
		// Create parser instance
		XercesDOMParser *parser = new XercesDOMParser();

		// Setup parser
		parser->setValidationScheme(XercesDOMParser::Val_Auto);
		parser->setDoNamespaces(false);
		parser->setDoSchema(false);
		parser->setCreateEntityReferenceNodes(false);

		// Set customized error handler
		CXercesErrorHandler *errorHandler = new CXercesErrorHandler();
		parser->setErrorHandler(errorHandler);

		// Push the CLogger to mark it's reading this file.

		// Get main node
		LocalFileInputSource source( XMLString::transcode(filename) );

		// Parse file
		parser->parse(source);

		// Check how many errors
		parseOK = parser->getErrorCount() == 0;

		if (parseOK) {
			// parsed successfully - grab our data
			DOMDocument *doc = parser->getDocument(); 
			DOMElement *element = doc->getDocumentElement(); 

			// root_name should be Object
			CStr root_name = XMLString::transcode( element->getNodeName() );

			// should have at least 3 children - Name, ModelName and TextureName
			DOMNodeList *children = element->getChildNodes();
			int numChildren=children->getLength();
			for (int i=0; i<numChildren; ++i) {
				// Get node
				DOMNode *child = children->item(i);

				// A child element
				if (child->getNodeType() == DOMNode::ELEMENT_NODE)
				{
					// First get element and not node
					DOMElement *child_element = (DOMElement*)child;

					CStr element_name = XMLString::transcode( child_element->getNodeName() );
					DOMNode *value_node= child_element->getChildNodes()->item(0);
					CStr element_value=value_node ? XMLString::transcode(value_node->getNodeValue()) : "";

					if (element_name==CStr("Name")) {
						m_Name=element_value;
					} else if (element_name==CStr("ModelName")) {
						m_ModelName=element_value;
					} else if (element_name==CStr("TextureName")) {
						m_TextureName=element_value;
					} else if (element_name==CStr("Animations")) {
						DOMNodeList* animations=(DOMNodeList*) child_element->getChildNodes();

						for (uint j=0; j<animations->getLength(); ++j) {
							DOMElement *anim_element = (DOMElement*) animations->item(j);
							CStr element_name = XMLString::transcode( anim_element->getNodeName() );
							DOMNamedNodeMap* attributes=anim_element->getAttributes();
							if (attributes) {
								Anim anim;

								DOMNode *nameattr=attributes->getNamedItem(XMLString::transcode("name"));
								anim.m_AnimName=XMLString::transcode(nameattr->getChildNodes()->item(0)->getNodeValue());
								DOMNode *fileattr=attributes->getNamedItem(XMLString::transcode("file"));
								anim.m_FileName=XMLString::transcode(fileattr->getChildNodes()->item(0)->getNodeValue());

								m_Animations.push_back(anim);
							}
						}
					}
				}
			}

			// try and build the model
			BuildModel();
		}

		delete parser;
	}
	XMLPlatformUtils::Terminate();

	return parseOK;
}

bool CObjectEntry::Save(const char* filename)
{
	FILE* fp=fopen(filename,"w");
	if (!fp) return false;

	// write XML header
	fprintf(fp,"<?xml version=\"1.0\" encoding=\"iso-8859-1\" standalone=\"no\"?>\n\n");
	fprintf(fp,"<!DOCTYPE Object SYSTEM \"..\\object.dtd\">\n\n");

	// write the object itself
	fprintf(fp,"<!-- File automatically generated by ScEd -->\n");
	fprintf(fp,"<Object>\n");
	fprintf(fp,"\t<Name>%s</Name>\n",(const char*) m_Name);
	fprintf(fp,"\t<ModelName>%s</ModelName>\n",(const char*) m_ModelName);
	fprintf(fp,"\t<TextureName>%s</TextureName>\n",(const char*) m_TextureName);
	if (m_Animations.size()>0) {
		fprintf(fp,"\t<Animations>\n");
		for (uint i=0;i<m_Animations.size();i++) {
			fprintf(fp,"\t\t<Animation name=\"%s\" file=\"%s\"> </Animation>\n",(const char*) m_Animations[i].m_AnimName,(const char*) m_Animations[i].m_FileName);
		}
		fprintf(fp,"\t</Animations>\n");
	}
	fprintf(fp,"</Object>\n");
	fclose(fp);

	return true;
}
