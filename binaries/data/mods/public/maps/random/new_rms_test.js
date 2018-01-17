Engine.LoadLibrary("rmgen");

InitMap(g_MapSettings.BaseHeight, g_MapSettings.BaseTerrain);

if (isNomad())
	placePlayersNomad(createTileClass());
else
	placePlayerBases({
		"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.39))
	});

ExportMap();
