#include "AtlasObject.h"
#include "AtlasObjectImpl.h"

#include <assert.h>

#ifdef _MSC_VER
# ifndef NDEBUG
#  pragma comment(lib, "xerces-c_2D.lib")
# else
#  pragma comment(lib, "xerces-c_2.lib")
# endif
#endif

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>

XERCES_CPP_NAMESPACE_USE

static AtSmartPtr<AtNode> ConvertNode(DOMElement* element);

AtObj AtlasObject::LoadFromXML(const wchar_t* filename)
{
	// TODO: Convert wchar_t* to XMLCh* when running under GCC
	assert(sizeof(wchar_t) == sizeof(XMLCh));


	XMLPlatformUtils::Initialize();

	XercesDOMParser* parser = new XercesDOMParser();

	parser->setValidationScheme(XercesDOMParser::Val_Never);
	parser->setCreateEntityReferenceNodes(false);

	parser->parse((XMLCh*)filename);

	if (parser->getErrorCount() != 0)
	{
		assert(! "Error while loading XML - invalid XML data?");
		return AtObj();
	}

	DOMDocument* doc = parser->getDocument();
	DOMElement* root = doc->getDocumentElement();

	AtObj obj;
	obj.p = ConvertNode(root);

	AtObj rootObj;
	char* rootName = XMLString::transcode(root->getNodeName());
	rootObj.set(rootName, obj);
	XMLString::release(&rootName);

	// TODO: Initialise/terminate properly
//	XMLPlatformUtils::Terminate();

	return rootObj;
}

// Convert from a DOMElement to an AtNode
static AtSmartPtr<AtNode> ConvertNode(DOMElement* element)
{
	AtSmartPtr<AtNode> obj (new AtNode());

	// Loop through all child elements
	DOMNodeList* children = element->getChildNodes();
	XMLSize_t len = children->getLength();
	for (XMLSize_t i = 0; i < len; ++i)
	{
		DOMNode* node = children->item(i);
		short type = node->getNodeType();

		if (type == DOMNode::ELEMENT_NODE)
		{
			// Sub-element.

			// Use its name for the AtNode key
			char* name = XMLString::transcode(node->getNodeName());

			// Recursively convert the sub-element, and add it into this node
			obj->children.insert(AtNode::child_pairtype(
				name, ConvertNode((DOMElement*)node)
			));

			// Free memory
			XMLString::release((char**)&name);
		}
		else if (type == DOMNode::TEXT_NODE)
		{
			// Text inside the element. Append it to the current node's string.

			// TODO: Make this work on GCC, where wchar_t != XMLCh
			std::wstring value_wstr (node->getNodeValue());
			obj->value += value_wstr;
		}
	}

	// Trim whitespace surrounding the string value
	const std::wstring whitespace = L" \t\r\n";
	size_t first = obj->value.find_first_not_of(whitespace);
	if (first == std::wstring::npos)
		obj->value = L"";
	else
	{
		size_t last = obj->value.find_last_not_of(whitespace);
		obj->value = obj->value.substr(first, 1+last-first);
	}

	return obj;
}


// Build a DOM node from a given AtNode
static DOMNode* BuildDOM(DOMDocument* doc, const XMLCh* name, AtNode::Ptr p)
{
	DOMElement* node = doc->createElement(name);

	if (p)
	{
		// TODO: make this work on GCC
		node->setTextContent(p->value.c_str());

		XMLCh tempStr[256]; // urgh, nasty fixed-size buffer
		for (AtNode::child_maptype::const_iterator it = p->children.begin(); it != p->children.end(); ++it)
		{
			XMLString::transcode(it->first.c_str(), tempStr, 255);
			node->appendChild(BuildDOM(doc, tempStr, it->second));
		}
	}

	return node;
}

bool AtlasObject::SaveToXML(AtObj& obj, const wchar_t* filename)
{
	// TODO: Convert wchar_t* to XMLCh* when running under GCC
	assert(sizeof(wchar_t) == sizeof(XMLCh));


	XMLPlatformUtils::Initialize();

	// Why does it take so much work just to create a DOMWriter? :-(
	XMLCh domFeatures[100];
	XMLString::transcode("LS", domFeatures, 99); // maybe "LS" means "load/save", but I really don't know
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(domFeatures);
	DOMWriter* writer = ((DOMImplementationLS*)impl)->createDOMWriter();

	if (writer->canSetFeature(XMLUni::fgDOMWRTDiscardDefaultContent, true))
		writer->setFeature(XMLUni::fgDOMWRTDiscardDefaultContent, true);

	if (writer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true))
		writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);


	// Find the root element of the object:

	if (obj.p->children.size() != 1)
	{
		assert(! "SaveToXML: root must only have one child");
		return false;
	}
	XMLCh rootName[255];
	XMLString::transcode(obj.p->children.begin()->first.c_str(), rootName, 255);
	AtNode::Ptr firstChild (obj.p->children.begin()->second);

	try
	{
		DOMDocument* doc = impl->createDocument();
		doc->appendChild(BuildDOM(doc, rootName, firstChild));

		XMLFormatTarget* FormatTarget = new LocalFileFormatTarget((XMLCh*)filename);
		writer->writeNode(FormatTarget, *doc);
		delete FormatTarget;
	}
	catch (const XMLException& e) {
		char* message = XMLString::transcode(e.getMessage());
		assert(! "XML exception - maybe failed while writing the file");
		XMLString::release(&message);
//		XMLPlatformUtils::Terminate();
		return false;
	}
	catch (const DOMException& e) {
		char* message = XMLString::transcode(e.msg);
		assert(! "DOM exception");
		XMLString::release(&message);
//		XMLPlatformUtils::Terminate();
		return false;
	}

	writer->release();

//	XMLPlatformUtils::Terminate();

	return true;
}
