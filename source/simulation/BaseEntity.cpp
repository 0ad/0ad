#include "BaseEntity.h"
#include "ObjectManager.h"
#include "CStr.h"

// xerces XML stuff
#include <xercesc/dom/DOM.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>

// Gee's custom error handler
#include "ps/XercesErrorHandler.h"

// automatically use namespace ..
XERCES_CPP_NAMESPACE_USE

CBaseEntity::CBaseEntity( const CBaseEntity& copy )
{
	m_actorObject = copy.m_actorObject;

	m_name = copy.m_name;
	m_bound_type = copy.m_bound_type;
	m_speed = copy.m_speed;
	m_turningRadius = copy.m_turningRadius;

	m_bound_circle = NULL;
	m_bound_box = NULL;
	if( copy.m_bound_circle )
		m_bound_circle = new CBoundingCircle( 0.0f, 0.0f, copy.m_bound_circle );
	if( copy.m_bound_box )
		m_bound_box = new CBoundingBox( 0.0f, 0.0f, 0.0f, copy.m_bound_box );
}

CBaseEntity::~CBaseEntity()
{
	if( m_bound_box )
		delete( m_bound_box );
	if( m_bound_circle )
		delete( m_bound_circle );
}

bool CBaseEntity::loadXML( CStr filename )
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

					//m_properties[element_name] = element_value;

					if( element_name == CStr( "Name" ) )
					{
						m_name = element_value;
					}
					else if( element_name == CStr( "Actor" ) ) 
					{
						m_actorObject = g_ObjMan.FindObject( element_value );
					}
					else if( element_name == CStr( "Speed" ) )
					{
						m_speed = element_value.ToFloat();
					}
					else if( element_name == CStr( "TurningRadius" ) )
					{
						m_turningRadius = element_value.ToFloat();
					}
					else if( element_name == CStr( "Size" ) )
					{
						if( !m_bound_circle )
							m_bound_circle = new CBoundingCircle();
						CStr radius = XMLString::transcode( child_element->getAttribute( XMLString::transcode( "Radius" ) ) );
						m_bound_circle->setRadius( radius.ToFloat() );
						m_bound_type = CBoundingObject::BOUND_CIRCLE;
					}
					else if( element_name == CStr( "Footprint" ) )
					{
						if( !m_bound_box )
							m_bound_box = new CBoundingBox();
						CStr width = XMLString::transcode( child_element->getAttribute( XMLString::transcode( "Width" ) ) );
						CStr height = XMLString::transcode( child_element->getAttribute( XMLString::transcode( "Height" ) ) ); 

						m_bound_box->setDimensions( width.ToFloat(), height.ToFloat() );
						m_bound_type = CBoundingObject::BOUND_OABB;
					}
					else if( element_name == CStr( "BoundsOffset" ) )
					{
						CStr x = XMLString::transcode( child_element->getAttribute( XMLString::transcode( "x" ) ) );
						CStr y = XMLString::transcode( child_element->getAttribute( XMLString::transcode( "y" ) ) );

						if( !m_bound_circle )
							m_bound_circle = new CBoundingCircle();
						if( !m_bound_box )
							m_bound_box = new CBoundingBox();

						m_bound_circle->m_offset.x = x.ToFloat();
						m_bound_circle->m_offset.y = y.ToFloat();
						m_bound_box->m_offset.x = x.ToFloat();
						m_bound_box->m_offset.y = y.ToFloat();

					}
					
				}
			}

		}
		delete errorHandler;
		delete parser;
	}
	XMLPlatformUtils::Terminate();

	return parseOK;
}
