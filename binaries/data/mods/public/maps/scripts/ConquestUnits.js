var cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
cmpTrigger.conquestClassFilter = "Unit";

var data = {"enabled": true};
cmpTrigger.RegisterTrigger("OnOwnershipChanged", "ConquestHandlerOwnerShipChanged", data);
cmpTrigger.RegisterTrigger("OnTrainingFinished", "ConquestTrainingFinished", data);

cmpTrigger.DoAfterDelay(0, "ConquestStartGameCount", null);
