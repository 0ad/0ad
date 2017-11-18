{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.conquestClassFilter = "Structure";
	cmpTrigger.conquestDefeatReason = markForTranslation("%(player)s has been defeated (lost all structures).");
}
