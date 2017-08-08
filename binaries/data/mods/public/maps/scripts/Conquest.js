{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.conquestClassFilter = "ConquestCritical";
	cmpTrigger.conquestDefeatReason = markForTranslation("%(player)s has been defeated (lost all workers and structures).");

	let data = { "enabled": true };
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "ConquestHandlerOwnerShipChanged", data);
	cmpTrigger.RegisterTrigger("OnStructureBuilt", "ConquestAddStructure", data);
	cmpTrigger.RegisterTrigger("OnTrainingFinished", "ConquestTrainingFinished", data);

	cmpTrigger.DoAfterDelay(0, "ConquestStartGameCount", null);
}
