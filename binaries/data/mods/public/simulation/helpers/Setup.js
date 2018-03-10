
/**
 * Used to initialize non-player settings relevant to the map, like
 * default stance and victory conditions. DO NOT load players here
 */
function LoadMapSettings(settings)
{
	if (!settings)
		settings = {};

	if (settings.DefaultStance)
	{
		for (let ent of Engine.GetEntitiesWithInterface(IID_UnitAI))
		{
			let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			cmpUnitAI.SwitchToStance(settings.DefaultStance);
		}
	}

	if (settings.RevealMap)
	{
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetLosRevealAll(-1, true);
	}

	if (settings.DisableTreasures)
		for (let ent of Engine.GetEntitiesWithInterface(IID_ResourceSupply))
		{
			let cmpResourceSupply = Engine.QueryInterface(ent, IID_ResourceSupply);
			if (cmpResourceSupply.GetType().generic == "treasure")
				Engine.DestroyEntity(ent);
		}

	if (settings.CircularMap)
	{
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetLosCircular(true);

		let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
		if (cmpObstructionManager)
			cmpObstructionManager.SetPassabilityCircular(true);
	}

	if (settings.TriggerDifficulty != undefined)
		Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger).SetDifficulty(settings.TriggerDifficulty);
	else if (settings.SupportedTriggerDifficulties)	// used by Atlas and autostart games
	{
		let difficulties = Engine.ReadJSONFile("simulation/data/settings/trigger_difficulties.json").Data;
		let defaultDiff = difficulties.find(d => d.Name == settings.SupportedTriggerDifficulties.Default).Difficulty;
		Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger).SetDifficulty(defaultDiff);
	}

	let cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	let gameSettings = { "victoryConditions": settings.VictoryConditions };
	if (gameSettings.victoryConditions.indexOf("capture_the_relic") != -1)
	{
		gameSettings.relicCount = settings.RelicCount;
		gameSettings.relicDuration = settings.RelicDuration * 60 * 1000;
	}
	if (gameSettings.victoryConditions.indexOf("wonder") != -1)
		gameSettings.wonderDuration = settings.WonderDuration * 60 * 1000;
	if (gameSettings.victoryConditions.indexOf("regicide") != -1)
		gameSettings.regicideGarrison = settings.RegicideGarrison;
	cmpEndGameManager.SetGameSettings(gameSettings);

	cmpEndGameManager.SetAlliedVictory(settings.LockTeams || !settings.LastManStanding);
	if (settings.LockTeams && settings.LastManStanding)
		warn("Last man standing is only available in games with unlocked teams!");

	if (settings.Garrison)
		for (let holder in settings.Garrison)
		{
			let cmpGarrisonHolder = Engine.QueryInterface(+holder, IID_GarrisonHolder);
			if (!cmpGarrisonHolder)
				warn("Map error in Setup.js: entity " + holder + " can not garrison units");
			else
				cmpGarrisonHolder.initGarrison = settings.Garrison[holder];
		}

	let cmpCeasefireManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_CeasefireManager);
	if (settings.Ceasefire)
		cmpCeasefireManager.StartCeasefire(settings.Ceasefire * 60 * 1000);
}

Engine.RegisterGlobal("LoadMapSettings", LoadMapSettings);
