{
	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.conquestClassFilter = "Unit";
	cmpTrigger.conquestDefeatReason = markForTranslation("%(player)s has been defeated (lost all workers).");

	let data = { "enabled": true };
	cmpTrigger.RegisterTrigger("OnOwnershipChanged", "ConquestHandlerOwnerShipChanged", data);
	cmpTrigger.RegisterTrigger("OnTrainingFinished", "ConquestTrainingFinished", data);

	cmpTrigger.DoAfterDelay(0, "ConquestStartGameCount", null);
}
