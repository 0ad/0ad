Engine.LoadLibrary("rmgen");

var g_MapSettings = { "Size": 512 };
var g_Map = new RandomMap(0, 0, "blackness");

{
	let min = new Vector2D(4, 4);
	let max = new Vector2D(10, 10);

	let center = Vector2D.average([min, max]);

	createArea(
		new RectPlacer(min, max),
		new LayeredPainter(["red", "blue"], [2]));

	TS_ASSERT_EQUALS(g_Map.getTexture(min), "red");
	TS_ASSERT_EQUALS(g_Map.getTexture(max), "red");
	TS_ASSERT_EQUALS(g_Map.getTexture(new Vector2D(-1, -1).add(max)), "red");
	TS_ASSERT_EQUALS(g_Map.getTexture(new Vector2D(-2, -2).add(max)), "blue");
	TS_ASSERT_EQUALS(g_Map.getTexture(new Vector2D(-3, -3).add(max)), "blue");
	TS_ASSERT_EQUALS(g_Map.getTexture(center), "blue");
}
