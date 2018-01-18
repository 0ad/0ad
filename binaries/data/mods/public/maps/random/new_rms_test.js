Engine.LoadLibrary("rmgen");

InitMap(0, g_MapSettings.BaseTerrain);

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.39))
});

placePlayersNomad(createTileClass());

ExportMap();
