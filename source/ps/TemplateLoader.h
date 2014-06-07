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

#ifndef INCLUDED_TEMPLATELOADER
#define INCLUDED_TEMPLATELOADER

#include "simulation2/system/ParamNode.h"

enum ETemplatesType
{
	ALL_TEMPLATES,
	ACTOR_TEMPLATES,
	SIMULATION_TEMPLATES
};

/**
 * Template loader: Handles the loading of entity template files for:
 * - the initialisation and deserialization of entity components in the 
 *   simulation (CmpTemplateManager).
 * - access to actor templates, obstruction data, etc. in RMS/RMGEN
 *
 * Template names are intentionally restricted to ASCII strings for storage/serialization
 * efficiency (we have a lot of strings so this is significant);
 * they correspond to filenames so they shouldn't contain non-ASCII anyway.
 */
class CTemplateLoader
{
public:
	CTemplateLoader()
	{
	}
	
	/**
	 * Provides the file data for requested template.
	 */
	const CParamNode &GetTemplateFileData(const std::string& templateName);

	/**
	 * Returns a list of strings that could be validly passed as @c templateName to LoadTemplateFile.
	 * (This includes "actor|foo" etc names).
	 */
	std::vector<std::string> FindTemplates(const std::string& path, bool includeSubdirectories, ETemplatesType templatesType);

private:
	/**
	 * (Re)loads the given template, regardless of whether it exists already,
	 * and saves into m_TemplateFileData. Also loads any parents that are not yet
	 * loaded. Returns false on error.
	 * @param templateName XML filename to load (not a |-separated string)
	 */
	bool LoadTemplateFile(const std::string& templateName, int depth);

	/**
	 * Constructs a standard static-decorative-object template for the given actor
	 */
	void ConstructTemplateActor(const std::string& actorName, CParamNode& out);

	/** 
	 * Copy the non-interactive components of an entity template (position, actor, etc) into
	 * a new entity template
	 */
	void CopyPreviewSubset(CParamNode& out, const CParamNode& in, bool corpse);

	/** 
	 * Copy the components of an entity template necessary for a construction foundation
	 * (position, actor, armour, health, etc) into a new entity template
	 */
	void CopyFoundationSubset(CParamNode& out, const CParamNode& in);

	/** 
	 * Copy the components of an entity template necessary for a non-foundation construction entity
	 * into a new entity template
	 */
	void CopyConstructionSubset(CParamNode& out, const CParamNode& in);

	/** 
	 * Copy the components of an entity template necessary for a gatherable resource
	 * into a new entity template
	 */
	void CopyResourceSubset(CParamNode& out, const CParamNode& in);

	/**
	 * Map from template name (XML filename or special |-separated string) to the most recently
	 * loaded non-broken template data. This includes files that will fail schema validation.
	 * (Failed loads won't remove existing entries under the same name, so we behave more nicely
	 * when hotloading broken files)
	 */
	std::map<std::string, CParamNode> m_TemplateFileData;
};

#endif // INCLUDED_TEMPLATELOADER
