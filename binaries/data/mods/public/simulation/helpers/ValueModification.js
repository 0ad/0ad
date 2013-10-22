// Little helper functions to make applying tehnology more covenient

function ApplyValueModificationsToEntity(tech_type, current_value, entity)
{
	var cmpTechMan = QueryOwnerInterface(entity, IID_TechnologyManager);
	if (cmpTechMan)
		var value = cmpTechMan.ApplyModifications(tech_type, current_value, entity);
	else
		var value = current_value;

	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager);
	if (!cmpAuraManager)
	    return value;
	return cmpAuraManager.ApplyModifications(tech_type, value, entity);
}

function ApplyValueModificationsToPlayer(tech_type, current_value, player_entity)
{
	var cmpTechMan = Engine.QueryInterface(player_entity, IID_TechnologyManager);

	if (!cmpTechMan)
		return current_value;

	return cmpTechMan.ApplyModifications(tech_type, current_value, player_entity);
}

function ApplyValueModificationsToTemplate(tech_type, current_value, playerID, template)
{
	var cmpTechMan = QueryPlayerIDInterface(playerID, IID_TechnologyManager);
	if (cmpTechMan)
		var value = cmpTechMan.ApplyModificationsTemplate(tech_type, current_value, template);
	else
		var value = current_value; 

	var cmpAuraManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AuraManager); 
	if (!cmpAuraManager)
		return value; 
	return cmpAuraManager.ApplyTemplateModifications(tech_type, value, playerID, template);
}

Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
Engine.RegisterGlobal("ApplyValueModificationsToPlayer", ApplyValueModificationsToPlayer);
Engine.RegisterGlobal("ApplyValueModificationsToTemplate", ApplyValueModificationsToTemplate);
