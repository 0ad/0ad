const SIZE = 64;

init(SIZE, ["grass dirt 50", "grass dirt 75"], 0);

var tc = createTileClass();
var tc2 = createTileClass();

createAreas(
	new ClumpPlacer(4.0, 0.01, 0.01), 
	[new TerrainPainter("dirt_forest|flora/wrld_flora_deci_1.xml"), new TileClassPainter(tc)],
	new AvoidTileClassConstraint(tc, 5),
	100);

createObjectGroups(
	new SimpleGroup([new SimpleObject("foliage/bush.xml", 1, 0)], tc2),
	[new AvoidTileClassConstraint(tc, 3), new AvoidTileClassConstraint(tc2, 2)],
	200);
