#include "precompiled.h"

#include "ObjectEntry.h"
#include "ObjectManager.h"
#include "Model.h"
#include "ModelDef.h"
#include "CLogger.h"

#include "UnitManager.h"

#include "XML.h"


// automatically use namespace ..
XERCES_CPP_NAMESPACE_USE


CObjectEntry::CObjectEntry(int type) : m_Model(0), m_Type(type)
{
	m_IdleAnim=0;
	m_WalkAnim=0;
	m_DeathAnim=0;
	m_MeleeAnim=0;
	m_RangedAnim=0;
}

CObjectEntry::~CObjectEntry()
{
	for (size_t i=0;i<m_Animations.size();i++) {
		delete m_Animations[i].m_AnimData;
	}

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
	CStr modelfilename("mods/official/");
	modelfilename+=m_ModelName;

	// try and create a model
	CModelDef* modeldef;

	try {
		modeldef=CModelDef::Load((const char*) modelfilename);
	} catch (...) {
		LOG(ERROR, "CObjectEntry::BuildModel(): Model %s failed to load\n", modelfilename.c_str());
		return false;
	}

	// create new Model
	m_Model=new CModel;
	m_Model->SetTexture((const char*) m_TextureName);
	m_Model->InitModel(modeldef);

	// calculate initial object space bounds, based on vertex positions
	m_Model->CalcObjectBounds();

	// load animations
	for( uint t = 0; t < m_Animations.size(); t++ )
	{
		if( m_Animations[t].m_FileName.Length() > 0 )
		{
			CStr animfilename( "mods/official/" );
			animfilename += m_Animations[t].m_FileName;
			m_Animations[t].m_AnimData = m_Model->BuildAnimation((const char*) animfilename,m_Animations[t].m_Speed);

			if( m_Animations[t].m_AnimName.LowerCase() == CStr( "idle" ) )
				m_IdleAnim = m_Animations[t].m_AnimData;
			if( m_Animations[t].m_AnimName.LowerCase() == CStr( "walk" ) )
				m_WalkAnim = m_Animations[t].m_AnimData;
		}
		else
		{
			// FIXME, RC - don't store invalid animations (possible?)
			m_Animations[t].m_AnimData=0;
		}
	}
	// start up idling
	m_Model->SetAnimation( m_IdleAnim );

	// build props - TODO, RC - need to fix up bounds here
	for (uint p=0;p<m_Props.size();p++) {
		const Prop& prop=m_Props[p];
		SPropPoint* proppoint=modeldef->FindPropPoint((const char*) prop.m_PropPointName);
		if (proppoint) {
			CObjectEntry* oe=g_ObjMan.FindObject(prop.m_ModelName);
			if (oe) {
				// try and build model if we haven't already got it
				if (!oe->m_Model) oe->BuildModel();
				if (oe->m_Model) {
					CModel* propmodel=oe->m_Model->Clone();
					m_Model->AddProp(proppoint,propmodel);
					if (oe->m_WalkAnim) propmodel->SetAnimation(oe->m_WalkAnim);
				} else {
					LOG(ERROR,"Failed to build prop model \"%s\" on actor \"%s\"\n",(const char*) m_Name,(const char*) prop.m_ModelName);
				}
			}
		} else {
			LOG(ERROR,"Failed to matching prop point called \"%s\" in model \"%s\"\n",prop.m_PropPointName,(const char*) modelfilename);
		}
	}

	// build world space bounds
	m_Model->CalcBounds();

	// replace any units using old model to now use new model; also reprop models, if necessary
	const std::vector<CUnit*>& units=g_UnitMan.GetUnits();
	for (uint i=0;i<units.size();++i) {
		CModel* unitmodel=units[i]->GetModel();
		if (unitmodel->GetModelDef()==oldmodel) {
			unitmodel->InitModel(m_Model->GetModelDef());

			const std::vector<CModel::Prop>& newprops=m_Model->GetProps();
			for (uint j=0;j<newprops.size();j++) {
				unitmodel->AddProp(newprops[j].m_Point,newprops[j].m_Model->Clone());
			}
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
	{
		XMLCh* attachpointtext=XMLString::transcode("attachpoint");
		XMLCh* modeltext=XMLString::transcode("model");
		XMLCh* nametext=XMLString::transcode("name");
		XMLCh* filetext=XMLString::transcode("file");
		XMLCh* speedtext=XMLString::transcode("speed");

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

		CVFSEntityResolver *entityResolver = new CVFSEntityResolver(filename);
		parser->setEntityResolver(entityResolver);

		// Push the CLogger to mark it's reading this file.

		// Get main node
		CVFSInputSource source;
		parseOK = source.OpenFile(filename) == 0;
		if (parseOK)
		{
			// Parse file
			parser->parse(source);

			// Check how many errors
			parseOK = parser->getErrorCount() == 0;
		}
		if (parseOK) {
			// parsed successfully - grab our data
			DOMDocument *doc = parser->getDocument();
			DOMElement *element = doc->getDocumentElement();

			// root_name should be Object
			CStr root_name = XMLTranscode( element->getNodeName() );

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

					CStr element_name = XMLTranscode( child_element->getNodeName() );
					DOMNode *value_node= child_element->getChildNodes()->item(0);
					CStr element_value=value_node ? XMLTranscode(value_node->getNodeValue()) : "";

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
							CStr element_name = XMLTranscode( anim_element->getNodeName() );
							DOMNamedNodeMap* attributes=anim_element->getAttributes();
							if (attributes) {
								Anim anim;

								DOMNode *nameattr=attributes->getNamedItem(nametext);
								anim.m_AnimName=XMLTranscode(nameattr->getChildNodes()->item(0)->getNodeValue());
								DOMNode *fileattr=attributes->getNamedItem(filetext);
								anim.m_FileName=XMLTranscode(fileattr->getChildNodes()->item(0)->getNodeValue());

								DOMNode *speedattr=attributes->getNamedItem(speedtext);
								CStr speedstr=XMLTranscode(speedattr->getChildNodes()->item(0)->getNodeValue());

								anim.m_Speed=float(atoi((const char*) speedstr))/100.0f;
								if (anim.m_Speed<=0) anim.m_Speed=1.0f;

								m_Animations.push_back(anim);
							}
						}
					} else if (element_name==CStr("Props")) {
						DOMNodeList* props=(DOMNodeList*) child_element->getChildNodes();

						for (uint j=0; j<props->getLength(); ++j) {
							DOMElement *prop_element = (DOMElement*) props->item(j);
							CStr element_name = XMLTranscode( prop_element->getNodeName() );
							DOMNamedNodeMap* attributes=prop_element->getAttributes();
							if (attributes) {
								Prop prop;

								DOMNode *nameattr=attributes->getNamedItem(attachpointtext);
								prop.m_PropPointName=XMLTranscode(nameattr->getChildNodes()->item(0)->getNodeValue());
								DOMNode *modelattr=attributes->getNamedItem(modeltext);
								prop.m_ModelName=XMLString::transcode(modelattr->getChildNodes()->item(0)->getNodeValue());

								m_Props.push_back(prop);
							}
						}
					}
				}
			}
		}

		XMLString::release(&attachpointtext);
		XMLString::release(&modeltext);
		XMLString::release(&nametext);
		XMLString::release(&filetext);
		XMLString::release(&speedtext);

		delete parser;
		delete errorHandler;
		delete entityResolver;
	}

	return parseOK;
}

bool CObjectEntry::Save(const char* filename)
{
	FILE* fp=fopen(filename,"w");
	if (!fp) return false;

	// write XML header
	fprintf(fp,"<?xml version=\"1.0\" encoding=\"iso-8859-1\" standalone=\"no\"?>\n\n");
	fprintf(fp,"<!DOCTYPE Object SYSTEM \"../object.dtd\">\n\n");

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
