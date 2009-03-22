#include "AtlasObject.h"
#include "AtlasObjectImpl.h"

#include <assert.h>

#include <memory>
#include <fstream>

#ifdef _MSC_VER
# ifndef NDEBUG
#  pragma comment(lib, "xerces-c_2D.lib")
# else
#  pragma comment(lib, "xerces-c_2.lib")
# endif

// Disable some warnings:
//   "warning C4673: throwing 'blahblahException' the following types will not be considered at the catch site ..."
//   "warning C4671: 'XMemory' : the copy constructor is inaccessible"
//   "warning C4244: 'return' : conversion from '__w64 int' to 'unsigned long', possible loss of data"
# pragma warning(disable: 4673 4671 4244)

#endif // _MSC_VER

#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/framework/LocalFileFormatTarget.hpp>
#include <xercesc/sax/SAXParseException.hpp>
#include <xercesc/sax/ErrorHandler.hpp>

#include <libxml/parser.h>

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

template <typename T>
class StrConv
{
public:
	template <typename S> StrConv(const S* str) { init(str); }
	template <typename S> StrConv(const std::basic_string<S>& str) { init(str.c_str()); }
	
	~StrConv()
	{
		delete[] data;
	}
	const T* c_str()
	{
		return data;	
	}
	
private:
	template <typename S>
	void init(S* str)
	{
		size_t len = 0;
		while (str[len] != '\0')
			++len;
		data = new T[len+1];
		for (size_t i = 0; i < len+1; ++i)
			data[i] = (T)str[i];
	}
	
	T* data;
};

// UTF conversion code adapted from http://www.unicode.org/Public/PROGRAMS/CVTUTF/ConvertUTF.c
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
class toXmlChar
{
public:
	toXmlChar(const std::wstring& str)
	{
		for (size_t i = 0; i < str.length(); ++i)
		{
			unsigned short bytesToWrite;
			wchar_t ch = str[i];

			if (ch < 0x80) bytesToWrite = 1;
			else if (ch < 0x800) bytesToWrite = 2;
			else if (ch < 0x10000) bytesToWrite = 3;
			else if (ch < 0x110000) bytesToWrite = 4;
			else bytesToWrite = 3, ch = 0xFFFD; // replacement character

			char buf[4];
			char* target = &buf[bytesToWrite];
			switch (bytesToWrite)
			{
			case 4: *--target = ((ch | 0x80) & 0xBF); ch >>= 6;
			case 3: *--target = ((ch | 0x80) & 0xBF); ch >>= 6;
			case 2: *--target = ((ch | 0x80) & 0xBF); ch >>= 6;
			case 1: *--target = (ch | firstByteMark[bytesToWrite]);
			}
			data += std::string(buf, bytesToWrite);
		}
	}
	operator const xmlChar*()
	{
		return (const xmlChar*)data.c_str();
	}

private:
	std::string data;
};

// TODO: replace most of the asserts below (e.g. for when it fails to load
// a file) with some proper logging/reporting system

static AtSmartPtr<AtNode> ConvertNode(DOMElement* element);

AtObj AtlasObject::LoadFromXML(const wchar_t* filename)
{
	XercesInitialiser::enable();

	XercesDOMParser* parser = new XercesDOMParser();

	XercesErrorHandler ErrHandler;
	parser->setErrorHandler(&ErrHandler);
	parser->setValidationScheme(XercesDOMParser::Val_Never);
	parser->setLoadExternalDTD(false); // because I really don't like bothering with them
	parser->setCreateEntityReferenceNodes(false);

	parser->parse(StrConv<XMLCh>(filename).c_str());

	if (parser->getErrorCount() != 0)
	{
		delete parser;
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

	delete parser;

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
			std::wstring value_wstr (StrConv<wchar_t>(node->getNodeValue()).c_str());
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

			// Get name and value
			char* name = XMLString::transcode(attr->getName());
			StrConv<wchar_t> value (attr->getValue());

			// Prefix the name with an @, to differentiate it from an element
			std::string newName ("@"); newName += name;

			// Create new node
			AtNode* newNode = new AtNode(value.c_str());

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
static void BuildDOMNode(xmlDocPtr doc, xmlNodePtr node, AtNode::Ptr p)
{
	if (p)
	{
		if (p->value.length())
			xmlNodeAddContent(node, toXmlChar(p->value));

		for (AtNode::child_maptype::const_iterator it = p->children.begin(); it != p->children.end(); ++it)
		{
			// Test for attribute nodes (whose names start with @)
			if (it->first.length() && it->first[0] == '@')
			{
				assert(it->second);
				assert(it->second->children.empty());
				xmlNewProp(node, (const xmlChar*)it->first.c_str()+1, toXmlChar(it->second->value));
			}
			else
			{
				if (node == NULL) // first node in the document - needs to be made the root node
				{
					xmlNodePtr root = xmlNewNode(NULL, (const xmlChar*)it->first.c_str());
					xmlDocSetRootElement(doc, root);
					BuildDOMNode(doc, root, it->second);
				}
				else
				{
					xmlNodePtr child = xmlNewChild(node, NULL, (const xmlChar*)it->first.c_str(), NULL);
					BuildDOMNode(doc, child, it->second);
				}
			}
		}
	}
}

std::string AtlasObject::SaveToXML(AtObj& obj)
{
	if (!obj.p || obj.p->children.size() != 1)
	{
		assert(! "SaveToXML: root must only have one child");
		return "";
	}

	AtNode::Ptr firstChild (obj.p->children.begin()->second);

	xmlDocPtr doc = xmlNewDoc((const xmlChar*)"1.0");
	BuildDOMNode(doc, NULL, obj.p);

	xmlChar* buf;
	int size;
	xmlDocDumpFormatMemoryEnc(doc, &buf, &size, "utf-8", 1);

	std::string ret((const char*)buf, size);

	xmlFree(buf);
	xmlFreeDoc(doc);

	// TODO: handle errors better

	return ret;
}
