Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

var g_Map = new RandomMap(0, "grass1");

/**
 * Demonstration code for wall placement.
 *
 * Some notes/reminders:
 * - All angles (orientation) are in radians.
 * - When looking at the map, with the x-axis being horizontal and the y-axis vertical:
 *   -- The origin point (0,0) of the map is in the bottom-left corner,
 *   -- A wall orientated at 0 has its "outside" facing right and its "inside" facing left.
 *   -- A wall orientated at Pi is reversed (obviously).
 *   -- A wall orientated at Pi/2 has its "outside" facing down and its "inside" facing up.
 * - As a general rule, walls are drawn in a anti-clockwise direction.
 *
 * Some general notes concerning the arguments:
 *
 * - The first two arguments for most placement functions are x/y co-ordinates needed to position the wall. These are received via separate arguments, like in placeEntityPassable and their exact meaning differs between methods, but should be mostly self explanatory. The exception to this is placeLinearWall(), where the first four arguments are co-ordinates. However, whether two argument or four, the initial x/y co-ordinates are required parameters.
 *
 * - For some functions, the next argument is radius, indicating how far from a central point the wall should be drawn. The functions that use this are marked as doing so below.
 *
 * - The next argument is usually an array containing wall element type strings. (See the block comment for getWallElement() for a list of accepted type strings.) The exception to this is placeFortress(), which accepts a string here instead, identifying which of the predefined fortresses designs you wish to use. (See the example provided below for details.)
 *
 *   Most functions will ask that you do not include "bending" wall elements in your array ("cornerIn", "cornerOut", "turn_{x}") and will complain if you attempt to do so. The ones that do this are clearly marked below.
 *
 *   The array will generally look like:
 *     ["start", "medium", "tower", "gate", "tower", "medium", "end"]
 *
 *   Remember that walls are drawn in an anti-clockwise direction. Thus, when looking at a wall element in-game, with the "outside" facing up, then the *next* wall element will be drawn to the left of *this* element.
 *
 *   This argument is optional, and each function has a different default value.
 *
 * - The next argument is a string denoting the style of the wall. These are derived from the names of wallsets defined in 0ad: for example "athen_wallset_stone" becomes "athen_stone", and "rome_wallset_siege" becomes "rome_siege". (A full list can be found stored as the keys of the global constant g_WallStyles.) This argument is optional, and if not set, the civ's basic stone wallset will be used.
 *
 * - The next argument is the player-id of the player that is to own the wall. This argument is optional, and defaults to 0 (gaia).
 *
 * - The next argument is an angle defining the angle of orientation of the wall. The exact use differs slightly between functions, but hopefully the comments on the examples below should help. Also see the notes above about wall orientation. This argument is optional, and defaults to 0.
 *
 * - Any remaining arguments differ from function to function, but are all optional. Please read the comments below, and also the block comment of the function in wall_builder.js for further details.
 *
 * And have fun!
 */

var mapSize = g_Map.getSize();

/**
 * General wall placement setup
 */
const distToMapBorder = 5;
const distToOtherWalls = 10;
var buildableMapSize = mapSize - 2 * distToMapBorder;

var position = new Vector2D(distToMapBorder, distToMapBorder);
var playerID = 0;
const wallStyleList = Object.keys(g_WallStyles);

/**
 * Custom wall placement (element based).
 *
 * Like most wall placement functions, we have to supply an x/y position.
 * In this case, the x/y position marks the start of the wall.
 *
 * For this function, orientation indicates the angle at which the first
 * wall element should be drawn. (The direction that the outside of the
 * first wall element faces towards.)
 *
 * This function permits bending wall elements.
 */
for (let styleIndex in wallStyleList)
{
	let pos = Vector2D.add(position, new Vector2D(styleIndex * buildableMapSize / wallStyleList.length));
	let wall = ['start', 'long', 'tower', 'tower', 'tower', 'medium', 'outpost', 'medium', 'cornerOut', 'medium', 'cornerIn', 'medium', 'house', 'end', 'entryTower', 'start', 'short', 'barracks', 'gate', 'tower', 'medium', 'fort', 'medium', 'end'];
	let style = wallStyleList[styleIndex];
	let orientation = Math.PI / 16 * Math.sin(styleIndex * Math.PI / 4);
	placeWall(pos, wall, style, playerID, orientation);
}

position.y += 80 + distToOtherWalls;

/**
 * Default fortress placement (chosen by fortress type string)
 *
 * The x/y position in this case marks the center point of the fortress.
 * To make it clearer, we add an obilisk as a visual marker.
 *
 * This is the only wall placement function that does not take an array
 * of elements as an argument. Instead, we provide a "type" that identifies
 * a predefined design to draw. The list of possible types are: "tiny",
 * "small", "medium", "normal", "large", "veryLarge", and "giant".
 *
 * For this function, orientation is the direction in which the main gate
 * is facing.
 */
