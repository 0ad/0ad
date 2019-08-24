// Little helper functions to make applying technology and auras more convenient

function ApplyValueModificationsToEntity(tech_type, current_value, entity)
{
	let value = current_value;

	// entity can be an owned entity or a player entity.
	let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
	if (cmpModifiersManager)
		value = cmpModifiersManager.ApplyModifiers(tech_type, current_value, entity);
	return value;
}

function ApplyValueModificationsToTemplate(tech_type, current_value, playerID, template)
{
	let value = current_value;
	let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
	if (cmpModifiersManager)
		value = cmpModifiersManager.ApplyTemplateModifiers(tech_type, current_value, template, playerID);
	return value;
}

Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
Engine.RegisterGlobal("ApplyValueModificationsToTemplate", ApplyValueModificationsToTemplate);
