#include "precompiled.h"

#include "XMB.h"
#include "Stream/Stream.h"

#include <cassert>
#include <set>
#include <map>
#include <stack>

#ifdef _MSC_VER // shut up about Xerces headers
# pragma warning(disable: 4127) // conditional expression is constant
# pragma warning(disable: 4671) // the copy constructor is inaccessible
# pragma warning(disable: 4673) // throwing 'x' the following types will not be considered at the catch site
# pragma warning(disable: 4244) // 'return' : conversion from '__w64 int' to 'unsigned long', possible loss of data
# pragma warning(disable: 4267) // 'argument' : conversion from 'size_t' to 'const unsigned int', possible loss of data
#endif


#include <xercesc/sax/ErrorHandler.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/sax/Locator.hpp>
#include <xercesc/sax/SAXParseException.hpp>

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>

#include <xercesc/util/BinInputStream.hpp>
#include <xercesc/util/XMLString.hpp>




XERCES_CPP_NAMESPACE_USE

using namespace DatafileIO;

//////////////////////////////////////////////////////////////////////////

// XMBFile functions:

static void DeallocateElement(XMLElement* el)
{
	size_t i;

	for (i = 0; i < el->childs.size(); ++i)
		DeallocateElement(el->childs[i]);

	delete el;
}

XMBFile::XMBFile()
: root(NULL)
{
}

XMBFile::~XMBFile()
{
	DeallocateElement(root);
}

// XMLReader init/shutdown:

DatafileIO::XMLReader::XMLReader()
{
	XMLPlatformUtils::Initialize();
}

DatafileIO::XMLReader::~XMLReader()
{
	XMLPlatformUtils::Terminate();
}

//////////////////////////////////////////////////////////////////////////

// Input utility functions:

#define READ(type, name) type name; stream.Read(&name, sizeof(type))
#define READARRAY(type, n, name) type name[n]; stream.Read(&name, sizeof(type)*n)
#define CHECK(expr, ret) if (!(expr)) { assert(!(expr)); return (ret); }

static void StringReplace(std::wstring& s, const std::wstring& pattern, const std::wstring& replacement)
{
	size_t pos = 0;
	while (pos != s.npos)
	{
		pos = s.find(pattern, pos);
		if (pos != s.npos)
		{
			s.replace(pos, pattern.length(), replacement);
			pos += replacement.length();
		}
	}
}

//////////////////////////////////////////////////////////////////////////

// XMB reading:

static XMLElement* ParseNode(InputStream& stream, XMBFile* file, const std::vector<utf16string>& ElementNames, const std::vector<utf16string>& AttributeNames)
{
	READARRAY(char, 2, Head); CHECK(strncmp(Head, "XN", 2) == 0, NULL);
	READ(int32_t, Length);

	XMLElement* node = new XMLElement;

	node->text = ReadUString(stream);

	READ(int32_t, Name);
	node->name = ElementNames[Name];

	if (file->format == XMBFile::AOE3)
	{
		READ(int32_t, LineNumber);
		node->linenum = LineNumber;
	}

	READ(int32_t, NumAttributes);
	node->attrs.reserve(NumAttributes);
	for (int32_t i = 0; i < NumAttributes; ++i)
	{
		READ(int32_t, AttrID);

		XMLAttribute attr;
		attr.name = AttributeNames[AttrID];
		attr.value = ReadUString(stream);
		node->attrs.push_back(attr);
	}

	READ(int32_t, NumChildren);
	node->childs.reserve(NumChildren);
	for (int32_t i = 0; i < NumChildren; ++i)
	{
		XMLElement* child = ParseNode(stream, file, ElementNames, AttributeNames);
		if (! child)
		{
			DeallocateElement(node);
			return NULL;
		}
		node->childs.push_back(child);
	}

	return node;
}



static bool ParseXMB(InputStream& stream, XMBFile* file)
{
	READARRAY(char, 2, Header); CHECK(strncmp(Header, "X1", 2) == 0, false);
	READ(int32_t, DataLength);
	READARRAY(char, 2, RootHeader); CHECK(strncmp(RootHeader, "XR", 2) == 0, false);
	READARRAY(int32_t, 2, Unknown);

	CHECK(Unknown[0] == 4, false);

	if (Unknown[1] == 7)
		file->format = XMBFile::AOM;
	else if (Unknown[1] == 8)
		file->format = XMBFile::AOE3;
	else
	{
//		wxLogWarning(_("Unrecognised XMB format - assuming AoE3"));
		assert(! "Unrecognised format");
		file->format = XMBFile::AOE3;
	}

	std::vector<utf16string> ElementNames;
	READ(int, NumElements);
	ElementNames.reserve(NumElements);
	for (int i = 0; i < NumElements; ++i)
		ElementNames.push_back(ReadUString(stream));

	std::vector<utf16string> AttributeNames;
	READ(int, NumAttributes);
	AttributeNames.reserve(NumAttributes);
	for (int i = 0; i < NumAttributes; ++i)
		AttributeNames.push_back(ReadUString(stream));

	XMLElement* root = ParseNode(stream, file, ElementNames, AttributeNames);
	if (! root)
		return false;

	file->root = root;
	return true;
}



