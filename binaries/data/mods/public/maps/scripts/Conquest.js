{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.ConquestAddVictoryCondition({
		"classFilter": "ConquestCritical",
		"defeatReason": markForTranslation("%(player)s has been defeated (lost all critical units and structures).")
	});
}
