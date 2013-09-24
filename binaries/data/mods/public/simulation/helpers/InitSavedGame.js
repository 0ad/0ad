function InitSavedGame()
{
	// This will be called after the map settings have been loaded,
	// before the simulation has started.
	// This is only called at the start of a saved game, not when loading
	// a new game.

	// rebuild the RallyPointRenderers from the RallyPoints info
	var rallyPoints = Engine.GetEntitiesWithInterface(IID_RallyPoint);
	for each (var ent in rallyPoints)
	{
		var cmpRallyPointRenderer = Engine.QueryInterface(ent, IID_RallyPointRenderer);
		if (!cmpRallyPointRenderer)
			continue;
		var positions = Engine.QueryInterface(ent, IID_RallyPoint).GetPositions();
		for each (var pos in positions)
			cmpRallyPointRenderer.AddPosition({'x': pos.x, 'y': pos.z});
	}
}

Engine.RegisterGlobal("InitSavedGame", InitSavedGame);
