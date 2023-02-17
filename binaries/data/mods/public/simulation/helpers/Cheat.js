function Cheat(input)
{
	if (input.player < 0)
		return;

	const cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	const playerEnt = cmpPlayerManager.GetPlayerByID(input.player);
	if (playerEnt == INVALID_ENTITY)
		return;
	const cmpPlayer = Engine.QueryInterface(playerEnt, IID_Player);
	if (!cmpPlayer?.GetCheatsEnabled())
		return;

	const cmpGuiInterface = Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface);

	switch(input.action)
	{
	case "addresource":
		if (isNaN(input.parameter))
			return;
		cmpPlayer.AddResource(input.text, input.parameter);
		return;
	case "revealmap":
		Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager).SetLosRevealAll(-1, true);
		return;
	case "maxpopulation":
		cmpPlayer.SetPopulationBonuses((cmpPlayerManager.GetMaxWorldPopulation() || cmpPlayer.GetMaxPopulation()) + 500);
		return;
	case "changemaxpopulation":
	{
		Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager).AddModifiers("cheat/maxpopulation", {
			"Player/MaxPopulation": [{ "affects": ["Player"], "add": 500 }],
		}, playerEnt);
		return;
	}
	case "convertunit":
		if (isNaN(input.parameter))
			return;
		const playerID = (input.parameter > -1 && QueryPlayerIDInterface(input.parameter) || cmpPlayer).GetPlayerID();
		for (const ent of input.selected)
			Engine.QueryInterface(ent, IID_Ownership)?.SetOwner(playerID);
		return;
	case "killunits":
		for (const ent of input.selected)
		{
			const cmpHealth = Engine.QueryInterface(ent, IID_Health);
			if (cmpHealth)
				cmpHealth.Kill();
			else
				Engine.DestroyEntity(ent);
		}
		return;
	case "defeatplayer":
		if (isNaN(input.parameter))
			return;
		QueryPlayerIDInterface(input.parameter)?.Defeat(
			markForTranslation("%(player)s has been defeated (cheat).")
		);
		return;
	case "createunits":
	{
		if (isNaN(input.player) || isNaN(input.parameter))
			return;

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
			if (!batch.IsFinished())
				batch.Stop();
		}
		return;
	}
	case "fastactions":
	{
		const cmpModifiersManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ModifiersManager);
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
	{
		const cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager);
		if (!cmpTechnologyManager)
			return;

		// store the phase we want in the next input parameter
		let parameter;
		if (!cmpTechnologyManager.IsTechnologyResearched("phase_town"))
			parameter = "phase_town";
		else if (!cmpTechnologyManager.IsTechnologyResearched("phase_city"))
			parameter = "phase_city";
		else
			return;

		const civ = Engine.QueryInterface(playerEnt, IID_Identity).GetCiv();
		parameter += TechnologyTemplates.Has(parameter + "_" + civ) ? "_" + civ : "_generic";

		Cheat({
			"player": input.player,
			"action": "researchTechnology",
			"parameter": parameter,
			"selected": input.selected
		});
		return;
	}
	case "researchTechnology":
	{
		if (!input.parameter.length)
			return;

		let techname = input.parameter;
		const cmpTechnologyManager = Engine.QueryInterface(playerEnt, IID_TechnologyManager);
		if (!cmpTechnologyManager)
			return;

		// check, if building is selected
		if (input.selected[0])
		{
			const cmpResearcher = Engine.QueryInterface(input.selected[0], IID_Researcher);
			if (cmpResearcher)
			{
				// try to spilt the input
				const tmp = input.parameter.split(/\s+/);
				const number = +tmp[0];
				const pair = tmp.length > 1 && (tmp[1] == "top" || tmp[1] == "bottom") ? tmp[1] : "top"; // use top as default value

				// check, if valid number was parsed.
				if (!isNaN(number))
				{
					// get name of tech
					const techs = cmpResearcher.GetTechnologiesList();
					if (number > 0 && number <= techs.length)
					{
						const tech = techs[number-1];
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
		for (const resource of Resources.GetCodes())
			Cheat({ "player": input.player, "action": "addresource", "text": resource, "parameter": input.parameter });
		Cheat({ "player": input.player, "action": "maxpopulation" });
		Cheat({ "player": input.player, "action": "changemaxpopulation" });
		Cheat({ "player": input.player, "action": "fastactions" });
		for (let i = 0; i < 2; ++i)
			Cheat({ "player": input.player, "action": "changephase", "selected": input.selected });
		return;
	case "playRetro":
		const play = input.parameter.toLowerCase() != "off";
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
