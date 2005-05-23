const SIZE = 48;
init(SIZE, "grass1_a", 0);

var placer = new RectPlacer(0,0,3,4);
placer.y2 = 6;
var paint = new TerrainPainter("snow");
var constr = new NullConstraint();
println(createArea(placer, paint));
