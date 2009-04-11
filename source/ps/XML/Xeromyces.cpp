#include "precompiled.h"

#include <vector>
#include <set>
#include <map>
#include <stack>
#include <algorithm>

#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "Xeromyces.h"

#include <libxml/parser.h>

#define LOG_CATEGORY "xml"

static void errorHandler(void* UNUSED(userData), xmlErrorPtr error)
{
	LOG(CLogger::Error, LOG_CATEGORY, "CXeromyces: Parse %s: %s:%d: %s",
		error->level == XML_ERR_WARNING ? "warning" : "error",
		error->file, error->line, error->message);
	// TODO: The (non-fatal) warnings and errors don't get stored in the XMB,
	// so the caching is less transparent than it should be
}

static bool g_XeromycesStarted = false;
void CXeromyces::Startup()
{
	debug_assert(!g_XeromycesStarted);
	xmlInitParser();
	xmlSetStructuredErrorFunc(NULL, &errorHandler);
	g_XeromycesStarted = true;
}

void CXeromyces::Terminate()
{
	debug_assert(g_XeromycesStarted);
	xmlCleanupParser();
	xmlSetStructuredErrorFunc(NULL, NULL);
	g_XeromycesStarted = false;
}


// Find out write location of the XMB file corresponding to xmlFilename
void CXeromyces::GetXMBPath(const PIVFS& vfs, const VfsPath& xmlFilename, const VfsPath& xmbFilename, VfsPath& xmbActualPath)
{
	// rationale:
	// - it is necessary to write out XMB files into a subdirectory
	//   corresponding to the mod from which the XML file is taken.
	//   this avoids confusion when multiple mods are active -
	//   their XMB files' VFS filename would otherwise be indistinguishable.
	// - we group files in the cache/ mount point first by mod, and only
	//   then XMB. this is so that all output files for a given mod can
	//   easily be deleted. the operation of deleting all old/unused
	//   XMB files requires a program anyway (to find out which are no
	//   longer needed), so it's not a problem that XMB files reside in
	//   a subdirectory (which would make manually deleting all harder).

	// get real path of XML file (e.g. mods/official/entities/...)
	Path P_XMBRealPath;
	vfs->GetRealPath(xmlFilename, P_XMBRealPath);

	// extract mod name from that
	char modName[PATH_MAX];
	// .. NOTE: can't use %s, of course (keeps going beyond '/')
	int matches = sscanf(P_XMBRealPath.string().c_str(), "mods/%[^/]", modName);
	debug_assert(matches == 1);

	// build full name: cache, then mod name, XMB subdir, original XMB path
	xmbActualPath = VfsPath("cache/mods") / modName / "xmb" / xmbFilename;
}