var fortressRadius = 15; // The space the fortresses take in average. Just for design of this map. Not passed to the function.

for (let styleIndex in wallStyleList)
{
	let type = "tiny";
	let style = wallStyleList[styleIndex];
	let orientation = styleIndex * Math.PI / 32;

	let pos = Vector2D.sum([
		position,
		new Vector2D(1, 1).mult(fortressRadius),
		new Vector2D(styleIndex * buildableMapSize / wallStyleList.length, 0)
	]);

	g_Map.placeEntityPassable("other/obelisk", playerID, pos, orientation);
	placeFortress(pos, type, style, playerID, orientation);
}

position.y += 2 * fortressRadius + distToOtherWalls;

/**
 * 'Generic' fortress placement (iberian wall circuit code)
 *
 * The function used here is unusual in that the owner and style arguments
 * are swapped. It is also unusual in that we do not supply an orientation.
 *
 * The x/y position in this case marks the center point of the fortress.
 * To make it clearer, we add an obilisk as a visual marker.
 *
 * We also supply a radius value to dictate how wide the circuit of walls should be.
 */
var radius = Math.min((mapSize - position.y - distToOtherWalls) / 3, (buildableMapSize / wallStyleList.length - distToOtherWalls) / 2);
for (let styleIndex in wallStyleList)
{
	let style = wallStyleList[styleIndex];

	let pos = Vector2D.sum([
		position,
		new Vector2D(radius, radius),
		new Vector2D(styleIndex * buildableMapSize / wallStyleList.length, 0)
	]);

	g_Map.placeEntityPassable("other/obelisk", playerID, pos, 0);
	placeGenericFortress(pos, radius, playerID, style);
}

position.y += 2 * radius + distToOtherWalls;

/**
 * Circular wall placement
 *
 * It is possible with this function to draw complete circles, or arcs.
 * Each side of the wall consists of the contents of the provided wall
 * array, with the code calculating the number and angle of turns and
 * sides automatically based on the calculated length of each side and
 * the given radius.
 *
 * This function does not permit the use of bending wall elements.
 *
 * In this case, the x/y co-ordinates are the center point around which
 * to draw the walls. To make this clearer, we add an obelisk as a visual
 * marker.
 *
 * We also provide a radius to define the distance between the center
 * point and the walls.
 *
 * For this function, orientation is the direction that the opening of an
 * arc faces. If the wall is to be a complete circle, then this is used as
 * the orientation of the first wall piece.
 */
radius = Math.min((mapSize - position.y - distToOtherWalls) / 3, (buildableMapSize / wallStyleList.length - distToOtherWalls) / 2);
for (let styleIndex in wallStyleList)
{
	let center = Vector2D.sum([position, new Vector2D(1, 1).mult(radius), new Vector2D(styleIndex * buildableMapSize / wallStyleList.length, 0)]);
	let wallPart = ['tower', 'medium', 'house'];
	let style = wallStyleList[styleIndex];
	let orientation = styleIndex * Math.PI / 16;

	// maxAngle is how far the wall should circumscribe the center.
	// If equal to Pi * 2, then the wall will be a full circle.
	// If less than Pi * 2, then the wall will be an arc.
	let maxAngle = Math.PI / 2 * (styleIndex % 3 + 2);

	g_Map.placeEntityPassable("other/obelisk", playerID, center, orientation);
	placeCircularWall(center, radius, wallPart, style, playerID, orientation, maxAngle);
}

position.y += 2 * radius + distToOtherWalls;

/**
 * Regular Polygonal wall placement
 *
 * This function draws a regular polygonal wall around a given point. All
 * the sides follow the same pattern, and the (automatically calculated)
 * angles at the corners are identical. We define how many corners we want.
 *
 * This function does not permit the use of bending wall elements.
 *
 * In this case, the x/y co-ordinates are the center point around which
 * to draw the walls. To make this clearer, we add an obelisk as a visual
 * marker.
 *
 * We also provide a radius to define the distance between the center
 * point and the walls.
 *
 * After the usual array of wall elements to use, and before the style
 * argument, we provide the name of a single wall element to use as a
 * corner piece.
 *
 * In this function, orientation is the direction the first wall has its
 * outward side facing or, if the `skipFirstWall` argument is true, the
 * opening in the wall.
 */

