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

#include "ICmpTemplateManager.h"

#include "simulation2/system/InterfaceScripted.h"

BEGIN_INTERFACE_WRAPPER(TemplateManager)
DEFINE_INTERFACE_METHOD_1("GetTemplate", const CParamNode*, ICmpTemplateManager, GetTemplate, std::string)
DEFINE_INTERFACE_METHOD_1("GetTemplateWithoutValidation", const CParamNode*, ICmpTemplateManager, GetTemplateWithoutValidation, std::string)
DEFINE_INTERFACE_METHOD_CONST_1("TemplateExists", bool, ICmpTemplateManager, TemplateExists, std::string)
DEFINE_INTERFACE_METHOD_CONST_1("GetCurrentTemplateName", std::string, ICmpTemplateManager, GetCurrentTemplateName, entity_id_t)
DEFINE_INTERFACE_METHOD_CONST_1("FindAllTemplates", std::vector<std::string>, ICmpTemplateManager, FindAllTemplates, bool)
DEFINE_INTERFACE_METHOD_CONST_1("FindAllPlaceableTemplates", std::vector<std::string>, ICmpTemplateManager, FindAllPlaceableTemplates, bool)
DEFINE_INTERFACE_METHOD_CONST_1("GetEntitiesUsingTemplate", std::vector<entity_id_t>, ICmpTemplateManager, GetEntitiesUsingTemplate, std::string)
END_INTERFACE_WRAPPER(TemplateManager)
