Engine.LoadLibrary("rmgen");

InitMap(0, "grass1_spring");

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.39))
});

placePlayersNomad(createTileClass());

ExportMap();
