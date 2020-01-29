Engine.LoadLibrary("rmgen");

var g_MapSettings = { "Size": 512 };
var g_Map = new RandomMap(0, 0, "blackness");

{
	let min = new Vector2D(5, 5);
	let max = new Vector2D(7, 7);

	let area = createArea(new RectPlacer(min, max));

	TS_ASSERT(!area.contains(new Vector2D(-1, -1).add(min)));
	TS_ASSERT(area.contains(min));
	TS_ASSERT(area.contains(max));
	TS_ASSERT(area.contains(max.clone()));
	TS_ASSERT(area.contains(Vector2D.average([min, max])));
}
