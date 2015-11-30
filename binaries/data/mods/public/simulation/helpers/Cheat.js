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
		cmpGuiInterface.PushNotification({ "type": "chat", "players": [input.player], "message": "Cheats are disabled in this match" });
		return;
	}

	switch(input.action)
	{
	case "addresource":
		cmpPlayer.AddResource(input.text, input.parameter);
		return;
	case "revealmap":
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		cmpRangeManager.SetLosRevealAll(-1, true);
		return;
	case "maxpopulation":
		cmpPlayer.SetPopulationBonuses(500);
		return;
	case "changemaxpopulation":
		cmpPlayer.SetMaxPopulation(500);
		return;
	case "convertunit":
		for (let ent of input.selected)
		{
			let cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
			if (cmpOwnership)
				cmpOwnership.SetOwner(cmpPlayer.GetPlayerID());
		}
		return;
	case "killunits":
		for (let ent of input.selected)
		{
			let cmpHealth = Engine.QueryInterface(ent, IID_Health);
			if (cmpHealth)
				cmpHealth.Kill();
			else
				Engine.DestroyEntity(ent);
		}
		return;
	case "defeatplayer":
		var playerEnt = cmpPlayerManager.GetPlayerByID(input.parameter);
		if (playerEnt == INVALID_ENTITY)
			return;
		Engine.PostMessage(playerEnt, MT_PlayerDefeated, { "playerId": input.parameter });
		return;
	case "createunits":
		if (!input.selected[0])
		{
			cmpGuiInterface.PushNotification({ "type": "chat", "players": [input.player], "message": "You need to select a building that trains units." });
			return;
		}

		var cmpProductionQueue = Engine.QueryInterface(input.selected[0], IID_ProductionQueue);
		if (!cmpProductionQueue)
		{
			cmpGuiInterface.PushNotification({ "type": "chat", "players": [input.player], "message": "You need to select a building that trains units." });
			return;
		}
		for (let i = 0; i < input.parameter; ++i)
			cmpProductionQueue.SpawnUnits(input.templates[i % input.templates.length], 1, null);
		return;
	case "fastactions":
		cmpPlayer.SetCheatTimeMultiplier((cmpPlayer.GetCheatTimeMultiplier() == 1) ? 0.01 : 1);
		return;
	case "changespeed":
		cmpPlayer.SetCheatTimeMultiplier(input.parameter);
		return;
	case "changephase":
		var cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager);
		if (!cmpTechnologyManager)
			return;

		// store the phase we want in the next input parameter
		input.parameter = "phase_";
		if (!cmpTechnologyManager.IsTechnologyResearched("phase_town"))
			input.parameter += "town";
		else if (!cmpTechnologyManager.IsTechnologyResearched("phase_city"))
			input.parameter += "city";
		else
			return;

		// check if specialised tech exists (like phase_town_athen)
		var cmpTechnologyTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TechnologyTemplateManager);
		if (cmpTechnologyTemplateManager.ListAllTechs().indexOf(input.parameter + "_" + cmpPlayer.civ) > -1)
			input.parameter += "_" + cmpPlayer.civ;

		Cheat({ "player": input.player, "action": "researchTechnology", "parameter": input.parameter, "selected": input.selected });
		return;
	case "researchTechnology":
		// check, if name of technology is given
		if (input.parameter.length == 0)
		{
			cmpGuiInterface.PushNotification({ "type": "chat", "players": [input.player], "message": "You have to enter the name of a technology or select a building and enter the number of the technology (brainiac number [top|paired].)" });
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
							cmpGuiInterface.PushNotification({ "type": "chat", "players": [input.player], "message": "You have already researched this technology." });
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
						cmpGuiInterface.PushNotification({ "type": "chat", "players": [input.player], "message": "This building only has " + techs.length + " technologies." });
						return;
					}
				}
			}
		}

		// check, if technology exists
		var template = cmpTechnologyManager.GetTechnologyTemplate(techname);
		if (!template)
		{
			cmpGuiInterface.PushNotification({ "type": "chat", "players": [input.player], "message": "Technology \"" + techname + "\" does not exist" });
			return;
		}

		// check, if technology is already researched
		if (!cmpTechnologyManager.IsTechnologyResearched(techname))
			cmpTechnologyManager.ResearchTechnology(techname);
		return;
	case "metaCheat":
		for (let resource of ["food", "wood", "metal", "stone"])
			Cheat({ "player": input.player, "action": "addresource", "text": resource, "parameter": input.parameter });
		Cheat({ "player": input.player, "action": "maxpopulation" });
		Cheat({ "player": input.player, "action": "changemaxpopulation" });
		Cheat({ "player": input.player, "action": "fastactions" });
		for (let i=0; i<2; ++i)
			Cheat({ "player": input.player, "action": "changephase", "selected": input.selected });
		return;
	default:
		warn("Cheat '" + input.action + "' is not implemented");
		return;
	}
}

Engine.RegisterGlobal("Cheat", Cheat);