PSRETURN CXeromyces::Load(const VfsPath& filename)
{
	debug_assert(g_XeromycesStarted);

	// Make sure the .xml actually exists
	if (! FileExists(filename))
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CXeromyces: Failed to find XML file %s", filename.string().c_str());
		return PSRETURN_Xeromyces_XMLOpenFailed;
	}

	// Get some data about the .xml file
	FileInfo fileInfo;
	if (g_VFS->GetFileInfo(filename, &fileInfo) < 0)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CXeromyces: Failed to stat XML file %s", filename.string().c_str());
		return PSRETURN_Xeromyces_XMLOpenFailed;
	}


	/*
	XMBs are stored with a unique name, where the name is generated from
	characteristics of the XML file. If a file already exists with the
	generated name, it is assumed that that file is a valid conversion of
	the XML, and so it's loaded. Otherwise, the XMB is created with that
	filename.

	This means it's never necessary to overwrite existing XMB files; since
	the XMBs are often in archives, it's not easy to rewrite those files,
	and it's not possible to switch to using a loose file because the VFS
	has already decided that file is inside an archive. So each XMB is given
	a unique name, and old ones are somehow purged.
	*/


	// Generate the filename for the xmb:
	//     <xml filename>_<mtime><size><format version>.xmb
	// with mtime/size as 8-digit hex, where mtime's lowest bit is
	// zeroed because zip files only have 2 second resolution.
	const int suffixLength = 22;
	char suffix[suffixLength+1];
	int printed = sprintf(suffix, "_%08x%08xB.xmb", (int)(fileInfo.MTime() & ~1), (int)fileInfo.Size());
	debug_assert(printed == suffixLength);
	VfsPath xmbFilename = change_extension(filename, suffix);

	VfsPath xmbPath;
	GetXMBPath(g_VFS, filename, xmbFilename, xmbPath);

	// If the file exists, use it
	if (FileExists(xmbPath))
	{
		if (ReadXMBFile(xmbPath))
			return PSRETURN_OK;
		// (no longer return PSRETURN_Xeromyces_XMLOpenFailed here because
		// failure legitimately happens due to partially-written XMB files.)
	}

	
	// XMB isn't up to date with the XML, so rebuild it:

	CVFSFile input;
	if (input.Load(filename))
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CXeromyces: Failed to open XML file %s", filename.string().c_str());
		return PSRETURN_Xeromyces_XMLOpenFailed;
	}

	xmlDocPtr doc = xmlReadMemory((const char*)input.GetBuffer(), (int)input.GetBufferSize(),
		filename.string().c_str(), NULL, XML_PARSE_NONET|XML_PARSE_NOCDATA);
	if (! doc)
	{
		LOG(CLogger::Error, LOG_CATEGORY, "CXeromyces: Failed to parse XML file %s", filename.string().c_str());
		return PSRETURN_Xeromyces_XMLParseError;
	}

	WriteBuffer writeBuffer;
	CreateXMB(doc, writeBuffer);

	xmlFreeDoc(doc);

	// Save the file to disk, so it can be loaded quickly next time
	g_VFS->CreateFile(xmbPath, writeBuffer.Data(), writeBuffer.Size());

	m_XMBBuffer = writeBuffer.Data(); // add a reference

	// Set up the XMBFile
	const bool ok = Initialise((const char*)m_XMBBuffer.get());
	debug_assert(ok);

	return PSRETURN_OK;
}

bool CXeromyces::ReadXMBFile(const VfsPath& filename)
{
	size_t size;
	if(g_VFS->LoadFile(filename, m_XMBBuffer, size) < 0)
		return false;
	debug_assert(size >= 4); // make sure it's at least got the initial header

	// Set up the XMBFile
	if(!Initialise((const char*)m_XMBBuffer.get()))
		return false;

	return true;
}



static void FindNames(const xmlNodePtr node, std::set<std::string>& elementNames, std::set<std::string>& attributeNames)
{
	elementNames.insert((const char*)node->name);

	for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
		attributeNames.insert((const char*)attr->name);

	for (xmlNodePtr child = node->children; child; child = child->next)
		if (child->type == XML_ELEMENT_NODE)
			FindNames(child, elementNames, attributeNames);
}