XMBFile* XMBFile::LoadFromXMB(InputStream& stream)
{
	XMBFile* file = new XMBFile();

	if (! ParseXMB(stream, file))
	{
		delete file;
		return NULL;
	}

	return file;
}


//////////////////////////////////////////////////////////////////////////

// XMB writing:


typedef std::map<utf16string, uint32_t> StringTable;

static void ExtractStrings(XMLElement* node, StringTable& elements, StringTable& attributes)
{
	if (elements.find(node->name) == elements.end())
		elements.insert(std::make_pair(node->name, (uint32_t)elements.size()));

	for (size_t i = 0; i < node->attrs.size(); ++i)
		if (attributes.find(node->attrs[i].name) == attributes.end())
			attributes.insert(std::make_pair(node->attrs[i].name, (uint32_t)attributes.size()));

	for (size_t i = 0; i < node->childs.size(); ++i)
		ExtractStrings(node->childs[i], elements, attributes);
}


static void WriteNode(SeekableOutputStream& stream, XMBFile* file, XMLElement* node, const StringTable& elements, const StringTable& attributes)
{
	stream.Write("XN", 2);

	off_t Length_off = stream.Tell();
	stream.Write("????", 4);

	WriteUString(stream, node->text);

	uint32_t Name = elements.find(node->name)->second;
	stream.Write(&Name, 4);

	if (file->format == XMBFile::AOE3)
	{
		int32_t lineNum = node->linenum;
		stream.Write(&lineNum, 4);
	}

	uint32_t NumAttributes = (uint32_t)node->attrs.size();
	stream.Write(&NumAttributes, 4);
	for (uint32_t i = 0; i < NumAttributes; ++i)
	{
		uint32_t n = attributes.find(node->attrs[i].name)->second;
		stream.Write(&n, 4);
		WriteUString(stream, node->attrs[i].value);
	}

	uint32_t NumChildren = (uint32_t)node->childs.size();
	stream.Write(&NumChildren, 4);
	for (uint32_t i = 0; i < NumChildren; ++i)
		WriteNode(stream, file, node->childs[i], elements, attributes);

	off_t NodeEnd = stream.Tell();
	stream.Seek(Length_off, Stream::FROM_START);
	int Length = NodeEnd - (Length_off+4);
	stream.Write(&Length, 4);
	stream.Seek(NodeEnd, Stream::FROM_START);
}

void XMBFile::SaveAsXMB(SeekableOutputStream& stream)
{
	stream.Write("X1", 2);

	off_t Length_off = stream.Tell();
	stream.Write("????", 4);

	stream.Write("XR", 2);

	int version[2] = { 4, -1 };
	if (format == XMBFile::AOM)
		version[1] = 7;
	else if (format == XMBFile::AOE3)
		version[1] = 8;
	else
	{
//		wxLogWarning(_("Unknown XMB format - assuming AoE3"));
		format = XMBFile::AOE3;
		version[1] = 8;
	}
	stream.Write(version, 8);

	// Get the list of element/attribute names, sorted by first appearance
	StringTable ElementNames;
	StringTable AttributeNames;
	ExtractStrings(root, ElementNames, AttributeNames);


	// Convert into handy vector format for outputting
	std::vector<utf16string> ElementNamesByID;
	ElementNamesByID.resize(ElementNames.size());
	for (StringTable::iterator it = ElementNames.begin(); it != ElementNames.end(); ++it)
		ElementNamesByID[it->second] = it->first;

	// Output element names
	uint32_t NumElements = (uint32_t)ElementNamesByID.size();
	stream.Write(&NumElements, 4);
	for (uint32_t i = 0; i < NumElements; ++i)
		WriteUString(stream, ElementNamesByID[i]);


	// Convert into handy vector format for outputting
	std::vector<utf16string> AttributeNamesByID;
	AttributeNamesByID.resize(AttributeNames.size());
	for (StringTable::iterator it = AttributeNames.begin(); it != AttributeNames.end(); ++it)
		AttributeNamesByID[it->second] = it->first;

	// Output attribute names
	uint32_t NumAttributes = (uint32_t)AttributeNamesByID.size();
	stream.Write(&NumAttributes, 4);
	for (uint32_t i = 0; i < NumAttributes; ++i)
		WriteUString(stream, AttributeNamesByID[i]);

	// Output root node, plus all descendants
	WriteNode(stream, this, root, ElementNames, AttributeNames);

	// Fill in data-length field near the beginning
	off_t DataEnd = stream.Tell();
	stream.Seek(Length_off, Stream::FROM_START);
	int Length = DataEnd - (Length_off+4);
	stream.Write(&Length, 4);
	stream.Seek(DataEnd, Stream::FROM_START);
}

