{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.conquestClassFilter = "ConquestCritical";
	cmpTrigger.conquestDefeatReason = markForTranslation("%(player)s has been defeated (lost all critical units and structures).");
}
