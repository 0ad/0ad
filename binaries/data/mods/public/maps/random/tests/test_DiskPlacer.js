Engine.LoadLibrary("rmgen");

{
	var g_MapSettings = { "Size": 512 };
	var g_Map = new RandomMap(0, 0, "blackness");
	let center = new Vector2D(10, 10);
	let area = createArea(new DiskPlacer(3, center));

	// Contains center
	TS_ASSERT(area.contains(center));

	// Contains disk boundaries
	TS_ASSERT(area.contains(new Vector2D(10, 13)));
	TS_ASSERT(area.contains(new Vector2D(10, 7)));
	TS_ASSERT(area.contains(new Vector2D(7, 10)));
	TS_ASSERT(area.contains(new Vector2D(13, 10)));

	// Does not contain rectangle vertices
	TS_ASSERT(!area.contains(new Vector2D(13, 13)));
	TS_ASSERT(!area.contains(new Vector2D(7, 7)));
	TS_ASSERT(!area.contains(new Vector2D(13, 7)));
	TS_ASSERT(!area.contains(new Vector2D(7, 13)));

	// Does not contain points outside disk range
	TS_ASSERT(!area.contains(new Vector2D(10, 14)));
	TS_ASSERT(!area.contains(new Vector2D(10, 6)));
	TS_ASSERT(!area.contains(new Vector2D(6, 10)));
	TS_ASSERT(!area.contains(new Vector2D(14, 10)));

	area = createArea(new DiskPlacer(3, new Vector2D(0, 0)));

	// Does not allow points out of map boundaries
	TS_ASSERT(!area.contains(new Vector2D(-1, -1)));
	// Contains map edge
	TS_ASSERT(area.contains(new Vector2D(0, 0)));
}

{
	// Contains points outside map disk range on CircularMap
	var g_MapSettings = { "Size": 512, "CircularMap": true };
	var g_Map = new RandomMap(0, 0, "blackness");
	var area = createArea(new DiskPlacer(10, new Vector2D(436, 436)));

	TS_ASSERT(area.contains(new Vector2D(438, 438)));
	TS_ASSERT(area.contains(new Vector2D(437, 436)));
	TS_ASSERT(area.contains(new Vector2D(436, 437)));
	TS_ASSERT(area.contains(new Vector2D(435, 435)));

	area = createArea(new DiskPlacer(3, new Vector2D(0, 0)));
	// Does not allow points out of map boundaries
	TS_ASSERT(!area.contains(new Vector2D(-1, -1)));
}

{
	var g_MapSettings = { "Size": 320, "CircularMap": true };
	var g_Map = new RandomMap(0, 0, "blackness");
	// Does not error with floating point radius
	var area = createArea(new DiskPlacer(86.4, new Vector2D(160, 160)));
	// Does not error with extreme out of bounds disk
	area = createArea(new DiskPlacer(86.4, new Vector2D(800, 800)));
	// Does not error when disk on edge
	area = createArea(new DiskPlacer(10, new Vector2D(321, 321)));
}
