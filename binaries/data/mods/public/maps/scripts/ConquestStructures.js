{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.conquestClassFilter = "Structure";
	cmpTrigger.conquestDefeatReason = markForTranslation("%(player)s has been defeated (lost all structures).");

	let data = { "enabled": true };
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "ConquestHandlerOwnerShipChanged", data);
	cmpTrigger.RegisterTrigger("OnStructureBuilt", "ConquestAddStructure", data);

	cmpTrigger.DoAfterDelay(0, "ConquestStartGameCount", null);
}
