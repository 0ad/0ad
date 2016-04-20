
/**
 * Used to initialize non-player settings relevant to the map, like
 * default stance and victory conditions. DO NOT load players here
 */
function LoadMapSettings(settings)
{
	// Default settings
	if (!settings)
		settings = {};
	
	if (settings.DefaultStance)
	{
		for each (var ent in Engine.GetEntitiesWithInterface(IID_UnitAI))
		{
			var cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
			cmpUnitAI.SwitchToStance(settings.DefaultStance);
		}
	}

	if (settings.RevealMap)
	{
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetLosRevealAll(-1, true);
	}

	if (settings.DisableTreasures)
	{
		for (let ent of Engine.GetEntitiesWithInterface(IID_ResourceSupply))
		{
			let cmpResourceSupply = Engine.QueryInterface(ent, IID_ResourceSupply);
			if (cmpResourceSupply.GetType().generic == "treasure")
				Engine.DestroyEntity(ent);
		}
	}

	if (settings.CircularMap)
	{
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetLosCircular(true);

		var cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
		if (cmpObstructionManager)
			cmpObstructionManager.SetPassabilityCircular(true);
	}

	var cmpEndGameManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_EndGameManager);
	if (settings.GameType)
		cmpEndGameManager.SetGameType(settings.GameType);
	if (settings.WonderDuration)
		cmpEndGameManager.SetWonderDuration(settings.WonderDuration * 60 * 1000);

	if (settings.Garrison)
	{
		for (let holder in settings.Garrison)
		{
			let cmpGarrisonHolder = Engine.QueryInterface(+holder, IID_GarrisonHolder);
			if (!cmpGarrisonHolder)
				warn("Map error in Setup.js: entity " + holder + " can not garrison units");
			else
				cmpGarrisonHolder.initGarrison = settings.Garrison[holder];
		}
	}
	
	var cmpCeasefireManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_CeasefireManager);
	if (settings.Ceasefire)
		cmpCeasefireManager.StartCeasefire(settings.Ceasefire * 60 * 1000);
}

Engine.RegisterGlobal("LoadMapSettings", LoadMapSettings);
