const SIZE = 64;

init(SIZE, "grass dirt 75", 0);

createAreas(
	new ClumpPlacer(23.0, 0.01, 0.01), 
	[new TerrainPainter("cliff_greekb_moss"), 
	 new SmoothElevationPainter(ELEVATION_SET, 30.0, 3)],
	null,
	20,
	200
);

createAreas(
	new ClumpPlacer(1.0, 1.0, 1.0), 
	new TerrainPainter(["grass_dirt_75|wrld_flora_oak", "grass_dirt_75|bush_medit_me"]),
	new AvoidTextureConstraint("cliff_greekb_moss"),
	150,
	200
);
