/* Copyright (C) 2010 Wildfire Games.
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

#include "simulation2/system/Component.h"
#include "ICmpTemplateManager.h"

#include "lib/wchar.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"

static const wchar_t TEMPLATE_ROOT[] = L"simulation/templates/";
static const wchar_t ACTOR_ROOT[] = L"art/actors/";

class CCmpTemplateManager : public ICmpTemplateManager
{
public:
	static void ClassInit(CComponentManager& UNUSED(componentManager))
	{
	}

	DEFAULT_COMPONENT_ALLOCATOR(TemplateManager)

	virtual void Init(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode))
	{
	}

	virtual void Deinit(const CSimContext& UNUSED(context))
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		// TODO: refactor the common bits of this? also need nicer debug output
		serialize.NumberU32_Unbounded("num entities", (u32)m_LatestTemplates.size());
		std::map<entity_id_t, std::wstring>::const_iterator it = m_LatestTemplates.begin();
		for (; it != m_LatestTemplates.end(); ++it)
		{
			serialize.NumberU32_Unbounded("id", it->first);
			serialize.String("template", it->second, 0, 256);
		}

		// TODO: will need to serialize techs too, because we need to be giving out
		// template data before other components (like the tech components) have been deserialized
	}

	virtual void Deserialize(const CSimContext& UNUSED(context), const CParamNode& UNUSED(paramNode), IDeserializer& deserialize)
	{
		u32 numEntities;
		deserialize.NumberU32_Unbounded(numEntities);
		for (u32 i = 0; i < numEntities; ++i)
		{
			entity_id_t ent;
			std::wstring templateName;
			deserialize.NumberU32_Unbounded(ent);
			deserialize.String(templateName, 0, 256);
			m_LatestTemplates[ent] = templateName;
		}
	}

	virtual void HandleMessage(const CSimContext& UNUSED(context), const CMessage& UNUSED(msg), bool UNUSED(global))
	{
		// TODO: should listen to entity destruction messages, to clean up m_LatestTemplates
	}

	virtual const CParamNode* LoadTemplate(entity_id_t ent, const std::wstring& templateName, int playerID);

	virtual const CParamNode* LoadLatestTemplate(entity_id_t ent);

	virtual std::wstring GetCurrentTemplateName(entity_id_t ent);

	virtual std::vector<std::wstring> FindAllTemplates();

private:
	// Map from template name (XML filename or special |-separated string) to the most recently
	// loaded valid template data.
	// (Failed loads won't remove valid entries under the same name, so we behave more nicely
	// when hotloading broken files)
	std::map<std::wstring, CParamNode> m_TemplateFileData;

	// Remember the template used by each entity, so we can return them
	// again for deserialization.
	// TODO: should store player ID etc.
	std::map<entity_id_t, std::wstring> m_LatestTemplates;

	// (Re)loads the given template, regardless of whether it exists already,
	// and saves into m_TemplateFileData. Also loads any parents that are not yet
	// loaded. Returns false on error.
	// @param templateName XML filename to load (not a |-separated string)
	bool LoadTemplateFile(const std::wstring& templateName, int depth);

	// Constructs a standard static-decorative-object template for the given actor
	void ConstructTemplateActor(const std::wstring& actorName, CParamNode& out);

	// Copy the non-interactive components of an entity template (position, actor, etc) into
	// a new entity template
	void CopyPreviewSubset(CParamNode& out, const CParamNode& in);
};

REGISTER_COMPONENT_TYPE(TemplateManager)

const CParamNode* CCmpTemplateManager::LoadTemplate(entity_id_t ent, const std::wstring& templateName, int UNUSED(playerID))
{
	m_LatestTemplates[ent] = templateName;

	// Load the template if necessary
	if (!LoadTemplateFile(templateName, 0))
	{
		LOGERROR(L"Failed to load entity template '%ls'", templateName.c_str());
		return NULL;
	}

	// TODO: Eventually we need to support techs in here, and return a different template per playerID

	const CParamNode* templateRoot = m_TemplateFileData[templateName].GetChild("Entity");
	if (!templateRoot)
	{
		LOGERROR(L"Invalid root element in entity template '%ls'", templateName.c_str());
		return NULL;
	}
	// TODO: the template ought to be validated with some schema, so we don't
	// need to nicely report errors like invalid root elements here

	return templateRoot;
}

const CParamNode* CCmpTemplateManager::LoadLatestTemplate(entity_id_t ent)
{
	std::map<entity_id_t, std::wstring>::const_iterator it = m_LatestTemplates.find(ent);
	if (it == m_LatestTemplates.end())
		return NULL;
	return LoadTemplate(ent, it->second, -1);
}

std::wstring CCmpTemplateManager::GetCurrentTemplateName(entity_id_t ent)
{
	std::map<entity_id_t, std::wstring>::const_iterator it = m_LatestTemplates.find(ent);
	if (it == m_LatestTemplates.end())
		return L"";
	return it->second;
}

bool CCmpTemplateManager::LoadTemplateFile(const std::wstring& templateName, int depth)
{
	// If this file was already loaded, we don't need to do anything
	if (m_TemplateFileData.find(templateName) != m_TemplateFileData.end())
		return true;

	// Handle infinite loops more gracefully than running out of stack space and crashing
	if (depth > 100)
	{
		LOGERROR(L"Probable infinite inheritance loop in entity template '%ls'", templateName.c_str());
		return false;
	}

	// Handle special case "actor|foo"
	if (templateName.find(L"actor|") == 0)
	{
		ConstructTemplateActor(templateName.substr(6), m_TemplateFileData[templateName]);
		return true;
	}

	// Handle special case "preview|foo"
	if (templateName.find(L"preview|") == 0)
	{
		// Load the base entity template, if it wasn't already loaded
		std::wstring baseName = templateName.substr(8);
		if (!LoadTemplateFile(baseName, depth+1))
		{
			LOGERROR(L"Failed to load entity template '%ls'", baseName.c_str());
			return NULL;
		}
		// Copy a subset to the requested template
		CopyPreviewSubset(m_TemplateFileData[templateName], m_TemplateFileData[baseName]);
		return true;
	}

	// Normal case: templateName is an XML file:

	VfsPath path = VfsPath(TEMPLATE_ROOT) / (templateName + L".xml");
	CXeromyces xero;
	PSRETURN ok = xero.Load(path);
	if (ok != PSRETURN_OK)
		return false; // (Xeromyces already logged an error with the full filename)

	int attr_parent = xero.GetAttributeID("parent");
	utf16string parentStr = xero.GetRoot().GetAttributes().GetNamedItem(attr_parent);
	if (!parentStr.empty())
	{
		std::wstring parentName(parentStr.begin(), parentStr.end());

		// To prevent needless complexity in template design, we don't allow |-separated strings as parents
		if (parentName.find('|') != parentName.npos)
		{
			LOGERROR(L"Invalid parent '%ls' in entity template '%ls'", parentName.c_str(), templateName.c_str());
			return false;
		}

		// Ensure the parent is loaded
		if (!LoadTemplateFile(parentName, depth+1))
		{
			LOGERROR(L"Failed to load parent '%ls' of entity template '%ls'", parentName.c_str(), templateName.c_str());
			return false;
		}

		CParamNode& parentData = m_TemplateFileData[parentName];

		// Initialise this template with its parent
		m_TemplateFileData[templateName] = parentData;
	}

	// Load the new file into the template data (overriding parent values)
	CParamNode::LoadXML(m_TemplateFileData[templateName], xero);

	return true;
}

void CCmpTemplateManager::ConstructTemplateActor(const std::wstring& actorName, CParamNode& out)
{
	std::string name = utf8_from_wstring(CParamNode::EscapeXMLString(actorName));
	std::string xml = "<?xml version='1.0' encoding='utf-8'?>"
		"<Entity>"
		"<Position>"
			"<Anchor>upright</Anchor>"
			"<Altitude>0</Altitude>"
			"<Floating>false</Floating>"
		"</Position>"
		"<MotionBallScripted/>"
		"<VisualActor>"
			"<Actor>" + name + "</Actor>"
		"</VisualActor>"
		"</Entity>";

	out.LoadXMLString(out, xml.c_str());
}

static LibError AddToTemplates(const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	std::vector<std::wstring>& templates = *(std::vector<std::wstring>*)cbData;

	// Strip the .xml extension
	VfsPath pathstem = change_extension(pathname, L"");
	// Strip the root from the path
	std::wstring name = pathstem.string().substr(ARRAY_SIZE(TEMPLATE_ROOT)-1);

	// We want to ignore template_*.xml templates, since they should never be built in the editor
	if (name.substr(0, 9) == L"template_")
		return INFO::OK;

	templates.push_back(name);
	return INFO::OK;
}

static LibError AddActorToTemplates(const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	std::vector<std::wstring>& templates = *(std::vector<std::wstring>*)cbData;

	// Strip the root from the path
	std::wstring name = pathname.string().substr(ARRAY_SIZE(ACTOR_ROOT)-1);

	templates.push_back(L"actor|" + name);
	return INFO::OK;
}

std::vector<std::wstring> CCmpTemplateManager::FindAllTemplates()
{
	// TODO: eventually this should probably read all the template files and look for flags to
	// determine which should be displayed in the editor (and in what categories etc); for now we'll
	// just return all the files

	std::vector<std::wstring> templates;

	LibError ok;

	// Find all the normal entity templates first
	ok = fs_util::ForEachFile(g_VFS, TEMPLATE_ROOT, AddToTemplates, (uintptr_t)&templates, L"*.xml", fs_util::DIR_RECURSIVE);
	WARN_ERR(ok);

	// Add all the actors too
	ok = fs_util::ForEachFile(g_VFS, ACTOR_ROOT, AddActorToTemplates, (uintptr_t)&templates, L"*.xml", fs_util::DIR_RECURSIVE);
	WARN_ERR(ok);

	return templates;
}

void CCmpTemplateManager::CopyPreviewSubset(CParamNode& out, const CParamNode& in)
{
	// We only want to include components which are necessary (for the visual previewing of an entity)
	// and safe (i.e. won't do anything that affects the synchronised simulation state), so additions
	// to this list should be carefully considered
	std::set<std::string> permittedComponentTypes;
	permittedComponentTypes.insert("Ownership");
	permittedComponentTypes.insert("Position");
	permittedComponentTypes.insert("VisualActor");
	// (This could be initialised once and reused, but it's not worth the effort)

	CParamNode::LoadXMLString(out, "<Entity/>");
	out.CopyFilteredChildrenOfChild(in, "Entity", permittedComponentTypes);
	// In the future, we might want to add some extra flags to certain components, to indicate they're
	// running in 'preview' mode and should not e.g. register with global managers
}
