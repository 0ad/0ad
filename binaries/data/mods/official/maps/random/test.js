const SIZE = 128;
init(SIZE, "snow forest", 0);

constr = new AvoidTerrainConstraint("snow grass 2");

println(createArea(
	new MultiPlacer(new ClumpPlacer(15.0, 0.01, 0.01), 10, 100), 
	new TerrainPainter(new RandomTerrain("snow grass 2|wrld_flora_pine", "snow grass 2")),
	constr
));