radius = Math.min((mapSize - position.y - distToOtherWalls) / 2, (buildableMapSize / wallStyleList.length - distToOtherWalls) / 2);
for (let styleIndex in wallStyleList)
{
	let centerPosition = Vector2D.sum([position, new Vector2D(1, 1).mult(radius), new Vector2D(styleIndex * buildableMapSize / wallStyleList.length, 0)]);
	let wallParts = ['medium', 'tower']; // Function default: ['long', 'tower']

	// Which wall element to use for the corners of the polygon
	let cornerWallElement = 'tower';

	let style = wallStyleList[styleIndex];
	let orientation = styleIndex * Math.PI / 16;

	// How many corners the polygon should have:
	let numCorners = styleIndex % 6 + 3;

	// If true, the first side will not be drawn, leaving the wall open.
	let skipFirstWall = true;

	g_Map.placeEntityPassable("other/obelisk", playerID, centerPosition, orientation);
	placePolygonalWall(centerPosition, radius, wallParts, cornerWallElement, style, playerID, orientation, numCorners, skipFirstWall);
}

position.y += 2 * radius + distToOtherWalls;

/**
 * Irregular Polygonal wall placement
 *
 * This function draws an irregular polygonal wall around a given point.
 * Each side of the wall is different, each element used selected at
 * pesudo-random from an assortment. The angles at the corners also differ.
 * We can control this randomness by changing the irregularity argument.
 *
 * This function does not permit the use of bending wall elements.
 *
 * In this case, the x/y co-ordinates are the center point around which
 * to draw the walls. To make this clearer, we add an obelisk as a visual
 * marker.
 *
 * We also provide a radius to define the distance between the center
 * point and the walls.
 *
 * The usual array of wall elements is left out here, instead we provide
 * the name of a single wall element to use as a corner piece.
 *
 * In this function, orientation is the direction the first wall has its
 * outward side facing or, if the `skipFirstWall` argument is true, the
 * opening in the wall.
 *
 * The very last argument is the collection of wallparts used to build
 * the wall. It is not defined in this example (so as to use the defaults)
 * as it is not easy to comprehend.
 */
radius = Math.min((mapSize - position.y - distToOtherWalls) / 2, (buildableMapSize / wallStyleList.length - distToOtherWalls) / 2); // The radius of wall polygons
for (let styleIndex in wallStyleList)
{
	let centerPosition = Vector2D.sum([
		position,
		new Vector2D(1, 1).mult(radius),
		new Vector2D(styleIndex * buildableMapSize / wallStyleList.length, 0)
	]);

	// Which wall element type will be used for the corners of the polygon.
	let cornerWallElement = 'tower';

	let style = wallStyleList[styleIndex];
	let orientation = styleIndex * Math.PI / 16;

	// How many corners the polygon will have
	let numCorners = styleIndex % 6 + 3;

	// Irregularity of the polygon.
	let irregularity = 0.5;

	// If true, the first side will not be drawn, leaving the wall open.
	let skipFirstWall = true;

	g_Map.placeEntityPassable("other/obelisk", playerID, centerPosition, orientation);
	placeIrregularPolygonalWall(centerPosition, radius, cornerWallElement, style, playerID, orientation, numCorners, irregularity, skipFirstWall);
}

position.y += 2 * radius + distToOtherWalls;

/**
 * Linear wall placement
 *
 * This function draws a straight wall between two given points.
 *
 * This function does not permit the use of bending wall elements.
 *
 * This function has no orientation parameter, the wall pieces are angled
 * automatically. Remember: each piece is placed to the left of the
 * previous piece. Thus, if the start point is at the right-hand side of
 * the screen and the end point is at the left-hand side, the "outside"
 * of the walls is facing the top of the screen.
 */
// Two vars, just for this map; firstly how long the longest wall will be.
var maxWallLength = (mapSize - position.y - distToMapBorder - distToOtherWalls);
// And secondly, how many walls of the same style will be placed.
var numWallsPerStyle = Math.floor(buildableMapSize / distToOtherWalls / wallStyleList.length);

for (let styleIndex in wallStyleList)
	for (let wallIndex = 0; wallIndex < numWallsPerStyle; ++wallIndex)
	{
		let offsetX = (styleIndex * numWallsPerStyle + wallIndex) * buildableMapSize / wallStyleList.length / numWallsPerStyle;
		let start = Vector2D.add(position, new Vector2D(offsetX, 0));

		let offsetY = (wallIndex + 1) * maxWallLength / numWallsPerStyle;
		let end = Vector2D.add(position, new Vector2D(offsetX, offsetY));

		placeLinearWall(start, end, ['tower', 'medium'], wallStyleList[styleIndex], playerID);
	}

g_Map.ExportMap();