static void OutputElement(const xmlNodePtr node, WriteBuffer& writeBuffer,
	std::map<std::string, u32>& elementIDs,
	std::map<std::string, u32>& attributeIDs
)
{
	// Filled in later with the length of the element
	size_t posLength = writeBuffer.Size();
	writeBuffer.Append("????", 4);

	writeBuffer.Append(&elementIDs[(const char*)node->name], 4);

	u32 attrCount = 0;
	for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
		++attrCount;
	writeBuffer.Append(&attrCount, 4);

	u32 childCount = 0;
	for (xmlNodePtr child = node->children; child; child = child->next)
		if (child->type == XML_ELEMENT_NODE)
			++childCount;
	writeBuffer.Append(&childCount, 4);

	// Filled in later with the offset to the list of child elements
	size_t posChildrenOffset = writeBuffer.Size();
	writeBuffer.Append("????", 4);


	// Trim excess whitespace in the entity's text, while counting
	// the number of newlines trimmed (so that JS error reporting
	// can give the correct line number within the script)

	std::string whitespace = " \t\r\n";
	std::string text;
	for (xmlNodePtr child = node->children; child; child = child->next)
	{
		if (child->type == XML_TEXT_NODE)
		{
			xmlChar* content = xmlNodeGetContent(child);
			text += std::string((const char*)content);
			xmlFree(content);
		}
	}

	u32 linenum = xmlGetLineNo(node);

	// Find the start of the non-whitespace section
	size_t first = text.find_first_not_of(whitespace);

	if (first == text.npos)
		// Entirely whitespace - easy to handle
		text = "";

	else
	{
		// Count the number of \n being cut off,
		// and add them to the line number
		std::string trimmed (text.begin(), text.begin()+first);
		linenum += std::count(trimmed.begin(), trimmed.end(), '\n');

		// Find the end of the non-whitespace section,
		// and trim off everything else
		size_t last = text.find_last_not_of(whitespace);
		text = text.substr(first, 1+last-first);
	}


	// Output text, prefixed by length in bytes
	if (text.length() == 0)
	{
		// No text; don't write much
		writeBuffer.Append("\0\0\0\0", 4);
	}
	else
	{
		// Write length and line number and null-terminated text
		utf16string textW = CStr8(text).FromUTF8().utf16();
		u32 nodeLen = u32(4 + 2*(textW.length()+1));
		writeBuffer.Append(&nodeLen, 4);
		writeBuffer.Append(&linenum, 4);
		writeBuffer.Append((void*)textW.c_str(), nodeLen-4);
	}

	// Output attributes
	for (xmlAttrPtr attr = node->properties; attr; attr = attr->next)
	{
		writeBuffer.Append(&attributeIDs[(const char*)attr->name], 4);

		xmlChar* value = xmlNodeGetContent(attr->children);
		utf16string textW = CStr8((const char*)value).FromUTF8().utf16();
		xmlFree(value);
		u32 attrLen = u32(2*(textW.length()+1));
		writeBuffer.Append(&attrLen, 4);
		writeBuffer.Append((void*)textW.c_str(), attrLen);
	}

	// Go back and fill in the child-element offset
	u32 childrenOffset = (u32)(writeBuffer.Size() - (posChildrenOffset+4));
	writeBuffer.Overwrite(&childrenOffset, 4, posChildrenOffset);

	// Output all child elements
	for (xmlNodePtr child = node->children; child; child = child->next)
		if (child->type == XML_ELEMENT_NODE)
			OutputElement(child, writeBuffer, elementIDs, attributeIDs);

	// Go back and fill in the length
	u32 length = (u32)(writeBuffer.Size() - posLength);
	writeBuffer.Overwrite(&length, 4, posLength);
}

PSRETURN CXeromyces::CreateXMB(const xmlDocPtr doc, WriteBuffer& writeBuffer)
{
	// Header
	writeBuffer.Append(UnfinishedHeaderMagicStr, 4);

	std::set<std::string>::iterator it;
	u32 i;

	// Find the unique element/attribute names
	std::set<std::string> elementNames;
	std::set<std::string> attributeNames;
	FindNames(xmlDocGetRootElement(doc), elementNames, attributeNames);

	std::map<std::string, u32> elementIDs;
	std::map<std::string, u32> attributeIDs;

	// Output element names
	i = 0;
	u32 elementCount = (u32)elementNames.size();
	writeBuffer.Append(&elementCount, 4);
	for (it = elementNames.begin(); it != elementNames.end(); ++it)
	{
		u32 textLen = (u32)it->length()+1;
		writeBuffer.Append(&textLen, 4);
		writeBuffer.Append((void*)it->c_str(), textLen);
		elementIDs[*it] = i++;
	}

	// Output attribute names
	i = 0;
	u32 attributeCount = (u32)attributeNames.size();
	writeBuffer.Append(&attributeCount, 4);
	for (it = attributeNames.begin(); it != attributeNames.end(); ++it)
	{
		u32 textLen = (u32)it->length()+1;
		writeBuffer.Append(&textLen, 4);
		writeBuffer.Append((void*)it->c_str(), textLen);
		attributeIDs[*it] = i++;
	}

	OutputElement(xmlDocGetRootElement(doc), writeBuffer, elementIDs, attributeIDs);

	// file is now valid, so insert correct magic string
	writeBuffer.Overwrite(HeaderMagicStr, 4, 0);

	return PSRETURN_OK;
}
