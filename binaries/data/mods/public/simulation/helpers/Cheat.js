function Cheat(input)
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (!cmpPlayerManager || input.player < 0)
		return;
	var playerEnt = cmpPlayerManager.GetPlayerByID(input.player);
	if (playerEnt == INVALID_ENTITY)
		return;
	var cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);
	if (!cmpPlayer)
		return;

	var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
	if (!cmpPlayer.GetCheatsEnabled())
	{
		cmpGuiInterface.PushNotification({"type": "chat", "player": input.player, "message": "Cheats are disbaled in this match"});
		return;
	}

	switch(input.action)
	{
	case "addresource":
		cmpPlayer.AddResource(input.text, input.number);
		break;
	case "revealmap":
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		cmpRangeManager.SetLosRevealAll(-1, true);
		break;
	case "maxpopulation":
		cmpPlayer.SetPopulationBonuses(500);
		break;
	case "changemaxpopulation":
		cmpPlayer.SetMaxPopulation(500);
		break;
	case "convertunit":
		for each (var ent in input.selected)
		{
			var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
			cmpOwnership.SetOwner(cmpPlayer.GetPlayerID());
		}
		break;
	case "killunits":
		for each (var ent in input.selected)
		{
			var cmpHealth = Engine.QueryInterface(ent, IID_Health);
			if (cmpHealth)
				cmpHealth.Kill();
			else
				Engine.DestroyEntity(ent);
		}
		break;
	case "defeatplayer":
		var playerEnt = cmpPlayerManager.GetPlayerByID(input.number);
		if (playerEnt == INVALID_ENTITY)
			return;
		Engine.PostMessage(playerEnt, MT_PlayerDefeated, { "playerId": input.number } );
		break;
	case "createunits":
		if (!input.selected[0])
		{
			cmpGuiInterface.PushNotification({"type": "notification", "player": input.player, "message": "You need to select a building that trains units."});
			return;
		}

		var cmpProductionQueue = Engine.QueryInterface(input.selected[0], IID_ProductionQueue);
		if (!cmpProductionQueue)
		{
			cmpGuiInterface.PushNotification({"type": "notification", "player": input.player, "message": "You need to select a building that trains units."});
			return;
		}
		for (var i = 0; i < input.number; i++)
			cmpProductionQueue.SpawnUnits (input.templates[i%(input.templates.length)],1, null);
		break;
	case "fastactions":
		cmpPlayer.SetCheatTimeMultiplier((cmpPlayer.GetCheatTimeMultiplier() == 1) ? 0.01 : 1);
		break;
	case "changespeed":
		cmpPlayer.SetCheatTimeMultiplier(input.number);
		break;
	default:
		warn("Cheat '" + input.action + "' is not implemented");
		break;
	}
}

Engine.RegisterGlobal("Cheat", Cheat);
