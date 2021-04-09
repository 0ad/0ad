/* Copyright (C) 2021 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "precompiled.h"

#include "TemplateLoader.h"

#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"

static const wchar_t TEMPLATE_ROOT[] = L"simulation/templates/";
static const wchar_t ACTOR_ROOT[] = L"art/actors/";

static CParamNode NULL_NODE(false);

bool CTemplateLoader::LoadTemplateFile(CParamNode& node, std::string_view templateName, bool compositing, int depth)
{
	// Handle special case "actor|foo", which does not load 'foo' at all, just uses the name.
	if (templateName.compare(0, 6, "actor|") == 0)
	{
		ConstructTemplateActor(templateName.substr(6), node);
		return true;
	}
	// Handle infinite loops more gracefully than running out of stack space and crashing
	if (depth > 100)
	{
		LOGERROR("Probable infinite inheritance loop in entity template '%s'", std::string(templateName));
		return false;
	}

	size_t pos = templateName.find_first_of('|');
	if (pos != std::string::npos)
	{
		// 'foo|bar' pattern: 'bar' is treated as the parent of 'foo'.
		if (!LoadTemplateFile(node, templateName.substr(pos + 1), false, depth + 1))
			return false;
		if (!LoadTemplateFile(node, templateName.substr(0, pos), true, depth + 1))
			return false;
		return true;
	}

	// Load the data we need to apply on the node. This data may contain special modifiers,
	// such as filters, merges, multiplying the parent values, etc. Applying it to paramnode is destructive.
	// Find the XML file to load - by default, this assumes the files reside in 'special/filter'.
	// If not found there, it will be searched for in 'mixins/', then from the root.
	// The reason for this order is that filters are used at runtime, mixins at load time.
	std::wstring wtempName = wstring_from_utf8(std::string(templateName) + ".xml");
	VfsPath path = VfsPath(TEMPLATE_ROOT) / L"special" / L"filter" / wtempName;
	if (!VfsFileExists(path))
		path = VfsPath(TEMPLATE_ROOT) / L"mixins" / wtempName;
	if (!VfsFileExists(path))
		path = VfsPath(TEMPLATE_ROOT) / wtempName;

	CXeromyces xero;
	PSRETURN ok = xero.Load(g_VFS, path);
	if (ok != PSRETURN_OK)
		return false; // (Xeromyces already logged an error with the full filename)

	// If the layer defines an explicit parent, we must load that and apply it before ourselves.
	int attr_parent = xero.GetAttributeID("parent");
	CStr parentName = xero.GetRoot().GetAttributes().GetNamedItem(attr_parent);
	if (!parentName.empty() && !LoadTemplateFile(node, parentName, compositing, depth + 1))
		return false;

	// Load the new file into the template data (overriding parent values).
	// TODO: error handling.
	CParamNode::LoadXML(node, xero);
	return true;
}

static Status AddToTemplates(const VfsPath& pathname, const CFileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	std::vector<std::string>& templates = *(std::vector<std::string>*)cbData;

	// Strip the .xml extension
	VfsPath pathstem = pathname.ChangeExtension(L"");
	// Strip the root from the path
	std::wstring_view name = pathstem.string().substr(ARRAY_SIZE(TEMPLATE_ROOT)-1);

	// We want to ignore template_*.xml templates, since they should never be built in the editor
	if (name.substr(0, 9) == L"template_")
		return INFO::OK;

	// Also ignore some subfolders.
	if (name.substr(0, 8) == L"special/" || name.substr(0, 7) == L"mixins/")
		return INFO::OK;

	templates.push_back(std::string(name.begin(), name.end()));
	return INFO::OK;
}

static Status AddActorToTemplates(const VfsPath& pathname, const CFileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	std::vector<std::string>& templates = *(std::vector<std::string>*)cbData;

	// Strip the root from the path
	std::wstring name = pathname.string().substr(ARRAY_SIZE(ACTOR_ROOT)-1);

	templates.push_back("actor|" + std::string(name.begin(), name.end()));
	return INFO::OK;
}

bool CTemplateLoader::TemplateExists(const std::string& templateName) const
{
	size_t pos = templateName.rfind('|');
	std::string baseName(pos != std::string::npos ? templateName.substr(pos+1) : templateName);
	return VfsFileExists(VfsPath(TEMPLATE_ROOT) / wstring_from_utf8(baseName + ".xml"));
}

std::vector<std::string> CTemplateLoader::FindTemplates(const std::string& path, bool includeSubdirectories, ETemplatesType templatesType) const
{
	std::vector<std::string> templates;

	if (templatesType != SIMULATION_TEMPLATES && templatesType != ACTOR_TEMPLATES && templatesType != ALL_TEMPLATES)
	{
		LOGERROR("Undefined template type (valid: all, simulation, actor)");
		return templates;
	}

	size_t flags = includeSubdirectories ? vfs::DIR_RECURSIVE : 0;

	if (templatesType == SIMULATION_TEMPLATES || templatesType == ALL_TEMPLATES)
		WARN_IF_ERR(vfs::ForEachFile(g_VFS, VfsPath(TEMPLATE_ROOT) / path, AddToTemplates, (uintptr_t)&templates, L"*.xml", flags));

	if (templatesType == ACTOR_TEMPLATES || templatesType == ALL_TEMPLATES)
		WARN_IF_ERR(vfs::ForEachFile(g_VFS, VfsPath(ACTOR_ROOT) / path, AddActorToTemplates, (uintptr_t)&templates, L"*.xml", flags));

	return templates;
}

const CParamNode& CTemplateLoader::GetTemplateFileData(const std::string& templateName)
{
	if (std::unordered_map<std::string, CParamNode>::const_iterator it = m_TemplateFileData.find(templateName); it != m_TemplateFileData.end())
		return it->second;

	CParamNode ret;
	if (!LoadTemplateFile(ret, templateName, false, 0))
	{
		LOGERROR("Failed to load entity template '%s'", templateName.c_str());
		return NULL_NODE;
	}
	return m_TemplateFileData.insert_or_assign(templateName, ret).first->second;
}

void CTemplateLoader::ConstructTemplateActor(std::string_view actorName, CParamNode& out)
{
	// Copy the actor template
	out = GetTemplateFileData("special/actor");

	// Initialize the actor's name and make it an Atlas selectable entity.
	std::string source(actorName);
	std::wstring actorNameW = wstring_from_utf8(source);
	source = "<Entity>"
	           "<VisualActor><Actor>" + source + "</Actor><ActorOnly/></VisualActor>"
               // Arbitrary-sized Footprint definition to make actors' selection outlines show up in Atlas.
	           "<Footprint><Circle radius='2.0'/><Height>1.0</Height></Footprint>"
	             "<Selectable>"
	               "<EditorOnly/>"
	               "<Overlay><Texture><MainTexture>128x128/ellipse.png</MainTexture><MainTextureMask>128x128/ellipse_mask.png</MainTextureMask></Texture></Overlay>"
	             "</Selectable>"
	           "</Entity>";
	// We'll assume that actorName is valid XML, otherwise this will fail and report the error anyways.
	CParamNode::LoadXMLString(out, source.c_str(), actorNameW.c_str());
}
