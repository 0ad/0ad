/**
 * Called when the map has been loaded, but before the simulation has started.
 * Only called when a new game is started, not when loading a saved game.
 */
function PreInitGame()
{
	// We need to replace skirmish "default" entities with real ones.
	// This needs to happen before AI initialization (in InitGame).
	// And we need to flush destroyed entities otherwise the AI gets the wrong game state in 
	// the beginning and a bunch of "destroy" messages on turn 0, which just shouldn't happen.
	Engine.BroadcastMessage(MT_SkirmishReplace, {});
	Engine.FlushDestroyedEntities(); 

	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let playerIds = cmpPlayerManager.GetAllPlayerEntities().slice(1); // ignore gaia
	for (let playerId of playerIds)
	{
		let cmpTechnologyManager = Engine.QueryInterface(playerId, IID_TechnologyManager);
		if (cmpTechnologyManager)
			cmpTechnologyManager.UpdateAutoResearch();
	}

	// Explore the map inside the players' territory borders
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	cmpRangeManager.ExploreTerritories();
}

function InitGame(settings)
{
	// No settings when loading a map in Atlas, so do nothing
	if (!settings)
	{
		// Map dependent initialisations of components (i.e. garrisoned units)
		Engine.BroadcastMessage(MT_InitGame, {});
		return;
	}

	if (settings.ExploreMap)
	{
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		for (let i = 1; i < settings.PlayerData.length; ++i)
			cmpRangeManager.ExploreAllTiles(i);
	}

	// Sandbox, Very Easy, Easy, Medium, Hard, Very Hard
	let rate = [ 0.50, 0.64, 0.80, 1.00, 1.25, 1.56 ];
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	let cmpAIManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AIManager);
	for (let i = 1; i < settings.PlayerData.length; ++i)
	{
		let cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(i), IID_Player);
		cmpPlayer.SetCheatsEnabled(!!settings.CheatsEnabled);
		if (settings.PlayerData[i] && settings.PlayerData[i].AI && settings.PlayerData[i].AI != "")
		{
			let AIDiff = +settings.PlayerData[i].AIDiff;
			cmpAIManager.AddPlayer(settings.PlayerData[i].AI, i, AIDiff);
			cmpPlayer.SetAI(true);
			AIDiff = Math.min(AIDiff, rate.length - 1);
			cmpPlayer.SetGatherRateMultiplier(rate[AIDiff]);
			cmpPlayer.SetTradeRateMultiplier(rate[AIDiff]);
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
	// Map or player data (handicap...) dependent initialisations of components (i.e. garrisoned units)
	Engine.BroadcastMessage(MT_InitGame, {});

	let seed = settings.AISeed ? settings.AISeed : 0;
	cmpAIManager.SetRNGSeed(seed);
	cmpAIManager.TryLoadSharedComponent();
	cmpAIManager.RunGamestateInit();
}

Engine.RegisterGlobal("PreInitGame", PreInitGame);
Engine.RegisterGlobal("InitGame", InitGame);
