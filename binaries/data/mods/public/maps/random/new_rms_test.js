Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

function* GenerateMap()
{
	globalThis.g_Map = new RandomMap(0, "grass1_spring");

	placePlayerBases({
		"PlayerPlacement": playerPlacementCircle(fractionToTiles(0.39))
	});

	placePlayersNomad(g_Map.createTileClass());

	return g_Map;
}
