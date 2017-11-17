{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.conquestClassFilter = "Unit+!Animal";
	cmpTrigger.conquestDefeatReason = markForTranslation("%(player)s has been defeated (lost all units).");
}
