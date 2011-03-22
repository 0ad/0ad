function InitGame(settings)
{
	// This will be called after the map settings have been loaded,
	// before the simulation has started.

	// No settings when loading a map in Atlas, so do nothing
	if (!settings)
		return;

	var cmpAIManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AIManager);
	for (var i = 0; i < settings.PlayerData.length; ++i)
	{
		if (settings.PlayerData[i] && settings.PlayerData[i].AI && settings.PlayerData[i].AI != "")
			cmpAIManager.AddPlayer(settings.PlayerData[i].AI, i+1);
	}
}

Engine.RegisterGlobal("InitGame", InitGame);
