// Little helper functions to make applying tehnology more covenient

function ApplyTechModificationsToEntity(tech_type, current_value, entity) {
  var cmpTechMan = QueryOwnerInterface(IID_TechnologyManager, entity);

  if (!cmpTechMan)
    return current_value;

  return cmpTechMan.ApplyModifications(entity, tech_type, current_value); 
}

function ApplyTechModificationsToPlayer(tech_type, current_value, player_entity) {
  var cmpTechMan = Engine.QueryInterface(IID_TechnologyManager, player_entity);

  if (!cmpTechMan)
    return current_value;

  return cmpTechMan.ApplyModifications(player_entity, tech_type, current_value); 
}

Engine.RegisterGlobal("ApplyTechModificationsToEntity", ApplyTechModificationsToEntity);
Engine.RegisterGlobal("ApplyTechModificationsToPlayer", ApplyTechModificationsToPlayer);

