// Little helper functions to make applying tehnology more covenient

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

function ApplyValueModificationsToPlayer(tech_type, current_value, player_entity)
{
	let cmpTechnologyManager = Engine.QueryInterface(player_entity, IID_TechnologyManager);

	if (!cmpTechnologyManager)
		return current_value;

	return cmpTechnologyManager.ApplyModifications(tech_type, current_value, player_entity);
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
