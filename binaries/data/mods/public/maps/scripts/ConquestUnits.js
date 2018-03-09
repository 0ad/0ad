{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.ConquestAddVictoryCondition({
		"classFilter": "Unit+!Animal",
		"defeatReason": markForTranslation("%(player)s has been defeated (lost all units).")
	});
}
