/**
 * Called when the map has been loaded, but before the simulation has started.
 * Only called when a new game is started, not when loading a saved game.
 */
function PreInitGame()
{
	Engine.BroadcastMessage(MT_SkirmishReplace, {});

	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let playerIds = cmpPlayerManager.GetAllPlayerEntities().slice(1); // ignore gaia
	for (let playerId of playerIds)
	{
		let cmpTechnologyManager = Engine.QueryInterface(playerId, IID_TechnologyManager);
		if (cmpTechnologyManager)
			cmpTechnologyManager.UpdateAutoResearch();
	}
}

function InitGame(settings)
{
	// No settings when loading a map in Atlas, so do nothing
	if (!settings)
		return;

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (settings.ExploreMap)
		for (var i = 0; i < settings.PlayerData.length; i++)
			cmpRangeManager.ExploreAllTiles(i+1);
	else
		// Explore the map only inside the players' territory borders
		cmpRangeManager.ExploreTerritories();

	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let cmpAIManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AIManager);
	for (let i = 0; i < settings.PlayerData.length; ++i)
	{
		let cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(i+1), IID_Player);
		cmpPlayer.SetCheatsEnabled(!!settings.CheatsEnabled);
		if (settings.PlayerData[i] && settings.PlayerData[i].AI && settings.PlayerData[i].AI != "")
		{
			cmpAIManager.AddPlayer(settings.PlayerData[i].AI, i+1, +settings.PlayerData[i].AIDiff);
			cmpPlayer.SetAI(true);
			// Sandbox: 50%, very easy: 50%, easy: 66%, Medium: 100%, hard: 133%, very hard: 166%
			cmpPlayer.SetGatherRateMultiplier(Math.max(0.5,(+settings.PlayerData[i].AIDiff)/3.0));
		}
		if (settings.PopulationCap)
			cmpPlayer.SetMaxPopulation(settings.PopulationCap);

		if (settings.mapType !== "scenario" && settings.StartingResources)
		{
			let resourceCounts = cmpPlayer.GetResourceCounts();
			let newResourceCounts = {};
			for (let resouces in resourceCounts)
				newResourceCounts[resouces] = settings.StartingResources;
			cmpPlayer.SetResourceCounts(newResourceCounts);
		}
	}
	let seed = settings.AISeed ? settings.AISeed : 0;
	cmpAIManager.SetRNGSeed(seed);
	cmpAIManager.TryLoadSharedComponent();
	cmpAIManager.RunGamestateInit();
}

Engine.RegisterGlobal("PreInitGame", PreInitGame);
Engine.RegisterGlobal("InitGame", InitGame);
