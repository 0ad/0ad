
/**
 * Used to initialize non-player settings relevant to the map, like
 * default stance and victory conditions. DO NOT load players here
 */
function LoadMapSettings(settings)
{
	if (!settings)
		settings = {};

	if (settings.DefaultStance)
		for (const ent of Engine.GetEntitiesWithInterface(IID_UnitAI))
		{
			const cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			cmpUnitAI.SwitchToStance(settings.DefaultStance);
		}

	if (settings.RevealMap)
	{
		const cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetLosRevealAll(-1, true);
	}

	if (settings.DisableTreasures)
		for (const ent of Engine.GetEntitiesWithInterface(IID_Treasure))
			Engine.DestroyEntity(ent);

	const cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (cmpRangeManager)
		cmpRangeManager.SetLosCircular(!!settings.CircularMap);

	const cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	if (cmpObstructionManager)
		cmpObstructionManager.SetPassabilityCircular(!!settings.CircularMap);

	if (settings.TriggerDifficulty !== undefined)
		Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger).SetDifficulty(settings.TriggerDifficulty);
	else if (settings.SupportedTriggerDifficulties)	// used by Atlas and autostart games
	{
		let difficulties = Engine.ReadJSONFile("simulation/data/settings/trigger_difficulties.json").Data;
		let defaultDiff;
		if (settings.SupportedTriggerDifficulties.Default)
			defaultDiff = difficulties.find(d => d.Name == settings.SupportedTriggerDifficulties.Default).Difficulty;
		else
			defaultDiff = difficulties.find(d => d.Default).Difficulty;
		Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger).SetDifficulty(defaultDiff);
	}

	const cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	const gameSettings = { "victoryConditions": clone(settings.VictoryConditions) };
	if (gameSettings.victoryConditions.indexOf("capture_the_relic") != -1)
	{
		gameSettings.relicCount = (settings.RelicCount ?? 1);
		gameSettings.relicDuration = (settings.RelicDuration ?? 1) * 60 * 1000;
	}
	if (gameSettings.victoryConditions.indexOf("wonder") != -1)
		gameSettings.wonderDuration = (settings.WonderDuration ?? 1) * 60 * 1000;
	if (gameSettings.victoryConditions.indexOf("regicide") != -1)
		gameSettings.regicideGarrison = settings.RegicideGarrison;
	cmpEndGameManager.SetGameSettings(gameSettings);

	cmpEndGameManager.SetAlliedVictory(settings.LockTeams || !settings.LastManStanding);
	if (settings.LockTeams && settings.LastManStanding)
		warn("Last man standing is only available in games with unlocked teams!");

	const cmpCeasefireManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_CeasefireManager);
	if (settings.Ceasefire)
		cmpCeasefireManager.StartCeasefire((settings.Ceasefire ?? 1) * 60 * 1000);
}

Engine.RegisterGlobal("LoadMapSettings", LoadMapSettings);
