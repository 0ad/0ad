// Little helper functions to make applying tehnology more covenient

function ApplyTechModificationsToEntity(tech_type, current_value, entity)
{
	var cmpTechMan = QueryOwnerInterface(entity, IID_TechnologyManager);

	if (!cmpTechMan)
		return current_value;

	return cmpTechMan.ApplyModifications(tech_type, current_value, entity);
}

function ApplyTechModificationsToPlayer(tech_type, current_value, player_entity)
{
	var cmpTechMan = Engine.QueryInterface(player_entity, IID_TechnologyManager);

	if (!cmpTechMan)
		return current_value;

	return cmpTechMan.ApplyModifications(tech_type, current_value, player_entity);
}

function ApplyTechModificationsToTemplate(tech_type, current_value, owner_entity, template)
{
	var cmpTechMan = QueryOwnerInterface(owner_entity, IID_TechnologyManager);

	if (!cmpTechMan)
		return current_value;

	return cmpTechMan.ApplyModificationsTemplate(tech_type, current_value, template);
}

Engine.RegisterGlobal("ApplyTechModificationsToEntity", ApplyTechModificationsToEntity);
Engine.RegisterGlobal("ApplyTechModificationsToPlayer", ApplyTechModificationsToPlayer);
Engine.RegisterGlobal("ApplyTechModificationsToTemplate", ApplyTechModificationsToTemplate);
