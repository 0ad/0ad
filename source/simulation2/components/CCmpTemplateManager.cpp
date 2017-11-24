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

#include "simulation2/system/Component.h"
#include "ICmpTemplateManager.h"

#include "simulation2/MessageTypes.h"
#include "simulation2/serialization/SerializeTemplates.h"

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
		std::map<CStr, std::vector<entity_id_t>> templateMap;

		for (const std::pair<entity_id_t, std::string>& templateEnt : m_LatestTemplates)
			if (!ENTITY_IS_LOCAL(templateEnt.first))
				templateMap[templateEnt.second].push_back(templateEnt.first);

		SerializeMap<SerializeString, SerializeVector<SerializeU32_Unbounded>>()(serialize, "templates", templateMap);
	}

	virtual void Deserialize(const CParamNode& paramNode, IDeserializer& deserialize)
	{
		Init(paramNode);

		std::map<CStr, std::vector<entity_id_t>> templateMap;
		SerializeMap<SerializeString, SerializeVector<SerializeU32_Unbounded>>()(deserialize, "templates", templateMap);
		for (const std::pair<CStr, std::vector<entity_id_t>>& mapEl : templateMap)
			for (entity_id_t id : mapEl.second)
				m_LatestTemplates[id] = mapEl.first;
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

	virtual const CParamNode* LoadTemplate(entity_id_t ent, const std::string& templateName);

	virtual const CParamNode* GetTemplate(const std::string& templateName);

	virtual const CParamNode* GetTemplateWithoutValidation(const std::string& templateName);

	virtual bool TemplateExists(const std::string& templateName) const;

	virtual const CParamNode* LoadLatestTemplate(entity_id_t ent);

	virtual std::string GetCurrentTemplateName(entity_id_t ent) const;

	virtual std::vector<std::string> FindAllTemplates(bool includeActors) const;

	virtual std::vector<std::string> FindUsedTemplates() const;

	virtual std::vector<entity_id_t> GetEntitiesUsingTemplate(const std::string& templateName) const;

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
	std::map<entity_id_t, std::string> m_LatestTemplates;
};

REGISTER_COMPONENT_TYPE(TemplateManager)

const CParamNode* CCmpTemplateManager::LoadTemplate(entity_id_t ent, const std::string& templateName)
{
	m_LatestTemplates[ent] = templateName;

	return GetTemplate(templateName);
}

const CParamNode* CCmpTemplateManager::GetTemplate(const std::string& templateName)
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
				LOGERROR("Failed to validate entity template '%s'", templateName.c_str());
		}
		// Refuse to return invalid templates
		if (!m_TemplateSchemaValidity[templateName])
			return NULL;
	}

	const CParamNode& templateRoot = fileData.GetChild("Entity");
	if (!templateRoot.IsOk())
	{
		// The validator should never let this happen
		LOGERROR("Invalid root element in entity template '%s'", templateName.c_str());
		return NULL;
	}

	return &templateRoot;
}

const CParamNode* CCmpTemplateManager::GetTemplateWithoutValidation(const std::string& templateName)
{
	const CParamNode& templateRoot = m_templateLoader.GetTemplateFileData(templateName).GetChild("Entity");
	if (!templateRoot.IsOk())
		return NULL;

	return &templateRoot;
}

bool CCmpTemplateManager::TemplateExists(const std::string& templateName) const
{
	return m_templateLoader.TemplateExists(templateName);
}

const CParamNode* CCmpTemplateManager::LoadLatestTemplate(entity_id_t ent)
{
	std::map<entity_id_t, std::string>::const_iterator it = m_LatestTemplates.find(ent);
	if (it == m_LatestTemplates.end())
		return NULL;
	return LoadTemplate(ent, it->second);
}

std::string CCmpTemplateManager::GetCurrentTemplateName(entity_id_t ent) const
{
	std::map<entity_id_t, std::string>::const_iterator it = m_LatestTemplates.find(ent);
	if (it == m_LatestTemplates.end())
		return "";
	return it->second;
}

std::vector<std::string> CCmpTemplateManager::FindAllTemplates(bool includeActors) const
{
	ETemplatesType templatesType = includeActors ? ALL_TEMPLATES : SIMULATION_TEMPLATES;
	return m_templateLoader.FindTemplates("", true, templatesType);
}

std::vector<std::string> CCmpTemplateManager::FindUsedTemplates() const
{
	std::vector<std::string> usedTemplates;
	for (const std::pair<entity_id_t, std::string>& p : m_LatestTemplates)
		if (std::find(usedTemplates.begin(), usedTemplates.end(), p.second) == usedTemplates.end())
			usedTemplates.push_back(p.second);
	return usedTemplates;
}

/**
 * Get the list of entities using the specified template
 */
std::vector<entity_id_t> CCmpTemplateManager::GetEntitiesUsingTemplate(const std::string& templateName) const
{
	std::vector<entity_id_t> entities;
	for (const std::pair<entity_id_t, std::string>& p : m_LatestTemplates)
		if (p.second == templateName)
			entities.push_back(p.first);

	return entities;
}
