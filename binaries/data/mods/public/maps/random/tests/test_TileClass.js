Engine.GetTemplate = (path) => {
	return {
		"Identity": {
			"GenericName": null,
			"Icon": null,
			"History": null
		}
	};
};

Engine.LoadLibrary("rmgen");

var g_MapSettings = { "Size": 512 };
var g_Map = new RandomMap(0, "blackness");

// Test that that it checks by value, not by reference
{
	const tileClass = new TileClass(2);
	const reference1 = new Vector2D(1, 1);
	const reference2 = new Vector2D(1, 1);
	tileClass.add(reference1);
	TS_ASSERT(tileClass.has(reference2));
}

// Test out-of-bounds
{
	const tileClass = new TileClass(32);

	const absentPoints = [
		new Vector2D(0, 0),
		new Vector2D(0, 1),
		new Vector2D(1, 0),
		new Vector2D(-1, -1),
		new Vector2D(2048, 0),
		new Vector2D(0, NaN),
		new Vector2D(0, Infinity)
	];

	for (const point of absentPoints)
		TS_ASSERT(!tileClass.has(point));
}

// Test getters
{
	const tileClass = new TileClass(88);

	const point = new Vector2D(5, 5);
	tileClass.add(point);

	const pointBorder = new Vector2D(1, 9);
	tileClass.add(pointBorder);

	TS_ASSERT_EQUALS(tileClass.countMembersInRadius(point, 0), 1);
	TS_ASSERT_EQUALS(tileClass.countMembersInRadius(point, 1), 1);
	TS_ASSERT_EQUALS(tileClass.countMembersInRadius(point, 100), 2);

	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(point, 1), 4);
	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(point, 2), 12);
	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(point, 3), 28);

	// Points not on the map are not counted.
	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(pointBorder, 1), 4);
	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(pointBorder, 2), 11);
	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(pointBorder, 3), 22);
}
