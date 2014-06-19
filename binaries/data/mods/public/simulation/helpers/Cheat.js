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
		cmpGuiInterface.PushNotification({"type": "chat", "players": [input.player], "message": "Cheats are disbaled in this match"});
		return;
	}

	switch(input.action)
	{
	case "addresource":
		// force input.text to be an array
		input.text = [].concat(input.text);
		for each (var type in input.text)
			cmpPlayer.AddResource(type, input.parameter);
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
		var playerEnt = cmpPlayerManager.GetPlayerByID(input.parameter);
		if (playerEnt == INVALID_ENTITY)
			return;
		Engine.PostMessage(playerEnt, MT_PlayerDefeated, { "playerId": input.parameter } );
		break;
	case "createunits":
		if (!input.selected[0])
		{
			cmpGuiInterface.PushNotification({"type": "notification", "players": [input.player], "message": "You need to select a building that trains units."});
			return;
		}

		var cmpProductionQueue = Engine.QueryInterface(input.selected[0], IID_ProductionQueue);
		if (!cmpProductionQueue)
		{
			cmpGuiInterface.PushNotification({"type": "notification", "players": [input.player], "message": "You need to select a building that trains units."});
			return;
		}
		for (var i = 0; i < input.parameter; i++)
			cmpProductionQueue.SpawnUnits (input.templates[i%(input.templates.length)],1, null);
		break;
	case "fastactions":
		cmpPlayer.SetCheatTimeMultiplier((cmpPlayer.GetCheatTimeMultiplier() == 1) ? 0.01 : 1);
		break;
	case "changespeed":
		cmpPlayer.SetCheatTimeMultiplier(input.parameter);
		break;
	case "changephase": 
		var cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager); 
		if (!cmpTechnologyManager)
			return;

		// get phase we want to go
		var phaseToGo;
		var version = "generic";
		var special;
		if (!cmpTechnologyManager.IsTechnologyResearched("phase_town"))
		{
			phaseToGo = "town";
			special = {"athen":"athen"};
		}
		else if (!cmpTechnologyManager.IsTechnologyResearched("phase_city"))
		{
			phaseToGo = "city";
			special = {"athen":"athen", "celt": Math.round(Math.random()) == 0 ? "britons":"gauls"};
		}
		else
			return;

		// check, if special civ
		if (cmpPlayer.civ in special)
			version = special[cmpPlayer.civ];
			
		// rewrite input and call function
		input.action = "researchTechnology";
		input.parameter="phase_" + phaseToGo + "_" + version; 
		Cheat(input);
		break; 
	case "researchTechnology": 
		// check, if name of technology is given
		if (input.parameter.length == 0)
		{
			cmpGuiInterface.PushNotification({"type": "notification", "players": [input.player], "message": "You have to enter the name of a technology or select a building and enter the number of the technology (brainiac number [top|paired].)"});
			return;
		}
		var techname = input.parameter;
		var cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager); 
		if (!cmpTechnologyManager)
			return;
			
		// check, if building is selected
		if (input.selected[0])
		{
			var cmpProductionQueue = Engine.QueryInterface(input.selected[0], IID_ProductionQueue);
			if (cmpProductionQueue)
			{
				// try to spilt the input
				var tmp = input.parameter.split(/\s+/);
				var number = +tmp[0];
				var pair = tmp.length > 1 && (tmp[1] == "top" || tmp[1] == "bottom") ? tmp[1] : "top"; // use top as default value

				// check, if valid number was parsed.
				if (number || number === 0)
				{
					// get name of tech
					var techs = cmpProductionQueue.GetTechnologiesList();
					if (number > 0 && number <= techs.length)
					{
						var tech = techs[number-1];
						if (!tech)
						{
							cmpGuiInterface.PushNotification({"type": "notification", "players": [input.player], "message": "You have already researched this technology."});
							return;
						}
						// get name of tech
						if (tech.pair)
							techname = tech[pair];
						else
							techname = tech;
					}
					else
					{
						cmpGuiInterface.PushNotification({"type": "notification", "players": [input.player], "message": "This building only has " + techs.length + " technologies."});
						return;
					}
				}
			}	
		}

		// check, if technology exists
		var template = cmpTechnologyManager.GetTechnologyTemplate(techname);
		if (!template)
		{
			cmpGuiInterface.PushNotification({"type": "notification", "players": [input.player], "message": "Technology \"" + techname + "\" does not exist"});
			return;
		}

		// check, if technology is already researched
		if (!cmpTechnologyManager.IsTechnologyResearched(techname))
			cmpTechnologyManager.ResearchTechnology(techname); 
		break; 
	default:
		warn("Cheat '" + input.action + "' is not implemented");
		break;
	}
}

Engine.RegisterGlobal("Cheat", Cheat);
