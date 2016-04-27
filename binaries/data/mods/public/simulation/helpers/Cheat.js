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
		return;

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
		var cmpProductionQueue = input.selected.length && Engine.QueryInterface(input.selected[0], IID_ProductionQueue);
		if (!cmpProductionQueue)
		{
			cmpGuiInterface.PushNotification({
				"type": "text",
				"players": [input.player],
				"message": markForTranslation("You need to select a building that trains units."),
				"translateMessage": true
			});
			return;
		}

		for (let i = 0; i < Math.min(input.parameter, cmpPlayer.GetMaxPopulation() - cmpPlayer.GetPopulationCount()); ++i)
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
		var parameter;
		if (!cmpTechnologyManager.IsTechnologyResearched("phase_town"))
			parameter = "phase_town";
		else if (!cmpTechnologyManager.IsTechnologyResearched("phase_city"))
			parameter = "phase_city";
		else
			return;

		// check if specialised tech exists (like phase_town_athen)
		var cmpDataTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_DataTemplateManager);
		if (cmpDataTemplateManager.ListAllTechs().indexOf(parameter + "_" + cmpPlayer.civ) > -1)
			parameter += "_" + cmpPlayer.civ;

		Cheat({ "player": input.player, "action": "researchTechnology", "parameter": parameter, "selected": input.selected });
		return;
	case "researchTechnology":
		if (!input.parameter.length)
			return;

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
							return;

						// get name of tech
						if (tech.pair)
							techname = tech[pair];
						else
							techname = tech;
					}
					else
						return;
				}
			}
		}

		// check, if technology exists
		var template = cmpTechnologyManager.GetTechnologyTemplate(techname);
		if (!template)
			return;

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
