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

#ifndef INCLUDED_ICMPTEMPLATEMANAGER
#define INCLUDED_ICMPTEMPLATEMANAGER

#include "simulation2/system/Interface.h"

#include <vector>

/**
 * Template manager: Handles the loading of entity template files for the initialisation
 * and deserialization of entity components.
 *
 * Template names are intentionally restricted to ASCII strings for storage/serialization
 * efficiency (we have a lot of strings so this is significant);
 * they correspond to filenames so they shouldn't contain non-ASCII anyway.
 */
class ICmpTemplateManager : public IComponent
{
public:
	/**
	 * Loads the template XML file identified by 'templateName' (including inheritance
	 * from parent XML files) for use with a new entity 'ent'.
	 * The returned CParamNode must not be used for any entities other than 'ent'.
	 *
	 * If templateName is of the form "actor|foo" then it will load a default
	 * stationary entity template that uses actor "foo". (This is a convenience to
	 * avoid the need for hundreds of tiny decorative-object entity templates.)
	 *
	 * If templateName is of the form "preview|foo" then it will load a template
	 * based on entity template "foo" with the non-graphical components removed.
	 * (This is for previewing construction/placement of units.)
	 *
	 * If templateName is of the form "corpse|foo" then it will load a template
	 * like "preview|foo" but with corpse-related components included.
	 *
	 * If templateName is of the form "foundation|foo" then it will load a template
	 * based on entity template "foo" with various components removed and a few changed
	 * and added. (This is for constructing foundations of buildings.)
	 *
	 * @return NULL on error
	 */
	virtual const CParamNode* LoadTemplate(entity_id_t ent, const std::string& templateName, int playerID) = 0;

	/**
	 * Loads the template XML file identified by 'templateName' (including inheritance
	 * from parent XML files). The templateName syntax is the same as LoadTemplate.
	 *
	 * @return NULL on error
	 */
	virtual const CParamNode* GetTemplate(std::string templateName) = 0;

	/**
	 * Like GetTemplate, except without doing the XML validation (so it's faster but
	 * may return invalid templates).
	 *
	 * @return NULL on error
	 */
	virtual const CParamNode* GetTemplateWithoutValidation(std::string templateName) = 0;

	/**
	 * Returns the template most recently specified for the entity 'ent'.
	 * Used during deserialization.
	 *
	 * @return NULL on error
	 */
	virtual const CParamNode* LoadLatestTemplate(entity_id_t ent) = 0;

	/**
	 * Returns the name of the template most recently specified for the entity 'ent'.
	 */
	virtual std::string GetCurrentTemplateName(entity_id_t ent) = 0;

	/**
	 * Returns the list of entities having the specified template.
	 */
	virtual std::vector<entity_id_t> GetEntitiesUsingTemplate(std::string templateName) = 0;

	/**
	 * Returns a list of strings that could be validly passed as @c templateName to LoadTemplate.
	 * (This includes "actor|foo" etc names).
	 * Intended for use by the map editor. This is likely to be quite slow.
	 */
	virtual std::vector<std::string> FindAllTemplates(bool includeActors) = 0;

	virtual std::vector<std::string> FindAllPlaceableTemplates(bool includeActors) = 0;

	/**
	 * Permanently disable XML validation (intended solely for test cases).
	 */
	virtual void DisableValidation() = 0;

	/*
	 * TODO:
	 * When an entity changes template (e.g. upgrades) or player ownership, it
	 * should call some Reload(ent, templateName, playerID) function to load its new template.
	 * When a file changes on disk, something should call Reload(templateName).
	 *
	 * Reloading should happen by sending a message to affected components (containing
	 * their new CParamNode), then automatically updating this.template of scripted components.
	 */

	DECLARE_INTERFACE_TYPE(TemplateManager)
};

#endif // INCLUDED_ICMPTEMPLATEMANAGER
