function InitGame(settings)
{
	// This will be called after the map settings have been loaded,
	// before the simulation has started.

	// No settings when loading a map in Atlas, so do nothing
	if (!settings)
		return;

	var cmpAIManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_AIManager);
	for (var i = 0; i < settings.AIs.length; ++i)
	{
		if (settings.AIs[i])
			cmpAIManager.AddPlayer(settings.AIs[i], i);
	}
}

Engine.RegisterGlobal("InitGame", InitGame);
