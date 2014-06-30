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

#include "simulation2/system/Component.h"
#include "ICmpTemplateManager.h"

#include "simulation2/MessageTypes.h"

#include "ps/TemplateLoader.h"

#include "lib/utf8.h"
#include "ps/CLogger.h"
#include "ps/XML/RelaxNG.h"

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

	virtual std::vector<std::string> FindAllPlaceableTemplates(bool includeActors);

	virtual std::vector<entity_id_t> GetEntitiesUsingTemplate(std::string templateName);

private:
	// Template loader
	CTemplateLoader m_templateLoader;

	// Entity template XML validator
	RelaxNGValidator m_Validator;

	// Disable validation, for test cases
	bool m_DisableValidation;

	// Map from template name to schema validation status.
	// (Some files, e.g. inherited parent templates, may not be valid themselves but we still need to load
	// them and use them; we only reject invalid templates that were requested directly by GetTemplate/etc)
	std::map<std::string, bool> m_TemplateSchemaValidity;

	// Remember the template used by each entity, so we can return them
	// again for deserialization.
	// TODO: should store player ID etc.
	std::map<entity_id_t, std::string> m_LatestTemplates;
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
	const CParamNode& fileData = m_templateLoader.GetTemplateFileData(templateName);
	if (!fileData.IsOk())
		return NULL;

	if (!m_DisableValidation)
	{
		// Compute validity, if it's not computed before
		if (m_TemplateSchemaValidity.find(templateName) == m_TemplateSchemaValidity.end())
		{
			m_TemplateSchemaValidity[templateName] = m_Validator.Validate(wstring_from_utf8(templateName), fileData.ToXML());

			// Show error on the first failure to validate the template
			if (!m_TemplateSchemaValidity[templateName])
				LOGERROR(L"Failed to validate entity template '%hs'", templateName.c_str());
		}
		// Refuse to return invalid templates
		if (!m_TemplateSchemaValidity[templateName])
			return NULL;
	}

	const CParamNode& templateRoot = fileData.GetChild("Entity");
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
	const CParamNode& templateRoot = m_templateLoader.GetTemplateFileData(templateName).GetChild("Entity");
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

std::vector<std::string> CCmpTemplateManager::FindAllTemplates(bool includeActors)
{
	ETemplatesType templatesType = includeActors ? ALL_TEMPLATES : SIMULATION_TEMPLATES;
	return m_templateLoader.FindTemplates("", true, templatesType);
}

std::vector<std::string> CCmpTemplateManager::FindAllPlaceableTemplates(bool includeActors)
{
	ScriptInterface& scriptInterface = this->GetSimContext().GetScriptInterface();

	ETemplatesType templatesType = includeActors ? ALL_TEMPLATES : SIMULATION_TEMPLATES;
	return m_templateLoader.FindPlaceableTemplates("", true, templatesType, scriptInterface);
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
