Engine.LoadLibrary("rmgen");

var g_MapSettings = { "Size": 512 };
var g_Map = new RandomMap(0, 0, "blackness");

{
	let tileClass = new TileClass(g_Map.getSize());

	let addedPos = new Vector2D(5, 0);
	tileClass.add(addedPos);

	let origin = new Vector2D(0, 0);

	TS_ASSERT(!(new AvoidTileClassConstraint(tileClass, 0).allows(addedPos)));
	TS_ASSERT(new AvoidTileClassConstraint(tileClass, 0).allows(origin));
	TS_ASSERT(!(new AvoidTileClassConstraint(tileClass, 5).allows(origin)));

	TS_ASSERT(new NearTileClassConstraint(tileClass, 5).allows(origin));
	TS_ASSERT(new NearTileClassConstraint(tileClass, 20).allows(origin));
	TS_ASSERT(!(new NearTileClassConstraint(tileClass, 4).allows(origin)));
}