//////////////////////////////////////////////////////////////////////////

// XML writing:

const wchar_t* in = L"\t";
void OutputNode(std::wstring& output, const XMLElement* node, const std::wstring& indent)
{
	// Build up strings using lots of += instead of +, because it gives
	// a significant (>30%) performance increase.
	output += L"\n";
	output += indent;
	output += L"<";
	output += utf16tow(node->name);

	for (size_t i = 0; i < node->attrs.size(); ++i)
	{
		std::wstring value = utf16tow(node->attrs[i].value);
		// Escape funny characters
		if (value.find_first_of(L"&<>\"") != value.npos)
		{
			StringReplace(value, L"&", L"&amp;");
			StringReplace(value, L"<", L"&lt;");
			StringReplace(value, L">", L"&gt;");
			StringReplace(value, L"\"", L"&quot;");
		}

		output += L" ";
		output += utf16tow(node->attrs[i].name);
		output += L"=\"";
		output += value;
		output += L"\"";
	}

	// Output the element's contents, in an attemptedly nice readable format:

	std::wstring name = utf16tow(node->name);
	if (node->text.length())
	{
		std::wstring text = utf16tow(node->text);

		// Wrap text in CDATA when necessary. (Maybe we should use &lt; etc
		// in some cases?)
		if (text.find_first_of(L"<>&") != text.npos)
		{
			// (According to the XML spec, ">" is allowed in non-CDATA non-escaped
			// content, but we avoid that because it's confusing.)

			StringReplace(text, L"]]>", L"]]>]]&gt;<![CDATA["); // paranoia

			std::wstring newtext = L"<![CDATA[\n";
			newtext += indent;
			newtext += in;
			newtext += text;
			newtext += L"\n"; // (All this extra whitespace will be trimmed during XML->XMB)
			newtext += indent;
			newtext += L"]]>";
			std::swap(newtext, text);
		}

		if (node->childs.size())
		{
			/*	<el>
					Text
					<child>...
				</el>	*/
			output += L">\n";
			output += indent;
			output += in;
			output += text;
			std::wstring nextIndent = indent + in;
			for (size_t i = 0; i < node->childs.size(); ++i)
				OutputNode(output, node->childs[i], nextIndent);
			output += L"\n";
			output += indent;
			output += L"</";
			output += name;
			output += L">";
		}
		else
		{
			/*	<el>Text</el>	*/
			output += L">";
			output += text;
			output += L"</";
			output += name;
			output += L">";
		}
	}
	else
	{
		if (node->childs.size())
		{
			/*	<el>
					<child>...
				</el>	*/
			output += L">";
			std::wstring nextIndent = indent + in;
			for (size_t i = 0; i < node->childs.size(); ++i)
				OutputNode(output, node->childs[i], nextIndent);
			output += L"\n";
			output += indent;
			output += L"</";
			output += name;
			output += L">";
		}
		else
		{
			/*	<el/>	*/
			output += L"/>";
		}
	}
}

std::wstring XMBFile::SaveAsXML()
{
	std::wstring output;
	OutputNode(output, root, L"");
	return output;
}

//////////////////////////////////////////////////////////////////////////

// XML reading:

class XeroHandler : public DefaultHandler
{
public:
	XeroHandler() : Root(NULL), m_locator(NULL) {}
	~XeroHandler() { if (Root) DeallocateElement(Root); }

	// SAX2 event handlers:
	virtual void startDocument();
	virtual void endDocument();
	virtual void startElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname, const Attributes& attrs);
	virtual void endElement(const XMLCh* const uri, const XMLCh* const localname, const XMLCh* const qname);
	virtual void characters(const XMLCh* const chars, const unsigned int length);

	// Locator, for determining line numbers of elements
	const Locator* m_locator;
	virtual void setDocumentLocator(const Locator* const locator) {
		m_locator = locator;
	}

	// Return and remove the root element from this object (so it won't get deleted)
	XMLElement* ReleaseRoot() { XMLElement* t = Root; Root = NULL; return t; }

private:
	XMLElement* Root;
	std::stack<XMLElement*> ElementStack; // so the event handlers can be aware of the current element
};


