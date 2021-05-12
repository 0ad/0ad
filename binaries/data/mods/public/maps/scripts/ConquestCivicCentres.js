{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.ConquestAddVictoryCondition({
		"classFilter": "CivilCentre+!Foundation",
		"defeatReason": markForTranslation("%(player)s has been defeated (lost all civic centres).")
	});
	cmpTrigger.ConquestAddVictoryCondition({
		"classFilter": "ConquestCritical CivilCentre+!Foundation",
		"defeatReason": markForTranslation("%(player)s has been defeated (lost all civic centres and critical units and structures).")
	});
}
