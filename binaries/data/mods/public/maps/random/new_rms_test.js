Engine.LoadLibrary("rmgen");

InitMap();

placePlayerBases({
	"PlayerPlacement": playerPlacementCircle(0.39)
});

ExportMap();
