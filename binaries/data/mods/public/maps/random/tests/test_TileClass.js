Engine.LoadLibrary("rmgen");

var g_MapSettings = { "Size": 512 };
var g_Map = new RandomMap(0, "blackness");

// Test that that it checks by value, not by reference
{
	let tileClass = new TileClass(2);
	let reference1 = new Vector2D(1, 1);
	let reference2 = new Vector2D(1, 1);
	tileClass.add(reference1);
	TS_ASSERT(tileClass.has(reference2));
}

// Test out-of-bounds
{
	let tileClass = new TileClass(32);

	let absentPoints = [
		new Vector2D(0, 0),
		new Vector2D(0, 1),
		new Vector2D(1, 0),
		new Vector2D(-1, -1),
		new Vector2D(2048, 0),
		new Vector2D(0, NaN),
		new Vector2D(0, Infinity)
	];

	for (let point of absentPoints)
		TS_ASSERT(!tileClass.has(point));
}

// Test multi-insertion
{
	let tileClass = new TileClass(32);
	let point = new Vector2D(1, 1);

	tileClass.add(point);
	tileClass.add(point);
	TS_ASSERT_EQUALS(tileClass.countMembersInRadius(point, 0), 1);

	// Still one point remaining
	tileClass.remove(point);
	TS_ASSERT(tileClass.has(point));

	tileClass.remove(point);
	TS_ASSERT(!tileClass.has(point));
}

// Test multi-insertion removal
{
	let tileClass = new TileClass(32);
	let point = new Vector2D(2, 7);

	tileClass.add(point);
	tileClass.add(point);
	tileClass.remove(point);
	TS_ASSERT(tileClass.has(point));

	tileClass.remove(point);
	TS_ASSERT(!tileClass.has(point));
}

// Test multi-insertion removal
{
	let tileClass = new TileClass(55);
	let point = new Vector2D(5, 4);

	for (let i = 0; i < 50; ++i)
		tileClass.add(point);

	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(point, 1), 4);
	tileClass.remove(point);
	TS_ASSERT(tileClass.has(point));
	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(point, 1), 4);
	tileClass.remove(point);
	TS_ASSERT(tileClass.has(point));
}

// Test getters
{
	let tileClass = new TileClass(88);

	let point = new Vector2D(5, 1);
	tileClass.add(point);

	let point2 = new Vector2D(4, 9);
	tileClass.add(point2);

	TS_ASSERT_EQUALS(tileClass.countMembersInRadius(point, 0), 1);
	TS_ASSERT_EQUALS(tileClass.countMembersInRadius(point, 1), 1);
	TS_ASSERT_EQUALS(tileClass.countMembersInRadius(point, 100), 2);

	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(point, 1), 4);
	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(point, 1), 4);
	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(point, 2), 12);
	TS_ASSERT_EQUALS(tileClass.countNonMembersInRadius(point, 3), 28);
}
