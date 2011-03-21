/* Copyright (C) 2011 Wildfire Games.
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

#include "simulation2/MessageTypes.h"

#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/RelaxNG.h"
#include "ps/XML/Xeromyces.h"

static const wchar_t TEMPLATE_ROOT[] = L"simulation/templates/";
static const wchar_t ACTOR_ROOT[] = L"art/actors/";

class CCmpTemplateManager : public ICmpTemplateManager
{
public:
	static void ClassInit(CComponentManager& componentManager)
	{
		componentManager.SubscribeGloballyToMessageType(MT_Destroy);
	}

	DEFAULT_COMPONENT_ALLOCATOR(TemplateManager)

	static std::string GetSchema()
	{
		return "<a:component type='system'/><empty/>";
	}

	virtual void Init(const CParamNode& UNUSED(paramNode))
	{
		m_DisableValidation = false;

		m_Validator.LoadGrammar(GetSimContext().GetComponentManager().GenerateSchema());
		// TODO: handle errors loading the grammar here?
		// TODO: support hotloading changes to the grammar
	}

	virtual void Deinit()
	{
	}

	virtual void Serialize(ISerializer& serialize)
	{
		size_t count = 0;

		for (std::map<entity_id_t, std::string>::const_iterator it = m_LatestTemplates.begin(); it != m_LatestTemplates.end(); ++it)
		{
			if (ENTITY_IS_LOCAL(it->first))
				continue;
			++count;
		}
		serialize.NumberU32_Unbounded("num entities", (u32)count);

		for (std::map<entity_id_t, std::string>::const_iterator it = m_LatestTemplates.begin(); it != m_LatestTemplates.end(); ++it)
		{
			if (ENTITY_IS_LOCAL(it->first))
				continue;
			serialize.NumberU32_Unbounded("id", it->first);
			serialize.StringASCII("template", it->second, 0, 256);
		}
		// TODO: maybe we should do some kind of interning thing instead of printing so many strings?

		// TODO: will need to serialize techs too, because we need to be giving out
		// template data before other components (like the tech components) have been deserialized
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		u32 numEntities;
		deserialize.NumberU32_Unbounded("num entities", numEntities);
		for (u32 i = 0; i < numEntities; ++i)
		{
			entity_id_t ent;
			std::string templateName;
			deserialize.NumberU32_Unbounded("id", ent);
			deserialize.StringASCII("template", templateName, 0, 256);
			m_LatestTemplates[ent] = templateName;
		}
	}

	virtual void HandleMessage(const CMessage& msg, bool UNUSED(global))
	{
		switch (msg.GetType())
		{
		case MT_Destroy:
		{
			const CMessageDestroy& msgData = static_cast<const CMessageDestroy&> (msg);

			// Clean up m_LatestTemplates so it doesn't record any data for destroyed entities
			m_LatestTemplates.erase(msgData.entity);

			break;
		}
		}
	}

	virtual void DisableValidation()
	{
		m_DisableValidation = true;
	}

	virtual const CParamNode* LoadTemplate(entity_id_t ent, const std::string& templateName, int playerID);

	virtual const CParamNode* GetTemplate(std::string templateName);

	virtual const CParamNode* GetTemplateWithoutValidation(std::string templateName);

	virtual const CParamNode* LoadLatestTemplate(entity_id_t ent);

	virtual std::string GetCurrentTemplateName(entity_id_t ent);

	virtual std::vector<std::string> FindAllTemplates(bool includeActors);

	virtual std::vector<entity_id_t> GetEntitiesUsingTemplate(std::string templateName);

private:
	// Entity template XML validator
	RelaxNGValidator m_Validator;

	// Disable validation, for test cases
	bool m_DisableValidation;

	// Map from template name (XML filename or special |-separated string) to the most recently
	// loaded non-broken template data. This includes files that will fail schema validation.
	// (Failed loads won't remove existing entries under the same name, so we behave more nicely
	// when hotloading broken files)
	std::map<std::string, CParamNode> m_TemplateFileData;

	// Map from template name to schema validation status.
	// (Some files, e.g. inherited parent templates, may not be valid themselves but we still need to load
	// them and use them; we only reject invalid templates that were requested directly by GetTemplate/etc)
	std::map<std::string, bool> m_TemplateSchemaValidity;

	// Remember the template used by each entity, so we can return them
	// again for deserialization.
	// TODO: should store player ID etc.
	std::map<entity_id_t, std::string> m_LatestTemplates;

	// (Re)loads the given template, regardless of whether it exists already,
	// and saves into m_TemplateFileData. Also loads any parents that are not yet
	// loaded. Returns false on error.
	// @param templateName XML filename to load (not a |-separated string)
	bool LoadTemplateFile(const std::string& templateName, int depth);

	// Constructs a standard static-decorative-object template for the given actor
	void ConstructTemplateActor(const std::string& actorName, CParamNode& out);

	// Copy the non-interactive components of an entity template (position, actor, etc) into
	// a new entity template
	void CopyPreviewSubset(CParamNode& out, const CParamNode& in, bool corpse);

	// Copy the components of an entity necessary for a construction foundation
	// (position, actor, armour, health, etc) into a new entity template
	void CopyFoundationSubset(CParamNode& out, const CParamNode& in);
};

REGISTER_COMPONENT_TYPE(TemplateManager)

const CParamNode* CCmpTemplateManager::LoadTemplate(entity_id_t ent, const std::string& templateName, int UNUSED(playerID))
{
	m_LatestTemplates[ent] = templateName;

	const CParamNode* templateRoot = GetTemplate(templateName);
	if (!templateRoot)
		return NULL;

	// TODO: Eventually we need to support techs in here, and return a different template per playerID

	return templateRoot;
}

const CParamNode* CCmpTemplateManager::GetTemplate(std::string templateName)
{
	// Load the template if necessary
	if (!LoadTemplateFile(templateName, 0))
	{
		LOGERROR(L"Failed to load entity template '%hs'", templateName.c_str());
		return NULL;
	}

	if (!m_DisableValidation)
	{
		// Compute validity, if it's not computed before
		if (m_TemplateSchemaValidity.find(templateName) == m_TemplateSchemaValidity.end())
			m_TemplateSchemaValidity[templateName] = m_Validator.Validate(wstring_from_utf8(templateName), m_TemplateFileData[templateName].ToXML());
		// Refuse to return invalid templates
		if (!m_TemplateSchemaValidity[templateName])
			return NULL;
	}

	const CParamNode& templateRoot = m_TemplateFileData[templateName].GetChild("Entity");
	if (!templateRoot.IsOk())
	{
		// The validator should never let this happen
		LOGERROR(L"Invalid root element in entity template '%hs'", templateName.c_str());
		return NULL;
	}

	return &templateRoot;
}

const CParamNode* CCmpTemplateManager::GetTemplateWithoutValidation(std::string templateName)
{
	// Load the template if necessary
	if (!LoadTemplateFile(templateName, 0))
	{
		LOGERROR(L"Failed to load entity template '%hs'", templateName.c_str());
		return NULL;
	}

	const CParamNode& templateRoot = m_TemplateFileData[templateName].GetChild("Entity");
	if (!templateRoot.IsOk())
		return NULL;

	return &templateRoot;
}

const CParamNode* CCmpTemplateManager::LoadLatestTemplate(entity_id_t ent)
{
	std::map<entity_id_t, std::string>::const_iterator it = m_LatestTemplates.find(ent);
	if (it == m_LatestTemplates.end())
		return NULL;
	return LoadTemplate(ent, it->second, -1);
}

std::string CCmpTemplateManager::GetCurrentTemplateName(entity_id_t ent)
{
	std::map<entity_id_t, std::string>::const_iterator it = m_LatestTemplates.find(ent);
	if (it == m_LatestTemplates.end())
		return "";
	return it->second;
}

bool CCmpTemplateManager::LoadTemplateFile(const std::string& templateName, int depth)
{
	// If this file was already loaded, we don't need to do anything
	if (m_TemplateFileData.find(templateName) != m_TemplateFileData.end())
		return true;

	// Handle infinite loops more gracefully than running out of stack space and crashing
	if (depth > 100)
	{
		LOGERROR(L"Probable infinite inheritance loop in entity template '%hs'", templateName.c_str());
		return false;
	}

	// Handle special case "actor|foo"
	if (templateName.find("actor|") == 0)
	{
		ConstructTemplateActor(templateName.substr(6), m_TemplateFileData[templateName]);
		return true;
	}

	// Handle special case "preview|foo"
	if (templateName.find("preview|") == 0)
	{
		// Load the base entity template, if it wasn't already loaded
		std::string baseName = templateName.substr(8);
		if (!LoadTemplateFile(baseName, depth+1))
		{
			LOGERROR(L"Failed to load entity template '%hs'", baseName.c_str());
			return false;
		}
		// Copy a subset to the requested template
		CopyPreviewSubset(m_TemplateFileData[templateName], m_TemplateFileData[baseName], false);
		return true;
	}

	// Handle special case "corpse|foo"
	if (templateName.find("corpse|") == 0)
	{
		// Load the base entity template, if it wasn't already loaded
		std::string baseName = templateName.substr(7);
		if (!LoadTemplateFile(baseName, depth+1))
		{
			LOGERROR(L"Failed to load entity template '%hs'", baseName.c_str());
			return false;
		}
		// Copy a subset to the requested template
		CopyPreviewSubset(m_TemplateFileData[templateName], m_TemplateFileData[baseName], true);
		return true;
	}

	// Handle special case "foundation|foo"
	if (templateName.find("foundation|") == 0)
	{
		// Load the base entity template, if it wasn't already loaded
		std::string baseName = templateName.substr(11);
		if (!LoadTemplateFile(baseName, depth+1))
		{
			LOGERROR(L"Failed to load entity template '%hs'", baseName.c_str());
			return false;
		}
		// Copy a subset to the requested template
		CopyFoundationSubset(m_TemplateFileData[templateName], m_TemplateFileData[baseName]);
		return true;
	}

	// Normal case: templateName is an XML file:

	VfsPath path = Path::Join(VfsPath(TEMPLATE_ROOT), wstring_from_utf8(templateName + ".xml"));
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
			LOGERROR(L"Invalid parent '%hs' in entity template '%hs'", parentName.c_str(), templateName.c_str());
			return false;
		}

		// Ensure the parent is loaded
		if (!LoadTemplateFile(parentName, depth+1))
		{
			LOGERROR(L"Failed to load parent '%hs' of entity template '%hs'", parentName.c_str(), templateName.c_str());
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

void CCmpTemplateManager::ConstructTemplateActor(const std::string& actorName, CParamNode& out)
{
	std::string name = utf8_from_wstring(CParamNode::EscapeXMLString(wstring_from_utf8(actorName)));
	std::string xml = "<?xml version='1.0' encoding='utf-8'?>"
		"<Entity>"
		"<Position>"
			"<Anchor>upright</Anchor>"
			"<Altitude>0</Altitude>"
			"<Floating>false</Floating>"
		"</Position>"
		"<VisualActor>"
			"<Actor>" + name + "</Actor>"
			"<SilhouetteDisplay>false</SilhouetteDisplay>"
			"<SilhouetteOccluder>false</SilhouetteOccluder>"
		"</VisualActor>"
		"</Entity>";

	out.LoadXMLString(out, xml.c_str());
}

static LibError AddToTemplates(const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	std::vector<std::string>& templates = *(std::vector<std::string>*)cbData;

	// Strip the .xml extension
	VfsPath pathstem = Path::ChangeExtension(pathname, L"");
	// Strip the root from the path
	std::wstring name = pathstem.substr(ARRAY_SIZE(TEMPLATE_ROOT)-1);

	// We want to ignore template_*.xml templates, since they should never be built in the editor
	if (name.substr(0, 9) == L"template_")
		return INFO::OK;

	templates.push_back(std::string(name.begin(), name.end()));
	return INFO::OK;
}

static LibError AddActorToTemplates(const VfsPath& pathname, const FileInfo& UNUSED(fileInfo), const uintptr_t cbData)
{
	std::vector<std::string>& templates = *(std::vector<std::string>*)cbData;

	// Strip the root from the path
	std::wstring name = pathname.substr(ARRAY_SIZE(ACTOR_ROOT)-1);

	templates.push_back("actor|" + std::string(name.begin(), name.end()));
	return INFO::OK;
}

std::vector<std::string> CCmpTemplateManager::FindAllTemplates(bool includeActors)
{
	// TODO: eventually this should probably read all the template files and look for flags to
	// determine which should be displayed in the editor (and in what categories etc); for now we'll
	// just return all the files

	std::vector<std::string> templates;

	LibError ok;

	// Find all the normal entity templates first
	ok = fs_util::ForEachFile(g_VFS, TEMPLATE_ROOT, AddToTemplates, (uintptr_t)&templates, L"*.xml", fs_util::DIR_RECURSIVE);
	WARN_ERR(ok);

	if (includeActors)
	{
		// Add all the actors too
		ok = fs_util::ForEachFile(g_VFS, ACTOR_ROOT, AddActorToTemplates, (uintptr_t)&templates, L"*.xml", fs_util::DIR_RECURSIVE);
		WARN_ERR(ok);
	}

	return templates;
}

/**
 * Get the list of entities using the specified template
 */
