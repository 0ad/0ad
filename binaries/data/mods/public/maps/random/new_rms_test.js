Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

var g_Map = new RandomMap(0, "grass1_spring");

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.39))
});

placePlayersNomad(g_Map.createTileClass());

g_Map.ExportMap();
