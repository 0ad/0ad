Engine.LoadLibrary("rmgen");

InitMap();

if (isNomad())
	placePlayersNomad(createTileClass());
else
	placePlayerBases({
		"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.39))
	});

ExportMap();