class XeroErrorHandler : public ErrorHandler
{
public:
	XeroErrorHandler() : sawErrors(false) {}
	~XeroErrorHandler() {}
	void warning(const SAXParseException& err) { complain(err, L"warning"); }
	void error(const SAXParseException& err) { complain(err, L"error"); }
	void fatalError(const SAXParseException& err) { complain(err, L"fatal error"); };
	void resetErrors() { sawErrors = false; }
	bool getSawErrors() const { return sawErrors; }
	utf16string getErrorText() { return errorText; }
private:
	bool sawErrors;
	utf16string errorText;
	void complain(const SAXParseException& err, const wchar_t* severity) {
		sawErrors = true;
		C_ASSERT(sizeof(utf16_t) == sizeof(XMLCh));
		errorText += wtoutf16(L"XML ");
		errorText += wtoutf16(severity);
		errorText += wtoutf16(L": ");
		errorText += (utf16_t*)err.getSystemId();
		errorText += wtoutf16(L" / ");
		errorText += (utf16_t*)err.getMessage();
	}
};

class XeroBinInputStream : public BinInputStream
{
public:
	XeroBinInputStream(InputStream& stream) : m_stream(stream) {}
	virtual unsigned int curPos () const
	{
		return m_stream.Tell();
	}
	virtual unsigned int readBytes (XMLByte *const toFill, const unsigned int maxToRead)
	{
		return (unsigned int)m_stream.Read(toFill, (unsigned int)maxToRead);
	}
private:
	InputStream& m_stream;
};

class XeroInputSource : public InputSource
{
public:
	XeroInputSource(InputStream& stream) : m_stream(stream) {}
	virtual BinInputStream *makeStream() const
	{
		return new XeroBinInputStream(m_stream);
	}
private:
	InputStream& m_stream;
};


XMBFile* DatafileIO::XMLReader::LoadFromXML(InputStream& stream)
{
	XMBFile* ret = new XMBFile();

	// I don't like Xerces :-(

	SAX2XMLReader* Parser = XMLReaderFactory::createXMLReader();

	// DTD validation:
//	Parser->setFeature(XMLUni::fgSAX2CoreValidation, true);
//	Parser->setFeature(XMLUni::fgXercesDynamic, true);

	XeroHandler handler;
	Parser->setContentHandler(&handler);

	XeroErrorHandler errorHandler;
	Parser->setErrorHandler(&errorHandler);


	// Build the XMLElement tree inside the XeroHandler
	XeroInputSource src(stream);
	Parser->parse(src);

	delete Parser;

	if (errorHandler.getSawErrors())
	{
		//wxLogError(_("Error in XML file %s"), filename);
		// TODO URGENT: report errors
		delete ret;
		return NULL;
	}
	else
	{
		XMLElement* root = handler.ReleaseRoot();
		ret->root = root->childs[0];
		delete root;
	}

	return ret;
}


void XeroHandler::startDocument()
{
	Root = new XMLElement;
	ElementStack.push(Root);
}

void XeroHandler::endDocument()
{
}

void XeroHandler::startElement(const XMLCh* const /*uri*/, const XMLCh* const localname, const XMLCh* const /*qname*/, const Attributes& attrs)
{
	utf16string elementName = (utf16_t*)localname;

	// Create a new element
	XMLElement* e = new XMLElement;
	e->name = elementName;
	e->linenum = m_locator->getLineNumber();

	// Store all the attributes in the new element
	for (unsigned int i = 0; i < attrs.getLength(); ++i)
	{
		utf16string attrName = (utf16_t*)attrs.getLocalName(i);
		XMLAttribute attr;
		attr.name = attrName;
		attr.value = (utf16_t*)attrs.getValue(i);
		e->attrs.push_back(attr);
	}

	// Add the element to its parent
	ElementStack.top()->childs.push_back(e);

	// Set as parent of following elements
	ElementStack.push(e);
}

void XeroHandler::endElement(const XMLCh* const /*uri*/, const XMLCh* const /*localname*/, const XMLCh* const /*qname*/)
{
	// Trim whitespace around the element's contents

	std::string whitespaceA = " \t\r\n";
	utf16string whitespace (whitespaceA.begin(), whitespaceA.end());

	XMLElement* el = ElementStack.top();
	// Find the start of the non-whitespace section
	size_t first = el->text.find_first_not_of(whitespace);

	if (first == el->text.npos)
		// Entirely whitespace - easy to handle
		el->text = utf16string();

	else
	{
		// Count the number of \n being cut off,
		// and add them to the line number
//		utf16string trimmed (el->text.begin(), el->text.begin()+first);
//		el->linenum += (int)std::count(trimmed.begin(), trimmed.end(), (utf16_t)'\n');
	// (Only use this when the line number should point to the start of the
	// content, rather than the element tag. TODO: enable it in that situation.)

		// Find the end of the non-whitespace section,
		// and trim off everything else
		size_t last = el->text.find_last_not_of(whitespace);
		el->text = el->text.substr(first, 1+last-first);
	}

	ElementStack.pop();
}

void XeroHandler::characters(const XMLCh* const chars, const unsigned int length)
{
	ElementStack.top()->text += utf16string((utf16_t*)chars, length);
}
