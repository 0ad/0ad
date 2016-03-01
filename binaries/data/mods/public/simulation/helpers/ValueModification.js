// Little helper functions to make applying technology and auras more convenient

function ApplyValueModificationsToEntity(tech_type, current_value, entity)
{
	let value = current_value;
	let cmpTechnologyManager = QueryOwnerInterface(entity, IID_TechnologyManager);
	if (cmpTechnologyManager)
		value = cmpTechnologyManager.ApplyModifications(tech_type, current_value, entity);

	let cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	if (!cmpAuraManager)
		return value;
	return cmpAuraManager.ApplyModifications(tech_type, value, entity);
}

function ApplyValueModificationsToPlayer(tech_type, current_value, playerEntity, playerID)
{
	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let entityTemplateName = cmpTemplateManager.GetCurrentTemplateName(playerEntity);
	let entityTemplate = cmpTemplateManager.GetTemplate(entityTemplateName);
	return ApplyValueModificationsToTemplate(tech_type, current_value, playerID, entityTemplate);
}

function ApplyValueModificationsToTemplate(tech_type, current_value, playerID, template)
{
	let value = current_value;
	let cmpTechnologyManager = QueryPlayerIDInterface(playerID, IID_TechnologyManager);
	if (cmpTechnologyManager)
		value = cmpTechnologyManager.ApplyModificationsTemplate(tech_type, current_value, template);

	let cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager); 
	if (!cmpAuraManager)
		return value; 
	return cmpAuraManager.ApplyTemplateModifications(tech_type, value, playerID, template);
}

Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
Engine.RegisterGlobal("ApplyValueModificationsToPlayer", ApplyValueModificationsToPlayer);
Engine.RegisterGlobal("ApplyValueModificationsToTemplate", ApplyValueModificationsToTemplate);
