#include "precompiled.h"
#undef new // if it was redefined for leak detection, since xerces doesn't like it

#include "BaseEntity.h"
#include "ObjectManager.h"
#include "CStr.h"

#include "XML.h"

// automatically use namespace ..
XERCES_CPP_NAMESPACE_USE

CBaseEntity::CBaseEntity()
{
	m_base = NULL;
	m_base.associate( this, "super" );
	m_name.associate( this, "name" );
	m_speed.associate( this, "speed" );
	m_turningRadius.associate( this, "turningRadius" );
	
	m_bound_circle = NULL;
	m_bound_box = NULL;
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
		
		CVFSEntityResolver *entityResolver = new CVFSEntityResolver(filename);
		parser->setEntityResolver(entityResolver);

		// Get main node
		CVFSInputSource source;
		parseOK=source.OpenFile(filename)==0;
		
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
						CStr radius = XMLTranscode( child_element->getAttribute( (XMLCh*)L"Radius" ) );
						m_bound_circle->setRadius( radius.ToFloat() );
						m_bound_type = CBoundingObject::BOUND_CIRCLE;
					}
					else if( element_name == CStr( "Footprint" ) )
					{
						if( !m_bound_box )
							m_bound_box = new CBoundingBox();
						CStr width = XMLTranscode( child_element->getAttribute( (XMLCh*)L"Width" ) );
						CStr height = XMLTranscode( child_element->getAttribute( (XMLCh*)L"Height" ) ); 

						m_bound_box->setDimensions( width.ToFloat(), height.ToFloat() );
						m_bound_type = CBoundingObject::BOUND_OABB;
					}
					else if( element_name == CStr( "BoundsOffset" ) )
					{
						CStr x = XMLTranscode( child_element->getAttribute( (XMLCh*)L"x" ) );
						CStr y = XMLTranscode( child_element->getAttribute( (XMLCh*)L"y" ) );

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
		delete parser;
		delete errorHandler;
		delete entityResolver;
	}
	XMLPlatformUtils::Terminate();

	return parseOK;
}
