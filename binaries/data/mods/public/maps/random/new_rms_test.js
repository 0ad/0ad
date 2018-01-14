Engine.LoadLibrary("rmgen");

InitMap();

if (isNomad())
	placePlayersNomad(createTileClass());
else
	placePlayerBases({
		"PlayerPlacement": playerPlacementCircle(0.39)
	});

ExportMap();
