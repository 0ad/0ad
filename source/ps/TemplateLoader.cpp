/* Copyright (C) 2014 Wildfire Games.
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

#include "ps/CLogger.h"
#include "ps/Filesystem.h"
#include "ps/XML/Xeromyces.h"
#include "lib/utf8.h"

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

	// Handle special case "mirage|foo"
	if (templateName.find("mirage|") == 0)
	{
		// Load the base entity template, if it wasn't already loaded
		std::string baseName = templateName.substr(7);
		if (!LoadTemplateFile(baseName, depth+1))
		{
			LOGERROR(L"Failed to load entity template '%hs'", baseName.c_str());
			return false;
		}
		// Copy a subset to the requested template
		CopyMirageSubset(m_TemplateFileData[templateName], m_TemplateFileData[baseName]);
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

	// Handle special case "construction|foo"
	if (templateName.find("construction|") == 0)
	{
		// Load the base entity template, if it wasn't already loaded
		std::string baseName = templateName.substr(13);
		if (!LoadTemplateFile(baseName, depth+1))
		{
			LOGERROR(L"Failed to load entity template '%hs'", baseName.c_str());
			return false;
		}
		// Copy a subset to the requested template
		CopyConstructionSubset(m_TemplateFileData[templateName], m_TemplateFileData[baseName]);
		return true;
	}

	// Handle special case "resource|foo"
	if (templateName.find("resource|") == 0)
	{
		// Load the base entity template, if it wasn't already loaded
		std::string baseName = templateName.substr(9);
		if (!LoadTemplateFile(baseName, depth+1))
		{
			LOGERROR(L"Failed to load entity template '%hs'", baseName.c_str());
			return false;
		}
		// Copy a subset to the requested template
		CopyResourceSubset(m_TemplateFileData[templateName], m_TemplateFileData[baseName]);
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

std::vector<std::string> CTemplateLoader::FindPlaceableTemplates(const std::string& path, bool includeSubdirectories, ETemplatesType templatesType, ScriptInterface& scriptInterface)
{
	JSContext* cx = scriptInterface.GetContext();
	JSAutoRequest rq(cx);
	
	std::vector<std::string> templates;
	Status ok;
	VfsPath templatePath;


	if (templatesType == SIMULATION_TEMPLATES || templatesType == ALL_TEMPLATES)
	{
		JS::RootedValue placeablesFilter(cx);
		scriptInterface.ReadJSONFile("simulation/data/placeablesFilter.json", &placeablesFilter);
		
		std::vector<CScriptValRooted> folders;
		if (scriptInterface.GetProperty(placeablesFilter, "templates", folders))
		{
			templatePath = VfsPath(TEMPLATE_ROOT) / path;
			//I have every object inside, just run for each
			for (std::vector<CScriptValRooted>::iterator iterator = folders.begin(); iterator != folders.end();++iterator)
			{
				JS::RootedValue val(cx, (*iterator).get());
				std::string directoryPath;
				std::wstring fileFilter;
				scriptInterface.GetProperty(val, "directory", directoryPath);
				scriptInterface.GetProperty(val, "file", fileFilter);
				
				VfsPaths filenames;
				if (vfs::GetPathnames(g_VFS, templatePath / (directoryPath + "/"), fileFilter.c_str(), filenames) != INFO::OK)
					continue;
				
				for (VfsPaths::iterator it = filenames.begin(); it != filenames.end(); ++it)
				{
					VfsPath filename = *it;
					// Strip the .xml extension
					VfsPath pathstem = filename.ChangeExtension(L"");
					// Strip the root from the path
					std::wstring name = pathstem.string().substr(ARRAY_SIZE(TEMPLATE_ROOT) - 1);

					templates.push_back(std::string(name.begin(), name.end()));
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

	if (templatesType != SIMULATION_TEMPLATES && templatesType != ACTOR_TEMPLATES && templatesType != ALL_TEMPLATES)
		LOGERROR(L"Undefined template type (valid: all, simulation, actor)");

	return templates;
}

std::vector<std::string> CTemplateLoader::FindTemplates(const std::string& path, bool includeSubdirectories, ETemplatesType templatesType)
{
	std::vector<std::string> templates;

	Status ok;
	VfsPath templatePath;

	if (templatesType == SIMULATION_TEMPLATES || templatesType == ALL_TEMPLATES)
	{
		templatePath = VfsPath(TEMPLATE_ROOT) / path;
		if (includeSubdirectories)
			ok = vfs::ForEachFile(g_VFS, templatePath, AddToTemplates, (uintptr_t)&templates, L"*.xml", vfs::DIR_RECURSIVE);
		else
			ok = vfs::ForEachFile(g_VFS, templatePath, AddToTemplates, (uintptr_t)&templates, L"*.xml");
		WARN_IF_ERR(ok);
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

	if (templatesType != SIMULATION_TEMPLATES && templatesType != ACTOR_TEMPLATES && templatesType != ALL_TEMPLATES)
		LOGERROR(L"Undefined template type (valid: all, simulation, actor)");

	return templates;
}

const CParamNode& CTemplateLoader::GetTemplateFileData(const std::string& templateName)
{
	// Load the template if necessary
	if (!LoadTemplateFile(templateName, 0))
	{
		LOGERROR(L"Failed to load entity template '%hs'", templateName.c_str());
		return NULL_NODE;
	}

	return m_TemplateFileData[templateName];
}

void CTemplateLoader::ConstructTemplateActor(const std::string& actorName, CParamNode& out)
{
	// Load the base actor template if necessary
	const char* templateName = "special/actor";
	if (!LoadTemplateFile(templateName, 0))
	{
		LOGERROR(L"Failed to load entity template '%hs'", templateName);
		return;
	}

	// Copy the actor template
	out = m_TemplateFileData[templateName];

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

void CTemplateLoader::CopyPreviewSubset(CParamNode& out, const CParamNode& in, bool corpse)
{
	// We only want to include components which are necessary (for the visual previewing of an entity)
	// and safe (i.e. won't do anything that affects the synchronised simulation state), so additions
	// to this list should be carefully considered
	std::set<std::string> permittedComponentTypes;
	permittedComponentTypes.insert("Identity");
	permittedComponentTypes.insert("Ownership");
	permittedComponentTypes.insert("Position");
	permittedComponentTypes.insert("VisualActor");
	permittedComponentTypes.insert("Footprint");
	permittedComponentTypes.insert("Obstruction");
	permittedComponentTypes.insert("Decay");
	permittedComponentTypes.insert("BuildRestrictions");

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
		// Previews should not cast shadows
		if (out.GetChild("Entity").GetChild("VisualActor").IsOk())
			CParamNode::LoadXMLString(out, "<Entity><VisualActor><DisableShadows/></VisualActor></Entity>");

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

		// Corpses should remain visible in fog-of-war
		CParamNode::LoadXMLString(out, "<Entity><Vision><Range>0</Range><RetainInFog>true</RetainInFog><AlwaysVisible>false</AlwaysVisible></Vision></Entity>");
	}
}

void CTemplateLoader::CopyMirageSubset(CParamNode& out, const CParamNode& in)
{
	// Currently used for mirage entities replacing real ones in fog-of-war

	std::set<std::string> permittedComponentTypes;
	permittedComponentTypes.insert("Footprint");
	permittedComponentTypes.insert("Minimap");
	permittedComponentTypes.insert("Ownership");
	permittedComponentTypes.insert("Position");
	permittedComponentTypes.insert("Selectable");
	permittedComponentTypes.insert("VisualActor");

	CParamNode::LoadXMLString(out, "<Entity/>");
	out.CopyFilteredChildrenOfChild(in, "Entity", permittedComponentTypes);

	// Select a subset of identity data. We don't want to have, for example, a CC mirage
	// that has also the CC class and then prevents construction of other CCs
	std::set<std::string> identitySubset;
	identitySubset.insert("Civ");
	identitySubset.insert("GenericName");
	identitySubset.insert("SpecificName");
	identitySubset.insert("Tooltip");
	identitySubset.insert("History");
	identitySubset.insert("Icon");
	CParamNode identity;
	CParamNode::LoadXMLString(identity, "<Identity/>");
	identity.CopyFilteredChildrenOfChild(in.GetChild("Entity"), "Identity", identitySubset);
	CParamNode::LoadXMLString(out, ("<Entity>"+utf8_from_wstring(identity.ToXML())+"</Entity>").c_str());

	// Set the entity as mirage entity
	CParamNode::LoadXMLString(out, "<Entity><Mirage/></Entity>");
	CParamNode::LoadXMLString(out, "<Entity><Vision><Range>0</Range><RetainInFog>true</RetainInFog><AlwaysVisible>false</AlwaysVisible></Vision></Entity>");
}

void CTemplateLoader::CopyFoundationSubset(CParamNode& out, const CParamNode& in)
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
	permittedComponentTypes.insert("Fogging");
	permittedComponentTypes.insert("Armour");
	permittedComponentTypes.insert("Health");
	permittedComponentTypes.insert("StatusBars");
	permittedComponentTypes.insert("OverlayRenderer");
	permittedComponentTypes.insert("Decay");
	permittedComponentTypes.insert("Cost");
	permittedComponentTypes.insert("Sound");
	permittedComponentTypes.insert("Vision");
	permittedComponentTypes.insert("AIProxy");
	permittedComponentTypes.insert("RallyPoint");
	permittedComponentTypes.insert("RallyPointRenderer");

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

void CTemplateLoader::CopyConstructionSubset(CParamNode& out, const CParamNode& in)
{
	// Currently used for buildings rising during construction
	// Mostly serves to filter out components like Vision, UnitAI, etc.
	std::set<std::string> permittedComponentTypes;
	permittedComponentTypes.insert("Footprint");
	permittedComponentTypes.insert("Ownership");
	permittedComponentTypes.insert("Position");
	permittedComponentTypes.insert("VisualActor");

	CParamNode::LoadXMLString(out, "<Entity/>");
	out.CopyFilteredChildrenOfChild(in, "Entity", permittedComponentTypes);
}

void CTemplateLoader::CopyResourceSubset(CParamNode& out, const CParamNode& in)
{
	// Currently used for animals which die and leave a gatherable corpse.
	// Mostly serves to filter out components like Vision, UnitAI, etc.
	std::set<std::string> permittedComponentTypes;
	permittedComponentTypes.insert("Ownership");
	permittedComponentTypes.insert("Position");
	permittedComponentTypes.insert("VisualActor");
	permittedComponentTypes.insert("Identity");
	permittedComponentTypes.insert("Obstruction");
	permittedComponentTypes.insert("Minimap");
	permittedComponentTypes.insert("ResourceSupply");
	permittedComponentTypes.insert("Selectable");
	permittedComponentTypes.insert("Footprint");
	permittedComponentTypes.insert("StatusBars");
	permittedComponentTypes.insert("OverlayRenderer");
	permittedComponentTypes.insert("Sound");
	permittedComponentTypes.insert("AIProxy");

	CParamNode::LoadXMLString(out, "<Entity/>");
	out.CopyFilteredChildrenOfChild(in, "Entity", permittedComponentTypes);
}