std::vector<entity_id_t> CCmpTemplateManager::GetEntitiesUsingTemplate(std::string templateName)
{
	std::vector<entity_id_t> entities;
	for (std::map<entity_id_t, std::string>::const_iterator it = m_LatestTemplates.begin(); it != m_LatestTemplates.end(); ++it)
	{
		if(it->second == templateName)
			entities.push_back(it->first);
	}
	return entities;
}

void CCmpTemplateManager::CopyPreviewSubset(CParamNode& out, const CParamNode& in, bool corpse)
{
	// We only want to include components which are necessary (for the visual previewing of an entity)
	// and safe (i.e. won't do anything that affects the synchronised simulation state), so additions
	// to this list should be carefully considered
	std::set<std::string> permittedComponentTypes;
	permittedComponentTypes.insert("Ownership");
	permittedComponentTypes.insert("Position");
	permittedComponentTypes.insert("VisualActor");
	permittedComponentTypes.insert("Footprint");
	permittedComponentTypes.insert("Obstruction");
	permittedComponentTypes.insert("Decay");

	// Need these for the Actor Viewer:
	permittedComponentTypes.insert("Attack");
	permittedComponentTypes.insert("UnitMotion");
	permittedComponentTypes.insert("Sound");

	// (This set could be initialised once and reused, but it's not worth the effort)

	CParamNode::LoadXMLString(out, "<Entity/>");
	out.CopyFilteredChildrenOfChild(in, "Entity", permittedComponentTypes);

	// Disable the Obstruction component (if there is one) so it doesn't affect pathfinding
	// (but can still be used for testing this entity for collisions against others)
	if (out.GetChild("Entity").GetChild("Obstruction").IsOk())
		CParamNode::LoadXMLString(out, "<Entity><Obstruction><Active>false</Active></Obstruction></Entity>");

	if (!corpse)
	{
		// Previews should always be visible in fog-of-war/etc
		CParamNode::LoadXMLString(out, "<Entity><Vision><Range>0</Range><RetainInFog>false</RetainInFog><AlwaysVisible>true</AlwaysVisible></Vision></Entity>");
	}

	if (corpse)
	{
		// Corpses should include decay components and un-inactivate them
		if (out.GetChild("Entity").GetChild("Decay").IsOk())
			CParamNode::LoadXMLString(out, "<Entity><Decay><Inactive disable=''/></Decay></Entity>");

		// Corpses shouldn't display silhouettes (especially since they're often half underground)
		if (out.GetChild("Entity").GetChild("VisualActor").IsOk())
			CParamNode::LoadXMLString(out, "<Entity><VisualActor><SilhouetteDisplay>false</SilhouetteDisplay></VisualActor></Entity>");
	}
}

