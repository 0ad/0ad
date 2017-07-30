// Little helper functions to make applying technology and auras more convenient

function ApplyValueModificationsToEntity(tech_type, current_value, entity)
{
	let value = current_value;
	// entity can be an owned entity or a player entity.
	let cmpTechnologyManager = Engine.QueryInterface(entity, IID_Player) ?
		Engine.QueryInterface(entity, IID_TechnologyManager) : QueryOwnerInterface(entity, IID_TechnologyManager);
	if (cmpTechnologyManager)
		value = cmpTechnologyManager.ApplyModifications(tech_type, current_value, entity);

	let cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	if (!cmpAuraManager)
		return value;
	return cmpAuraManager.ApplyModifications(tech_type, value, entity);
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
Engine.RegisterGlobal("ApplyValueModificationsToTemplate", ApplyValueModificationsToTemplate);
