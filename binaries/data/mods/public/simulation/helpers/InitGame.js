function InitGame(settings)
{
	// This will be called after the map (settings) have been loaded, before the simulation has started.
	// TODO: Is this even needed?
	if (!settings)
		settings = {};
	
}

Engine.RegisterGlobal("InitGame", InitGame);
