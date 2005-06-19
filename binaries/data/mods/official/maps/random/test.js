const SIZE = 128;
init(SIZE, new RandomTerrain("snow", "snow forest"), 0);

createMulti(
	new ClumpPlacer(20.0, 0.01, 0.01), 
	new LayeredPainter([1], ["snow grass 2", "snow grass 2|wrld_flora_pine"]),
	new AvoidTextureConstraint("snow grass 2"),
	350,
	1000
);
