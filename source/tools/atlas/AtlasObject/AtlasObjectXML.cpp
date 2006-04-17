#include "AtlasObject.h"
#include "AtlasObjectImpl.h"

#include <assert.h>

#ifdef _MSC_VER
# ifndef NDEBUG
#  pragma comment(lib, "xerces-c_2D.lib")
# else
#  pragma comment(lib, "xerces-c_2.lib")
# endif

// Disable some warnings:
//   "warning C4673: throwing 'blahblahException' the following types will not be considered at the catch site ..."
//   "warning C4671: 'XMemory' : the copy constructor is inaccessible"
# pragma warning(disable: 4673 4671)

#endif // _MSC_VER

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/ErrorHandler.hpp>

XERCES_CPP_NAMESPACE_USE

class XercesInitialiser
{
	 XercesInitialiser() { XMLPlatformUtils::Initialize(); }
	~XercesInitialiser() { XMLPlatformUtils::Terminate (); }
public:
	static void enable()
	{
		static XercesInitialiser x;
	}
};

class XercesErrorHandler : public ErrorHandler
{
public:
	XercesErrorHandler() : fSawErrors(false) {}
	~XercesErrorHandler() {}
	void warning(const SAXParseException& err) { complain(err, "warning"); }
	void error(const SAXParseException& err) { complain(err, "error"); }
	void fatalError(const SAXParseException& err) { complain(err, "fatal error"); };
	void resetErrors() { fSawErrors = false; }
	bool getSawErrors() const { return fSawErrors; }
private:
	bool fSawErrors;

	void complain(const SAXParseException& err, const char* severity)
	{
		fSawErrors = true;
		char* systemId = XMLString::transcode(err.getSystemId());
		char* message = XMLString::transcode(err.getMessage());
		// TODO: do something
		(void)severity;
		XMLString::release(&systemId);
		XMLString::release(&message);
	}
};


static AtSmartPtr<AtNode> ConvertNode(DOMElement* element);

AtObj AtlasObject::LoadFromXML(const wchar_t* filename)
{
	// TODO: Convert wchar_t* to XMLCh* when running under GCC
	assert(sizeof(wchar_t) == sizeof(XMLCh));


	XercesInitialiser::enable();

	XercesDOMParser* parser = new XercesDOMParser();

	XercesErrorHandler ErrHandler;
	parser->setErrorHandler(&ErrHandler);
	parser->setValidationScheme(XercesDOMParser::Val_Never);
	parser->setLoadExternalDTD(false); // because I really don't like bothering with them
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
			XMLString::release(&name);
		}
		else if (type == DOMNode::TEXT_NODE)
		{
			// Text inside the element. Append it to the current node's string.

			// TODO: Make this work on GCC, where wchar_t != XMLCh
			assert(sizeof(wchar_t) == sizeof(XMLCh));
			std::wstring value_wstr (reinterpret_cast<const wchar_t*>(node->getNodeValue()));
			obj->value += value_wstr;
		}
	}

	DOMNamedNodeMap* attrs = element->getAttributes();
	len = attrs->getLength();
	for (XMLSize_t i = 0; i < len; ++i)
	{
		DOMNode* node = attrs->item(i);
		if (node->getNodeType() == DOMNode::ATTRIBUTE_NODE)
		{
			DOMAttr* attr = (DOMAttr*)node;

			// Get name and value. (TODO: GCC)
			char* name = XMLString::transcode(attr->getName());
			const wchar_t* value = reinterpret_cast<const wchar_t*>(attr->getValue());

			// Prefix the name with an @, to differentiate it from an element
			std::string newName ("@"); newName += name;

			// Create new node
			AtNode* newNode = new AtNode(value);

			// Add to this node's list of children
			obj->children.insert(AtNode::child_pairtype(newName.c_str(), AtNode::Ptr(newNode)));

			// Free memory
			XMLString::release(&name);
		}
		else
		{
			assert(! "Invalid attribute node");
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
static DOMAttr* BuildDOMAttr(DOMDocument* doc, const XMLCh* name, AtNode::Ptr p)
{
	assert(p); // attributes must contain some data
	assert(p->children.size() == 0); // attributes mustn't contain nested data

	if (!p || p->children.size() != 0)
	{
		// Oops - invalid data
		return NULL;
	}

	DOMAttr* attr = doc->createAttribute(name);

	// TODO: make work on GCC
	attr->setValue(reinterpret_cast<const XMLCh*>(p->value.c_str()));

	return attr;
}

// Build a DOM node from a given AtNode
static DOMNode* BuildDOMNode(DOMDocument* doc, const XMLCh* name, AtNode::Ptr p)
{
	DOMElement* node = doc->createElement(name);

	if (p)
	{
		// TODO: make this work on GCC
		if (p->value.length())
			node->setTextContent(reinterpret_cast<const XMLCh*>(p->value.c_str()));

		XMLCh tempStr[256]; // urgh, nasty fixed-size buffer
		for (AtNode::child_maptype::const_iterator it = p->children.begin(); it != p->children.end(); ++it)
		{
			// Test for attribute nodes (whose names start with @)
			if (it->first.length() && it->first[0] == '@')
			{
				XMLString::transcode(it->first.c_str()+1, tempStr, 255);
				node->setAttributeNode(BuildDOMAttr(doc, tempStr, it->second));
			}
			else
			{
				XMLString::transcode(it->first.c_str(), tempStr, 255);
				node->appendChild(BuildDOMNode(doc, tempStr, it->second));
			}
		}
	}

	return node;
}

bool AtlasObject::SaveToXML(AtObj& obj, const wchar_t* filename)
{
	// TODO: Convert wchar_t* to XMLCh* when running under GCC
	assert(sizeof(wchar_t) == sizeof(XMLCh));


	XercesInitialiser::enable();

	// Why does it take so much work just to create a standard DOMWriter? :-(
	XMLCh domFeatures[100] = { 0 };
	XMLString::transcode("LS", domFeatures, 99); // maybe "LS" means "load/save", but I really don't know
	DOMImplementation* impl = DOMImplementationRegistry::getDOMImplementation(domFeatures);
	DOMWriter* writer = ((DOMImplementationLS*)impl)->createDOMWriter();

	if (writer->canSetFeature(XMLUni::fgDOMWRTDiscardDefaultContent, true))
		writer->setFeature(XMLUni::fgDOMWRTDiscardDefaultContent, true);

	if (writer->canSetFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true))
		writer->setFeature(XMLUni::fgDOMWRTFormatPrettyPrint, true);


	// Find the root element of the object:

	if (!obj.p || obj.p->children.size() != 1)
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
		doc->appendChild(BuildDOMNode(doc, rootName, firstChild));

		XMLFormatTarget* FormatTarget = new LocalFileFormatTarget((XMLCh*)filename);
		writer->writeNode(FormatTarget, *doc);
		delete FormatTarget;
	}
	catch (const XMLException& e) {
		char* message = XMLString::transcode(e.getMessage());
		assert(! "XML exception - maybe failed while writing the file");
		XMLString::release(&message);
		return false;
	}
	catch (const DOMException& e) {
		char* message = XMLString::transcode(e.msg);
		assert(! "DOM exception");
		XMLString::release(&message);
		return false;
	}

	writer->release();

	return true;
}
