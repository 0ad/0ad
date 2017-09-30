/* Copyright (C) 2017 Wildfire Games.
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

bool CTemplateLoader::LoadTemplateFile(const std::string& templateName, int depth)
{
	// If this file was already loaded, we don't need to do anything
	if (m_TemplateFileData.find(templateName) != m_TemplateFileData.end())
		return true;

	// Handle infinite loops more gracefully than running out of stack space and crashing
	if (depth > 100)
	{
		LOGERROR("Probable infinite inheritance loop in entity template '%s'", templateName.c_str());
		return false;
	}

	// Handle special case "actor|foo"
	if (templateName.find("actor|") == 0)
	{
		ConstructTemplateActor(templateName.substr(6), m_TemplateFileData[templateName]);
		return true;
	}

	// Handle special case "bar|foo"
	size_t pos = templateName.find_first_of('|');
	if (pos != std::string::npos)
	{
		std::string prefix = templateName.substr(0, pos);
		std::string baseName = templateName.substr(pos+1);

		if (!LoadTemplateFile(baseName, depth+1))
		{
			LOGERROR("Failed to load entity template '%s'", baseName.c_str());
			return false;
		}

		VfsPath path = VfsPath(TEMPLATE_ROOT) / L"special" / L"filter" / wstring_from_utf8(prefix + ".xml");
		if (!VfsFileExists(path))
		{
			LOGERROR("Invalid subset '%s'", prefix.c_str());
			return false;
		}

		CXeromyces xero;
		PSRETURN ok = xero.Load(g_VFS, path);
		if (ok != PSRETURN_OK)
			return false; // (Xeromyces already logged an error with the full filename)

		m_TemplateFileData[templateName] = m_TemplateFileData[baseName];
		CParamNode::LoadXML(m_TemplateFileData[templateName], xero, path.string().c_str());
		return true;
	}

	// Normal case: templateName is an XML file:

	VfsPath path = VfsPath(TEMPLATE_ROOT) / wstring_from_utf8(templateName + ".xml");
	CXeromyces xero;
	PSRETURN ok = xero.Load(g_VFS, path);
	if (ok != PSRETURN_OK)
		return false; // (Xeromyces already logged an error with the full filename)

	int attr_parent = xero.GetAttributeID("parent");
	CStr parentName = xero.GetRoot().GetAttributes().GetNamedItem(attr_parent);
	if (!parentName.empty())
	{
		// To prevent needless complexity in template design, we don't allow |-separated strings as parents
		if (parentName.find('|') != parentName.npos)
		{
			LOGERROR("Invalid parent '%s' in entity template '%s'", parentName.c_str(), templateName.c_str());
			return false;
		}

		// Ensure the parent is loaded
		if (!LoadTemplateFile(parentName, depth+1))
		{
			LOGERROR("Failed to load parent '%s' of entity template '%s'", parentName.c_str(), templateName.c_str());
			return false;
		}

		CParamNode& parentData = m_TemplateFileData[parentName];

		// Initialise this template with its parent
		m_TemplateFileData[templateName] = parentData;
	}

	// Load the new file into the template data (overriding parent values)
	CParamNode::LoadXML(m_TemplateFileData[templateName], xero, wstring_from_utf8(templateName).c_str());

	return true;
}

static Status AddToTemplates(const VfsPath& pathname, const CFileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	std::vector<std::string>& templates = *(std::vector<std::string>*)cbData;

	// Strip the .xml extension
	VfsPath pathstem = pathname.ChangeExtension(L"");
	// Strip the root from the path
	std::wstring name = pathstem.string().substr(ARRAY_SIZE(TEMPLATE_ROOT)-1);

	// We want to ignore template_*.xml templates, since they should never be built in the editor
	if (name.substr(0, 9) == L"template_")
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

std::vector<std::string> CTemplateLoader::FindPlaceableTemplates(const std::string& path, bool includeSubdirectories, ETemplatesType templatesType, ScriptInterface& scriptInterface) const
{
	if (templatesType != SIMULATION_TEMPLATES && templatesType != ACTOR_TEMPLATES && templatesType != ALL_TEMPLATES)
	{
		LOGERROR("Undefined template type (valid: all, simulation, actor)");
		return std::vector<std::string>();
	}

	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);

	std::vector<std::string> templates;
	Status ok;
	VfsPath templatePath;

	if (templatesType == SIMULATION_TEMPLATES || templatesType == ALL_TEMPLATES)
	{
		JS::RootedValue placeablesFilter(cx);
		scriptInterface.ReadJSONFile("simulation/data/placeablesFilter.json", &placeablesFilter);

		JS::RootedObject folders(cx);
		if (scriptInterface.GetProperty(placeablesFilter, "templates", &folders))
		{
			if (!(JS_IsArrayObject(cx, folders)))
			{
				LOGERROR("FindPlaceableTemplates: Argument must be an array!");
				return templates;
			}

			u32 length;
			if (!JS_GetArrayLength(cx, folders, &length))
			{
				LOGERROR("FindPlaceableTemplates: Failed to get array length!");
				return templates;
			}

			templatePath = VfsPath(TEMPLATE_ROOT) / path;
			//I have every object inside, just run for each
			for (u32 i=0; i<length; ++i)
			{
				JS::RootedValue val(cx);
				if (!JS_GetElement(cx, folders, i, &val))
				{
					LOGERROR("FindPlaceableTemplates: Failed to read array element!");
					return templates;
				}

				std::string directoryPath;
				std::wstring fileFilter;
				scriptInterface.GetProperty(val, "directory", directoryPath);
				scriptInterface.GetProperty(val, "file", fileFilter);

				VfsPaths filenames;
				if (vfs::GetPathnames(g_VFS, templatePath / (directoryPath + "/"), fileFilter.c_str(), filenames) != INFO::OK)
					continue;

				for (const VfsPath& filename : filenames)
				{
					// Strip the .xml extension
					VfsPath pathstem = filename.ChangeExtension(L"");
					// Strip the root from the path
					std::wstring name = pathstem.string().substr(ARRAY_SIZE(TEMPLATE_ROOT) - 1);

					templates.emplace_back(name.begin(), name.end());
				}

			}
		}
	}

	if (templatesType == ACTOR_TEMPLATES || templatesType == ALL_TEMPLATES)
	{
		templatePath = VfsPath(ACTOR_ROOT) / path;
		if (includeSubdirectories)
			ok = vfs::ForEachFile(g_VFS, templatePath, AddActorToTemplates, (uintptr_t)&templates, L"*.xml", vfs::DIR_RECURSIVE);
		else
			ok = vfs::ForEachFile(g_VFS, templatePath, AddActorToTemplates, (uintptr_t)&templates, L"*.xml");
		WARN_IF_ERR(ok);
	}

	return templates;
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
	// Load the template if necessary
	if (!LoadTemplateFile(templateName, 0))
	{
		LOGERROR("Failed to load entity template '%s'", templateName.c_str());
		return NULL_NODE;
	}

	return m_TemplateFileData[templateName];
}

void CTemplateLoader::ConstructTemplateActor(const std::string& actorName, CParamNode& out)
{
	// Copy the actor template
	out = GetTemplateFileData("special/actor");

	// Initialise the actor's name and make it an Atlas selectable entity.
	std::wstring actorNameW = wstring_from_utf8(actorName);
	std::string name = utf8_from_wstring(CParamNode::EscapeXMLString(actorNameW));
	std::string xml = "<Entity>"
	                      "<VisualActor><Actor>" + name + "</Actor><ActorOnly/></VisualActor>"
						  // arbitrary-sized Footprint definition to make actors' selection outlines show up in Atlas
						  "<Footprint><Circle radius='2.0'/><Height>1.0</Height></Footprint>"
	                      "<Selectable>"
	                          "<EditorOnly/>"
	                          "<Overlay><Texture><MainTexture>actor.png</MainTexture><MainTextureMask>actor_mask.png</MainTextureMask></Texture></Overlay>"
	                      "</Selectable>"
	                  "</Entity>";

	CParamNode::LoadXMLString(out, xml.c_str(), actorNameW.c_str());
}
