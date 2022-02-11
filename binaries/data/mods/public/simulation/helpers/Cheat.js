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
		cmpPlayer.SetPopulationBonuses((cmpPlayerManager.GetMaxWorldPopulation() || cmpPlayer.GetMaxPopulation()) + 500);
		return;
	case "changemaxpopulation":
	{
		let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
		cmpModifiersManager.AddModifiers("cheat/maxpopulation", {
			"Player/MaxPopulation": [{ "affects": ["Player"], "add": 500 }],
		}, playerEnt);
		return;
	}
	case "convertunit":
		const playerID = (input.parameter > -1 && QueryPlayerIDInterface(input.parameter) || cmpPlayer).GetPlayerID();
		for (let ent of input.selected)
		{
			let cmpOwnership = Engine.QueryInterface(ent, IID_Ownership);
			if (cmpOwnership)
				cmpOwnership.SetOwner(playerID);
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
		cmpPlayer = QueryPlayerIDInterface(input.parameter);
		if (cmpPlayer)
			cmpPlayer.SetState("defeated", markForTranslation("%(player)s has been defeated (cheat)."));
		return;
	case "createunits":
	{
		const cmpTrainer = input.selected.length && Engine.QueryInterface(input.selected[0], IID_Trainer);
		if (!cmpTrainer)
		{
			cmpGuiInterface.PushNotification({
				"type": "text",
				"players": [input.player],
				"message": markForTranslation("You need to select a building that trains units."),
				"translateMessage": true
			});
			return;
		}

		let owner = input.player;
		const cmpOwnership = Engine.QueryInterface(input.selected[0], IID_Ownership);
		if (cmpOwnership)
			owner = cmpOwnership.GetOwner();
		for (let i = 0; i < Math.min(input.parameter, cmpPlayer.GetMaxPopulation() - cmpPlayer.GetPopulationCount()); ++i)
		{
			const batch = new cmpTrainer.Item(input.templates[i % input.templates.length], 1, input.selected[0], null);
			batch.player = owner;
			batch.Finish();
			// ToDo: If not able to spawn, cancel the batch.
		}
		return;
	}
	case "fastactions":
	{
		let cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
		if (cmpModifiersManager.HasAnyModifier("cheat/fastactions", playerEnt))
			cmpModifiersManager.RemoveAllModifiers("cheat/fastactions", playerEnt);
		else
			cmpModifiersManager.AddModifiers("cheat/fastactions", {
				"Cost/BuildTime": [{ "affects": [["Structure"], ["Unit"]], "multiply": 0.01 }],
				"ResourceGatherer/BaseSpeed": [{ "affects": [["Structure"], ["Unit"]], "multiply": 1000 }],
				"Pack/Time": [{ "affects": [["Structure"], ["Unit"]], "multiply": 0.01 }],
				"Upgrade/Time": [{ "affects": [["Structure"], ["Unit"]], "multiply": 0.01 }],
				"Researcher/TechCostMultiplier/time": [{ "affects": [["Structure"], ["Unit"]], "multiply": 0.01 }]
			}, playerEnt);
		return;
	}
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

		if (TechnologyTemplates.Has(parameter + "_" + cmpPlayer.civ))
			parameter += "_" + cmpPlayer.civ;
		else
			parameter += "_generic";

		Cheat({ "player": input.player, "action": "researchTechnology", "parameter": parameter, "selected": input.selected });
		return;
	case "researchTechnology":
	{
		if (!input.parameter.length)
			return;

		var techname = input.parameter;
		var cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager);
		if (!cmpTechnologyManager)
			return;

		// check, if building is selected
		if (input.selected[0])
		{
			const cmpResearcher = Engine.QueryInterface(input.selected[0], IID_Researcher);
			if (cmpResearcher)
			{
				// try to spilt the input
				var tmp = input.parameter.split(/\s+/);
				var number = +tmp[0];
				var pair = tmp.length > 1 && (tmp[1] == "top" || tmp[1] == "bottom") ? tmp[1] : "top"; // use top as default value

				// check, if valid number was parsed.
				if (number || number === 0)
				{
					// get name of tech
					const techs = cmpResearcher.GetTechnologiesList();
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

		if (TechnologyTemplates.Has(techname))
			cmpTechnologyManager.ResearchTechnology(techname);
		return;
	}
	case "metaCheat":
		for (let resource of Resources.GetCodes())
			Cheat({ "player": input.player, "action": "addresource", "text": resource, "parameter": input.parameter });
		Cheat({ "player": input.player, "action": "maxpopulation" });
		Cheat({ "player": input.player, "action": "changemaxpopulation" });
		Cheat({ "player": input.player, "action": "fastactions" });
		for (let i=0; i<2; ++i)
			Cheat({ "player": input.player, "action": "changephase", "selected": input.selected });
		return;
	case "playRetro":
		let play = input.parameter.toLowerCase() != "off";
		cmpGuiInterface.PushNotification({
			"type": "play-tracks",
			"tracks": play && input.parameter.split(" "),
			"lock": play,
			"players": [input.player]
		});
		return;

	default:
		warn("Cheat '" + input.action + "' is not implemented");
		return;
	}
}

Engine.RegisterGlobal("Cheat", Cheat);
