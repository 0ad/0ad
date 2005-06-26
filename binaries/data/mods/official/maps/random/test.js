const SIZE = 64;

init(SIZE, ["snow", "snow forest"], 0);

createAreas(
	new ClumpPlacer(20.0, 0.01, 0.01), 
	[new TerrainPainter("snow grass 2"), 
	 new SmoothElevationPainter(ELEVATION_SET, 15.0, 3)],
	new AvoidTextureConstraint("snow grass 2"),
	150,
	500
);
