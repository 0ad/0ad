

function Cheat(input)
{
	//computing the neccessary components
	var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (!cmpPlayerMan || input.player < 0)
		return;
	var playerEnt = cmpPlayerMan.GetPlayerByID(input.player);
	if (playerEnt == INVALID_ENTITY)
		return;
	var cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);
	if (!cmpPlayer)
		return;
	if (cmpPlayer.GetCheatEnabled())
	{
		if (input.action == "addfood")
		{
			cmpPlayer.AddResource("food", input.number);
		}
		else if (input.action == "addwood")
		{
			cmpPlayer.AddResource("wood", input.number);
		}
		else if (input.action == "addmetal")
		{
			cmpPlayer.AddResource("metal", input.number);
		}
		else if (input.action == "addstone")
		{
			cmpPlayer.AddResource("stone", input.number);
		}
		else if (input.action == "revealmap")
		{
			var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
			cmpRangeManager.SetLosRevealAll(-1, true);
		}
		else if (input.action == "maxpopulation")
		{
			cmpPlayer.popBonuses += 500;
		}
		else if (input.action == "changemaxpopulation")
		{
			//this changes the max population limit
			cmpPlayer.maxPop = 500;
		}
		else if (input.action == "convertunit")
		{
			for each (var ent in input.selected)
			{
				var cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
				cmpOwnership.SetOwner(cmpPlayer.playerID);
			}
		}
		else if (input.action == "killunits")
		{
			for each (var ent in input.selected)
			{
				var cmpHealth = Engine.QueryInterface(ent, IID_Health);
				if (cmpHealth)
					cmpHealth.Kill();
				else
					Engine.DestroyEntity(ent);
			}
		}
		else if (input.action == "defeatplayer")
		{
			var cmpPlayerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
			if (!cmpPlayerMan)
				return;
			var playerEnt = cmpPlayerMan.GetPlayerByID(input.number);
			if (playerEnt == INVALID_ENTITY)
				return;
			Engine.PostMessage(playerEnt, MT_PlayerDefeated, { "playerId": input.number } );
		}
		else if (input.action == "createunits")
		{	
			//the player must select a building that can train units for this.
			var cmpProductionQueue = Engine.QueryInterface(input.selected[0], IID_ProductionQueue);
			if (!cmpProductionQueue)
				return;
			for (var i = 0; i < input.number; i++)
			{
				cmpProductionQueue.SpawnUnits (input.templates[i%(input.templates.length)],1, null);
			}
		}
		else if (input.action == "fastactions")
		{	
			if (cmpPlayer.cheatTimeMultiplier == 1)
			{
				cmpPlayer.cheatTimeMultiplier = 0.01;
			}
			else
			{
				cmpPlayer.cheatTimeMultiplier = 1;
			}
		}
		//AI only
		else if (input.action == "changespeed")
		{	
			cmpPlayer.cheatTimeMultiplier = input.number;
		}
		if (cmpPlayer.name.indexOf(" the Cheater")==-1)
			cmpPlayer.name = cmpPlayer.name + " the Cheater";
	}
	else
	{
		var cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);
		cmpGuiInterface.PushNotification({"type": "chat", "player": input.player, "message": "Cheats are disbaled in this match"});
	}
}

Engine.RegisterGlobal("Cheat", Cheat);