void CCmpTemplateManager::CopyFoundationSubset(CParamNode& out, const CParamNode& in)
{
	// TODO: this is all kind of yucky and hard-coded; it'd be nice to have a more generic
	// extensible scriptable way to define these subsets

	std::set<std::string> permittedComponentTypes;
	permittedComponentTypes.insert("Ownership");
	permittedComponentTypes.insert("Position");
	permittedComponentTypes.insert("VisualActor");
	permittedComponentTypes.insert("Identity");
	permittedComponentTypes.insert("BuildRestrictions");
	permittedComponentTypes.insert("Obstruction");
	permittedComponentTypes.insert("Selectable");
	permittedComponentTypes.insert("Footprint");
	permittedComponentTypes.insert("Armour");
	permittedComponentTypes.insert("Health");
	permittedComponentTypes.insert("StatusBars");
	permittedComponentTypes.insert("OverlayRenderer");
	permittedComponentTypes.insert("Decay");
	permittedComponentTypes.insert("Cost");
	permittedComponentTypes.insert("Sound");
	permittedComponentTypes.insert("Vision");
	permittedComponentTypes.insert("AIProxy");

	CParamNode::LoadXMLString(out, "<Entity/>");
	out.CopyFilteredChildrenOfChild(in, "Entity", permittedComponentTypes);

	// Switch the actor to foundation mode
	CParamNode::LoadXMLString(out, "<Entity><VisualActor><Foundation/></VisualActor></Entity>");

	// Add the Foundation component, to deal with the construction process
	CParamNode::LoadXMLString(out, "<Entity><Foundation/></Entity>");

	// Initialise health to 1
	CParamNode::LoadXMLString(out, "<Entity><Health><Initial>1</Initial></Health></Entity>");

	// Foundations shouldn't initially block unit movement
	if (out.GetChild("Entity").GetChild("Obstruction").IsOk())
		CParamNode::LoadXMLString(out, "<Entity><Obstruction><DisableBlockMovement>true</DisableBlockMovement><DisableBlockPathfinding>true</DisableBlockPathfinding></Obstruction></Entity>");

	// Don't provide population bonuses yet (but still do take up population cost)
	if (out.GetChild("Entity").GetChild("Cost").IsOk())
		CParamNode::LoadXMLString(out, "<Entity><Cost><PopulationBonus>0</PopulationBonus></Cost></Entity>");

	// Foundations should be visible themselves in fog-of-war if their base template is,
	// but shouldn't have any vision range
	if (out.GetChild("Entity").GetChild("Vision").IsOk())
		CParamNode::LoadXMLString(out, "<Entity><Vision><Range>0</Range></Vision></Entity>");
}
