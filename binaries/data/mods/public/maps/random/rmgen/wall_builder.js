////////////////////////////////////////////////////////////////////
// This file contains functionality to place walls on random maps //
////////////////////////////////////////////////////////////////////

// To do:
// Check if all wall placement methods work with wall elements with entity === undefined (some still might raise errors in that case)
// Rename wall elements to fit the entity names so that entity = "structures/" + "civ + "_" + wallElement.type in the common case (as far as possible)
// Perhaps add Roman army camp to style palisades and add upgraded/balanced default palisade fortress types matching civ default fortresses strength
// Perhaps add further wall elements cornerInHalf, cornerOutHalf (banding PI/4) and adjust default fortress types to better fit in the octagonal territory of a civil center
// Perhaps swap angle and width in WallElement class(?) definition
// Adjust argument order to be always the same:
//	Coordinates (center/start/target)
//	Wall element arguments (wall/wallPart/fortressType/cornerElement)
//	playerId (optional, default is 0/gaia)
//	wallStyle (optional, default is the players civ/"palisades for gaia")
//	angle/orientation (optional, default is 0)
//	other (all optional) arguments especially those hard to define (wallPartsAssortment, maybe make an own function for it)
//	Some arguments don't clearly match to this concept:
//		endWithFirst (wall or other)
//		skipFirstWall (wall or other)
//		gateOccurence (wall or other)
//		numCorners (wall or other)
//		skipFirstWall (wall or other)
//		maxAngle (angle or other)
//		maxBendOff (angle or other, unused ATM!!!)
//		irregularity
//		maxTrys
// Add treasures to wall style "others"
// Adjust documentation
// Perhaps rename "endLeft" to "start" and "endRight" to "end"
// ?Use available civ-type wall elements rather than palisades: Remove "endLeft" and "endRight" as default wall elements and adjust default palisade fortress types?
// ?Remove "endRight", "endLeft" and adjust generic fortress types palisades?
// ?Think of something to enable splitting walls into two walls so more complex walls can be build and roads can have branches/crossroads?
// ?Readjust placement angle for wall elements with bending when used in linear/circular walls by their bending?

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  WallElement class definition
//
//	Concept: If placed unrotated the wall's course is towards positive Y (top) with "outside" right (+X) and "inside" left (-X) like unrotated entities has their drop-points right (in rmgen)
//	The course of the wall will be changed by corners (bending != 0) and so the "inside"/"outside" direction
//
//	type    Descriptive string, example: "wallLong". NOTE: Not really needed. Mainly for custom wall elements and to get the wall element type in code
//	entity  Optional. Template name string of the entity to be placed, example: "structures/cart_wall_long". Default is undefined (No entity placed)
//	angle   Optional. The angle (float) added to place the entity so "outside" is right when the wall element is placed unrotated. Default is 0
//	width   Optional. How far this wall element lengthens the wall (float), if unrotated the Y space needed. Default is 0
//	indent  Optional. The lateral indentation of the entity, drawn "inside" (positive values) or pushed "outside" (negative values). Default is 0
//	bending Optional. How the course of the wall is changed after this element, positive is bending "in"/left/counter clockwise (like entity placement)
//		NOTE: Bending is not supported by all placement functions (see there)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function WallElement(type, entity, angle, width, indent, bending)
{
	this.type = type;
	// Default wall element type documentation:
	// Lengthening straight blocking (mainly left/right symmetric) wall elements (Walls and wall fortifications)
		// "wall"          A blocking straight wall element that mainly lengthens the wall, self-explanatory
		// "wallShort"     self-explanatory
		// "wallLong"      self-explanatory
		// "tower"         A blocking straight wall element with damage potential (but for palisades) that slightly lengthens the wall, example: wall tower, palisade tower(No attack)
		// "wallFort"      A blocking straight wall element with massive damage potential that lengthens the wall, example: fortress, palisade fort
	// Lengthening straight non/custom blocking (mainly left/right symmetric) wall elements (Gates and entries)
		// "gate"          A blocking straight wall element with passability determined by owner, example: gate (Functionality not yet implemented)
		// "entry"         A non-blocking straight wall element (same width as gate) but without an actual template or just a flag/column/obelisk
		// "entryTower"    A non-blocking straight wall element (same width as gate) represented by a single (maybe indented) template, example: defence tower, wall tower, outpost, watchtower
		// "entryFort"     A non-blocking straight wall element represented by a single (maybe indented) template, example: fortress, palisade fort
	// Bending wall elements (Wall corners)
		// "cornerIn"      A wall element bending the wall by PI/2 "inside" (left, +, see above), example: wall tower, palisade curve
		// "cornerOut"     A wall element bending the wall by PI/2 "outside" (right, -, see above), example: wall tower, palisade curve
		// "cornerHalfIn"  A wall element bending the wall by PI/4 "inside" (left, +, see above), example: wall tower, palisade curve. NOTE: Not yet implemented
		// "cornerHalfOut" A wall element bending the wall by PI/4 "outside" (right, -, see above), example: wall tower, palisade curve. NOTE: Not yet implemented
	// Zero length straight indented (mainly left/right symmetric) wall elements (Outposts/watchtowers and non-defensive base structures)
		// "outpost"       A zero-length wall element without bending far indented so it stands outside the wall, example: outpost, defence tower, watchtower
		// "house"         A zero-length wall element without bending far indented so it stands inside the wall that grants population bonus, example: house, hut, longhouse
		// "barracks"      A zero-length wall element without bending far indented so it stands inside the wall that grants unit production, example: barracks, tavern, ...
	this.entity = entity;
	this.angle = angle !== undefined ? angle : 0;
	this.width = width !== undefined ? width : 0;
	this.indent = indent !== undefined ? indent : 0;
	this.bending = bending !== undefined ? bending : 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Fortress class definition
//
//	A "fortress" here is a closed wall build of multiple wall elements attached together defined in Fortress.wall
//	It's mainly the abstract shape defined in a Fortress instances wall because different styles can be used for it (see wallStyles)
//
//	type                  Descriptive string, example: "tiny". Not really needed (WallTool.wallTypes["type string"] is used). Mainly for custom wall elements
//	wall                  Optional. Array of wall element strings. Can be set afterwards. Default is an epty array.
//		Example: ["entrance", "wall", "cornerIn", "wall", "gate", "wall", "entrance", "wall", "cornerIn", "wall", "gate", "wall", "cornerIn", "wall"]
//	centerToFirstElement  Optional. Object with properties "x" and "y" representing a vector from the visual center to the first wall element. Default is undefined
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function Fortress(type, wall, centerToFirstElement)
{
	this.type = type; // Only usefull to get the type of the actual fortress
	this.wall = wall !== undefined ? wall : [];
	this.centerToFirstElement = undefined;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  wallStyles data structure for default wall styles
//
//	A wall style is an associative array with all wall elements of that style in it associated with the wall element type string
//	wallStyles holds all the wall styles within an associative array with the civ string or another descriptive strings as key
//	Examples: "athen", "rome_siege", "palisades", "fence", "road"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
var wallStyles = {};

// Generic civ dependent wall style definition. "rome_siege" needs some tweek...
var wallScaleByType = {
	"athen": 1.5,
	"brit": 1.5,
	"cart": 1.8,
	"gaul": 1.5,
	"iber": 1.5,
	"mace": 1.5,
	"maur": 1.5,
	"pers": 1.5,
	"ptol": 1.5,
	"rome": 1.5,
	"sele": 1.5,
	"spart": 1.5,
	"rome_siege": 1.5
};

for (let style in wallScaleByType)
{
	let civ = style;
	if (style == "rome_siege")
		civ = "rome";

	wallStyles[style] = {
		// Default wall elements
		"tower":      new WallElement("tower", "structures/" + style + "_wall_tower", PI, wallScaleByType[style]),
		"endLeft":    new WallElement("endLeft", "structures/" + style + "_wall_tower", PI, wallScaleByType[style]), // Same as tower. To be compatible with palisades...
		"endRight":   new WallElement("endRight", "structures/" + style + "_wall_tower", PI, wallScaleByType[style]), // Same as tower. To be compatible with palisades...
		"cornerIn":   new WallElement("cornerIn", "structures/" + style + "_wall_tower", 5/4*PI, 0, 0.35 * wallScaleByType[style], PI/2), // 2^0.5 / 4 ~= 0.35 ~= 1/3
		"cornerOut":  new WallElement("cornerOut", "structures/" + style + "_wall_tower", 3/4*PI, 0.71 * wallScaleByType[style], 0, -PI/2), // 2^0.5 / 2 ~= 0.71 ~= 2/3
		"wallShort":  new WallElement("wallShort", "structures/" + style + "_wall_short", 0, 2*wallScaleByType[style]),
		"wall":       new WallElement("wall", "structures/" + style + "_wall_medium", 0, 4*wallScaleByType[style]),
		"wallMedium": new WallElement("wall", "structures/" + style + "_wall_medium", 0, 4*wallScaleByType[style]),
		"wallLong":   new WallElement("wallLong", "structures/" + style + "_wall_long", 0, 6*wallScaleByType[style]),

		// Gate and entrance wall elements
		"gate":       new WallElement("gate", "structures/" + style + "_wall_gate", PI, 6*wallScaleByType[style]),
		"entry":      new WallElement("entry", undefined, 0, 6*wallScaleByType[style]),
		"entryTower": new WallElement("entryTower", "structures/" + civ + "_defense_tower", PI, 6*wallScaleByType[style], -4*wallScaleByType[style]),
		"entryFort":  new WallElement("entryFort", "structures/" + civ + "_fortress", 0, 8*wallScaleByType[style], 6*wallScaleByType[style]),

		// Defensive wall elements with 0 width outside the wall
		"outpost":      new WallElement("outpost", "structures/" + civ + "_outpost", PI, 0, -4*wallScaleByType[style]),
		"defenseTower": new WallElement("defenseTower", "structures/" + civ + "_defense_tower", PI, 0, -4*wallScaleByType[style]),

		// Base buildings wall elements with 0 width inside the wall
		"barracks":    new WallElement("barracks", "structures/" + civ + "_barracks", PI, 0, 4.5*wallScaleByType[style]),
		"civilCentre": new WallElement("civilCentre", "structures/" + civ + "_civil_centre", PI, 0, 4.5*wallScaleByType[style]),
		"farmstead":   new WallElement("farmstead", "structures/" + civ + "_farmstead", PI, 0, 4.5*wallScaleByType[style]),
		"field":       new WallElement("field", "structures/" + civ + "_field", PI, 0, 4.5*wallScaleByType[style]),
		"fortress":    new WallElement("fortress", "structures/" + civ + "_fortress", PI, 0, 4.5*wallScaleByType[style]),
		"house":       new WallElement("house", "structures/" + civ + "_house", PI, 0, 4.5*wallScaleByType[style]),
		"market":      new WallElement("market", "structures/" + civ + "_market", PI, 0, 4.5*wallScaleByType[style]),
		"storehouse":  new WallElement("storehouse", "structures/" + civ + "_storehouse", PI, 0, 4.5*wallScaleByType[style]),
		"temple":      new WallElement("temple", "structures/" + civ + "_temple", PI, 0, 4.5*wallScaleByType[style]),

		// Generic space/gap wall elements
		"space1": new WallElement("space1", undefined, 0, 1*wallScaleByType[style]),
		"space2": new WallElement("space2", undefined, 0, 2*wallScaleByType[style]),
		"space3": new WallElement("space3", undefined, 0, 3*wallScaleByType[style]),
		"space4": new WallElement("space4", undefined, 0, 4*wallScaleByType[style])
	};
}

// Add wall fortresses for all generic styles
wallStyles.athen.wallFort = new WallElement("wallFort", "structures/athen_fortress", 2*PI/2, 5.1, 1.9);
wallStyles.brit.wallFort = new WallElement("wallFort", "structures/brit_fortress", PI, 2.8);
wallStyles.cart.wallFort = new WallElement("wallFort", "structures/cart_fortress", PI, 5.1, 1.6);
wallStyles.gaul.wallFort = new WallElement("wallFort", "structures/gaul_fortress", PI, 4.2, 1.5);
wallStyles.iber.wallFort = new WallElement("wallFort", "structures/iber_fortress", PI, 5, 0.2);
wallStyles.mace.wallFort = new WallElement("wallFort", "structures/mace_fortress", 2*PI/2, 5.1, 1.9);
wallStyles.maur.wallFort = new WallElement("wallFort", "structures/maur_fortress", PI, 5.5);
wallStyles.pers.wallFort = new WallElement("wallFort", "structures/pers_fortress", PI, 5.6, 1.9);
wallStyles.ptol.wallFort = new WallElement("wallFort", "structures/ptol_fortress", 2*PI/2, 5.1, 1.9);
wallStyles.rome.wallFort = new WallElement("wallFort", "structures/rome_fortress", PI, 6.3, 2.1);
wallStyles.sele.wallFort = new WallElement("wallFort", "structures/sele_fortress", 2*PI/2, 5.1, 1.9);
wallStyles.spart.wallFort = new WallElement("wallFort", "structures/spart_fortress", 2*PI/2, 5.1, 1.9);

// Adjust "rome_siege" style
wallStyles.rome_siege.wallFort = new WallElement("wallFort", "structures/rome_army_camp", PI, 7.2, 2);
wallStyles.rome_siege.entryFort = new WallElement("entryFort", "structures/rome_army_camp", PI, 12, 7);
wallStyles.rome_siege.house = new WallElement("house", "structures/rome_tent", PI, 0, 4);

// Add special wall styles not well to implement generic (and to show how custom styles can be added)

wallScaleByType.palisades = 0.55;
let gate = new WallElement("gate", "other/palisades_rocks_gate", PI, 3.6);
wallStyles.palisades = {
	"wall": new WallElement("wall", "other/palisades_rocks_medium", 0, 2.3),
	"wallMedium": new WallElement("wall", "other/palisades_rocks_medium", 0, 2.3),
	"wallLong": new WallElement("wall", "other/palisades_rocks_long", 0, 3.5),
	"wallShort": new WallElement("wall", "other/palisades_rocks_short", 0, 1.2),
	"tower": new WallElement("tower", "other/palisades_rocks_tower", -PI/2, 0.7),
	"wallFort": new WallElement("wallFort", "other/palisades_rocks_fort", PI, 1.7),
	"gate": gate,
	"entry": new WallElement("entry", undefined, gate.angle, gate.width),
	"entryTower": new WallElement("entryTower", "other/palisades_rocks_watchtower", 0, gate.width, -3),
	"entryFort": new WallElement("entryFort", "other/palisades_rocks_fort", PI, 6, 3),
	"cornerIn": new WallElement("cornerIn", "other/palisades_rocks_curve", 3*PI/4, 2.1, 0.7, PI/2),
	"cornerOut": new WallElement("cornerOut", "other/palisades_rocks_curve", 5*PI/4, 2.1, -0.7, -PI/2),
	"outpost": new WallElement("outpost", "other/palisades_rocks_outpost", PI, 0, -2),
	"house": new WallElement("house", "other/celt_hut", PI, 0, 5),
	"barracks": new WallElement("barracks", "structures/gaul_tavern", PI, 0, 5),
	"endRight": new WallElement("endRight", "other/palisades_rocks_end", -PI/2, 0.2),
	"endLeft": new WallElement("endLeft", "other/palisades_rocks_end", PI/2, 0.2)
};

// NOTE: This is not a wall style in the common sense. Use with care!
wallStyles.road = {
	"short": new WallElement("road", "actor|props/special/eyecandy/road_temperate_short.xml", PI/2, 4.5),
	"long": new WallElement("road", "actor|props/special/eyecandy/road_temperate_long.xml", PI/2, 9.5),

	// Correct width by -2*indent to fit xStraicht/corner
	"cornerLeft": new WallElement("road", "actor|props/special/eyecandy/road_temperate_corner.xml", -PI/2, 4.5-2*1.25, 1.25, PI/2),
	"cornerRight": new WallElement("road", "actor|props/special/eyecandy/road_temperate_corner.xml", 0, 4.5-2*1.25, -1.25, -PI/2),
	"curveLeft": new WallElement("road", "actor|props/special/eyecandy/road_temperate_curve_small.xml", -PI/2, 4.5+2*0.2, -0.2, PI/2),
	"curveRight": new WallElement("road", "actor|props/special/eyecandy/road_temperate_curve_small.xml", 0, 4.5+2*0.2, 0.2, -PI/2),

	"start": new WallElement("road", "actor|props/special/eyecandy/road_temperate_end.xml", PI/2, 2),
	"end": new WallElement("road", "actor|props/special/eyecandy/road_temperate_end.xml", -PI/2, 2),
	"xStraight": new WallElement("road", "actor|props/special/eyecandy/road_temperate_intersect_x.xml", 0, 4.5),
	"xLeft": new WallElement("road", "actor|props/special/eyecandy/road_temperate_intersect_x.xml", 0, 4.5, 0, PI/2),
	"xRight": new WallElement("road", "actor|props/special/eyecandy/road_temperate_intersect_x.xml", 0, 4.5, 0, -PI/2),
	"tLeft": new WallElement("road", "actor|props/special/eyecandy/road_temperate_intersect_T.xml", PI, 4.5, 1.25),
	"tRight": new WallElement("road", "actor|props/special/eyecandy/road_temperate_intersect_T.xml", 0, 4.5, -1.25),
};

// NOTE: This is not a wall style in the common sense. Use with care!
wallStyles.other = {
	"fence": new WallElement("fence", "other/fence_long", -PI/2, 3.1),
	"fence_medium": new WallElement("fence", "other/fence_long", -PI/2, 3.1),
	"fence_short": new WallElement("fence_short", "other/fence_short", -PI/2, 1.5),
	"fence_stone": new WallElement("fence_stone", "other/fence_stone", -PI/2, 2.5),
	"palisade": new WallElement("palisade", "other/palisades_rocks_short", 0, 1.2),
	"column": new WallElement("column", "other/column_doric", 0, 1),
	"obelisk": new WallElement("obelisk", "other/obelisk", 0, 2),
	"spike": new WallElement("spike", "other/palisades_angle_spike", -PI/2, 1),
	"bench": new WallElement("bench", "other/bench", PI/2, 1.5),
	"benchForTable": new WallElement("benchForTable", "other/bench", 0, 0.5),
	"table": new WallElement("table", "other/table_rectangle", 0, 1),
	"table_square": new WallElement("table_square", "other/table_square", PI/2, 1),
	"flag": new WallElement("flag", "special/rallypoint", PI, 1),
	"standing_stone": new WallElement("standing_stone", "gaia/special_ruins_standing_stone", PI, 1),
	"settlement": new WallElement("settlement", "gaia/special_settlement", PI, 6),
	"gap": new WallElement("gap", undefined, 0, 2),
	"gapSmall": new WallElement("gapSmall", undefined, 0, 1),
	"gapLarge": new WallElement("gapLarge", undefined, 0, 4),
	"cornerIn": new WallElement("cornerIn", undefined, 0, 0, 0, PI/2),
	"cornerOut": new WallElement("cornerOut", undefined, 0, 0, 0, -PI/2)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  fortressTypes data structure for some default fortress types
//
//	A fortress type is just an instance of the Fortress class with actually something in it
//	fortressTypes holds all the fortresses within an associative array with a descriptive string as key (e.g. matching the map size)
//	Examples: "tiny", "veryLarge"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
var fortressTypes = {};

{
	let wallParts = {
		"tiny": ["gate", "tower", "wallShort", "cornerIn", "wallShort", "tower"],
		"small": ["gate", "tower", "wall", "cornerIn", "wall", "tower"],
		"medium": ["gate", "tower", "wallLong", "cornerIn", "wallLong", "tower"],
		"normal": ["gate", "tower", "wall", "cornerIn", "wall",
		           "cornerOut", "wall", "cornerIn", "wall", "tower"],
		"large": ["gate", "tower", "wallLong", "cornerIn", "wallLong",
		          "cornerOut", "wallLong", "cornerIn", "wallLong", "tower"],
		"veryLarge": ["gate", "tower", "wall", "cornerIn", "wall", "cornerOut", "wallLong",
		              "cornerIn", "wallLong", "cornerOut", "wall", "cornerIn", "wall", "tower"],
		"giant": ["gate", "tower", "wallLong", "cornerIn", "wallLong", "cornerOut", "wallLong",
		          "cornerIn", "wallLong", "cornerOut", "wallLong", "cornerIn", "wallLong", "tower"]
	};

	for (let type in wallParts)
	{
		fortressTypes[type] = new Fortress(type);

		let wp = wallParts[type];
		fortressTypes[type].wall = wp.concat(wp, wp, wp);
	}
}

// Setup some better looking semi default fortresses for "palisades" style
for (let type in fortressTypes)
{
	let newKey = type + "Palisades";
	let oldWall = fortressTypes[type].wall;
	fortressTypes[newKey] = new Fortress(newKey);
	let fillTowersBetween = ["wallShort", "wall", "wallLong", "endLeft", "endRight", "cornerIn", "cornerOut"];
	for (let j = 0; j < oldWall.length; ++j)
	{
		fortressTypes[newKey].wall.push(oldWall[j]); // Only works if the first element is not in fillTowersBetween (e.g. entry or gate like it should be)
		if (j+1 < oldWall.length)
			if (fillTowersBetween.indexOf(oldWall[j]) > -1 && fillTowersBetween.indexOf(oldWall[j+1]) > -1) // ... > -1 means "exists" here
				fortressTypes[newKey].wall.push("tower");
	}
}

// Setup some balanced (to civ type fortresses) semi default fortresses for "palisades" style
// TODO

// Add some "fortress types" for roads (will only work with style "road")
{
	// ["start", "short", "xRight", "xLeft", "cornerLeft", "xStraight", "long", "xLeft", "xRight", "cornerRight", "tRight", "tLeft", "xRight", "xLeft", "curveLeft", "xStraight", "curveRight", "end"];
	let roadTypes = {
		"road01": ["short", "curveLeft", "short", "curveLeft", "short", "curveLeft", "short", "curveLeft"],
		"road02": ["short", "cornerLeft", "short", "cornerLeft", "short", "cornerLeft", "short", "cornerLeft"],
		"road03": ["xStraight", "curveLeft", "xStraight", "curveLeft", "xStraight", "curveLeft", "xStraight", "curveLeft"],
		"road04": ["start", "curveLeft", "tRight", "cornerLeft", "tRight", "curveRight", "short", "xRight", "curveLeft", "xRight", "short", "cornerLeft", "tRight", "short",
		           "curveLeft", "short", "tRight", "cornerLeft", "short", "xRight", "curveLeft", "xRight", "short", "curveRight", "tRight", "cornerLeft", "tRight", "curveLeft", "end"],
		"road05": ["start", "tLeft", "short", "xRight",
		           "curveLeft", "xRight", "tRight", "cornerLeft", "tRight",
		           "curveLeft", "short", "tRight", "cornerLeft", "xRight",
		           "cornerLeft", "xRight", "short", "tRight", "curveLeft", "end"],
	};

	for (let type in roadTypes)
		fortressTypes[type] = new Fortress(type, roadTypes[type]);
}

///////////////////////////////
// Define some helper functions
///////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  getWallAlignment
//
//	Returns a list of objects containing all information to place all the wall elements entities with placeObject (but the player ID)
//	Placing the first wall element at startX/startY placed with an angle given by orientation
//	An alignment can be used to get the "center" of a "wall" (more likely used for fortresses) with getCenterToFirstElement
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function getWallAlignment(startX, startY, wall, style, orientation)
{
	// Graciously handle arguments
	if (wall === undefined)
		wall = [];
	if (!wallStyles.hasOwnProperty(style))
	{
		warn("Function getWallAlignment: Unknown style: " + style + ' (falling back to "athen")');
		style = "athen";
	}
	orientation = orientation || 0;

	let alignment = [];
	let wallX = startX;
	let wallY = startY;
	for (let i = 0; i < wall.length; ++i)
	{
		let element = wallStyles[style][wall[i]];
		if (element === undefined && i == 0)
			warn("No valid wall element: " + wall[i]);

		// Indentation
		let placeX = wallX - element.indent * cos(orientation);
		let placeY = wallY - element.indent * sin(orientation);

		// Add wall elements entity placement arguments to the alignment
		alignment.push({
			"x": placeX,
			"y": placeY,
			"entity": element.entity,
			"angle": orientation + element.angle
		});

		// Preset vars for the next wall element
		if (i+1 < wall.length)
		{
			orientation += element.bending;
			let nextElement = wallStyles[style][wall[i+1]];
			if (nextElement === undefined)
				warn("No valid wall element: " + wall[i+1]);
			let distance = (element.width + nextElement.width)/2;
			// Corrections for elements with indent AND bending
			let indent = element.indent;
			let bending = element.bending;
			if (bending !== 0 && indent !== 0)
			{
				// Indent correction to adjust distance
				distance += indent*sin(bending);
				// Indent correction to normalize indentation
				wallX += indent * cos(orientation);
				wallY += indent * sin(orientation);
			}
			// Set the next coordinates of the next element in the wall without indentation adjustment
			wallX -= distance * sin(orientation);
			wallY += distance * cos(orientation);
		}
	}
	return alignment;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  getCenterToFirstElement
//
//	Center calculation works like getting the center of mass assuming all wall elements have the same "weight"
//
//	It returns the vector from the center to the first wall element
//	Used to get centerToFirstElement of fortresses by default
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function getCenterToFirstElement(alignment)
{
	let centerToFirstElement = { "x": 0, "y": 0 };
	for (let i = 0; i < alignment.length; ++i)
	{
		centerToFirstElement.x -= alignment[i].x/alignment.length;
		centerToFirstElement.y -= alignment[i].y/alignment.length;
	}
	return centerToFirstElement;
}

//////////////////////////////////////////////////////////////////
//  getWallLength
//
//	NOTE: Does not support bending wall elements like corners!
//	e.g. used by placeIrregularPolygonalWall
//////////////////////////////////////////////////////////////////
function getWallLength(wall, style)
{
	// Graciously handle arguments
	if (wall === undefined)
		wall = [];
	if (!wallStyles.hasOwnProperty(style))
	{
		warn("Function getWallLength: Unknown style: " + style + ' (falling back to "athen")');
		style = "athen";
	}

	let length = 0;
	for (let i = 0; i < wall.length; ++i)
		length += wallStyles[style][wall[i]].width;

	return length;
}

/////////////////////////////////////////////
// Define the different wall placer functions
/////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  placeWall
//
//	Places a wall with wall elements attached to another like determined by WallElement properties.
//
//	startX, startY  Where the first wall element should be placed
//	wall            Array of wall element type strings. Example: ["endLeft", "wallLong", "tower", "wallLong", "endRight"]
//	style           Optional. Wall style string. Default is the civ of the given player, "palisades" for gaia
//	playerId        Optional. Number of the player the wall will be placed for. Default is 0 (gaia)
//	orientation     Optional. Angle the first wall element is placed. Default is 0
//	                0 means "outside" or "front" of the wall is right (positive X) like placeObject
//	                It will then be build towards top/positive Y (if no bending wall elements like corners are used)
//	                Raising orientation means the wall is rotated counter-clockwise like placeObject
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placeWall(startX, startY, wall, style, playerId, orientation)
{
	// Graciously handle arguments
	if (wall === undefined)
		wall = [];
	playerId = playerId || 0;
	if (!wallStyles.hasOwnProperty(style))
	{
		if (playerId == 0)
			style = style || "palisades";
		else
			style = getCivCode(playerId);
	}
	orientation = orientation || 0;

	// Get wall alignment
	let AM = getWallAlignment(startX, startY, wall, style, orientation);
	// Place the wall
	for (let iWall = 0; iWall < wall.length; ++iWall)
	{
		let entity = AM[iWall].entity;
		if (entity !== undefined)
			placeObject(AM[iWall].x, AM[iWall].y, entity, playerId, AM[iWall].angle);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  placeCustomFortress
//
//	Place a fortress (mainly a closed wall build like placeWall) centered at centerX/centerY
//	The fortress wall should always start with the main entrance (like "entry" or "gate") to get the orientation right (like placeObject)
//
//	fortress       An instance of Fortress with a wall defined
//	style          Optional. Wall style string. Default is the civ of the given player, "palisades" for gaia
//	playerId       Optional. Number of the player the wall will be placed for. Default is 0 (gaia)
//	orientation    Optional. Angle the first wall element (should be a gate or entrance) is placed. Default is BUILDING_ORIENTATION
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placeCustomFortress(centerX, centerY, fortress, style, playerId = 0, orientation = BUILDING_ORIENTATION)
{
	// Graciously handle arguments
	fortress = fortress || fortressTypes.medium;
	if (!wallStyles.hasOwnProperty(style))
	{
		if (playerId == 0)
			style = style || "palisades";
		else
			style = getCivCode(playerId);
	}

	// Calculate center if fortress.centerToFirstElement is undefined (default)
	let centerToFirstElement = fortress.centerToFirstElement;
	if (centerToFirstElement === undefined)
		centerToFirstElement = getCenterToFirstElement(getWallAlignment(0, 0, fortress.wall, style));
	// Placing the fortress wall
	let startX = centerX + centerToFirstElement.x * cos(orientation) - centerToFirstElement.y * sin(orientation);
	let startY = centerY + centerToFirstElement.y * cos(orientation) + centerToFirstElement.x * sin(orientation);
	placeWall(startX, startY, fortress.wall, style, playerId, orientation);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  placeFortress
//
//	Like placeCustomFortress just it takes type (a fortress type string, has to be in fortressTypes) instead of an instance of Fortress
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placeFortress(centerX, centerY, type, style, playerId, orientation)
{
	// Graciously handle arguments
	type = type || "medium";
	playerId = playerId || 0;
	if (!wallStyles.hasOwnProperty(style))
	{
		if (playerId == 0)
			style = style || "palisades";
		else
			style = getCivCode(playerId);
	}
	orientation = orientation || 0;

	// Call placeCustomFortress with the given arguments
	placeCustomFortress(centerX, centerY, fortressTypes[type], style, playerId, orientation);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  placeLinearWall
//
//	Places a straight wall from a given coordinate to an other repeatedly using the wall parts.
//
//	startX/startY    Coordinate of the approximate beginning of the wall (Not the place of the first wall element)
//	targetX/targetY  Coordinate of the approximate ending of the wall (Not the place of the last wall element)
//	wallPart         Optional. An array of NON-BENDING wall element type strings. Default is ["tower", "wallLong"]
//	style            Optional. Wall style string. Default is the civ of the given player, "palisades" for gaia
//	playerId         Optional. Integer number of the player. Default is 0 (gaia)
//	endWithFirst     Optional. A boolean value. If true the 1st wall element in the wallPart array will finalize the wall. Default is true
//
//	TODO: Maybe add angle offset for more generic looking?
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placeLinearWall(startX, startY, targetX, targetY, wallPart, style, playerId, endWithFirst)
{
	// Setup optional arguments to the default
	wallPart = wallPart || ["tower", "wallLong"];
	playerId = playerId || 0;
	if (!wallStyles.hasOwnProperty(style))
	{
		if (playerId == 0)
			style = style || "palisades";
		else
			style = getCivCode(playerId);
	}
	endWithFirst = typeof endWithFirst == "undefined" ? true : endWithFirst;

	// Check arguments
	for (let elementIndex = 0; elementIndex < wallPart.length; ++elementIndex)
	{
		let bending = wallStyles[style][wallPart[elementIndex]].bending;
		if (bending != 0)
			warn("Bending is not supported by placeLinearWall but a bending wall element is used: " + wallPart[elementIndex] + " -> wallStyles[style][wallPart[elementIndex]].entity");
	}
	// Setup number of wall parts
	let totalLength = Math.euclidDistance2D(startX, startY, targetX, targetY);
	let wallPartLength = 0;
	for (let elementIndex = 0; elementIndex < wallPart.length; ++elementIndex)
		wallPartLength += wallStyles[style][wallPart[elementIndex]].width;
	let numParts = 0;
	if (endWithFirst)
		numParts = ceil((totalLength - wallStyles[style][wallPart[0]].width) / wallPartLength);
	else
		numParts = ceil(totalLength / wallPartLength);
	// Setup scale factor
	let scaleFactor = 1;
	if (endWithFirst)
		scaleFactor = totalLength / (numParts * wallPartLength + wallStyles[style][wallPart[0]].width);
	else
		scaleFactor = totalLength / (numParts * wallPartLength);
	// Setup angle
	let wallAngle = getAngle(startX, startY, targetX, targetY); // NOTE: function "getAngle()" is about to be changed...
	let placeAngle = wallAngle - PI/2;
	// Place wall entities
	let x = startX;
	let y = startY;
	for (let partIndex = 0; partIndex < numParts; ++partIndex)
	{
		for (let elementIndex = 0; elementIndex < wallPart.length; ++elementIndex)
		{
			let wallEle = wallStyles[style][wallPart[elementIndex]];
			// Width correction
			x += scaleFactor * wallEle.width/2 * cos(wallAngle);
			y += scaleFactor * wallEle.width/2 * sin(wallAngle);
			// Indent correction
			let placeX = x - wallEle.indent * sin(wallAngle);
			let placeY = y + wallEle.indent * cos(wallAngle);
			// Placement
			let entity = wallEle.entity;
			if (entity !== undefined)
				placeObject(placeX, placeY, entity, playerId, placeAngle + wallEle.angle);
			x += scaleFactor * wallEle.width/2 * cos(wallAngle);
			y += scaleFactor * wallEle.width/2 * sin(wallAngle);
		}
	}
	if (endWithFirst)
	{
		let wallEle = wallStyles[style][wallPart[0]];
		x += scaleFactor * wallEle.width/2 * cos(wallAngle);
		y += scaleFactor * wallEle.width/2 * sin(wallAngle);
		let entity = wallEle.entity;
		if (entity !== undefined)
			placeObject(x, y, entity, playerId, placeAngle + wallEle.angle);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  placeCircularWall
//
//	Place a circular wall of repeated wall elements given in the argument wallPart around centerX/centerY with the given radius
//	The wall can be opened forming more an arc than a circle if maxAngle < 2*PI
//	The orientation then determines where this open part faces (0 means right like unrotated building's drop-points)
//
//	centerX/Y     Coordinates of the circle's center
//	radius        How wide the circle should be (approximate, especially if maxBendOff != 0)
//	wallPart      Optional. An array of NON-BENDING wall element type strings. Default is ["tower", "wallLong"]
//	style         Optional. Wall style string. Default is the civ of the given player, "palisades" for gaia
//	playerId      Optional. Integer number of the player. Default is 0 (gaia)
//	orientation   Optional. Where the open part of the (circular) arc should face (if maxAngle is < 2*PI). Default is 0
//	maxAngle      Optional. How far the wall should circumvent the center. Default is 2*PI (full circle)
//	endWithFirst  Optional. Boolean. If true the 1st wall element in the wallPart array will finalize the wall. Default is false for full circles, else true
//	maxBendOff    Optional. How irregular the circle should be. 0 means regular circle, PI/2 means very irregular. Default is 0 (regular circle)
//
//	NOTE: Don't use wall elements with bending like corners!
//	TODO: Perhaps add eccentricity and maxBendOff functionality (untill now an unused argument)
//	TODO: Perhaps add functionality for spirals
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placeCircularWall(centerX, centerY, radius, wallPart, style, playerId, orientation, maxAngle, endWithFirst, maxBendOff)
{
	// Setup optional arguments to the default
	wallPart = wallPart || ["tower", "wallLong"];
	playerId = playerId || 0;
	if (!wallStyles.hasOwnProperty(style))
	{
		if (playerId == 0)
			style = style || "palisades";
		else
			style = getCivCode(playerId);
	}
	orientation = orientation || 0;
	maxAngle = maxAngle || 2*PI;

	if (endWithFirst === undefined)
		endWithFirst = maxAngle < 2*PI - 0.001; // Can this be done better?

	maxBendOff = maxBendOff || 0;

	// Check arguments
	if (maxBendOff > PI/2 || maxBendOff < 0)
		warn("placeCircularWall maxBendOff sould satisfy 0 < maxBendOff < PI/2 (~1.5) but it is: " + maxBendOff);
	for (let elementIndex = 0; elementIndex < wallPart.length; ++elementIndex)
	{
		let bending = wallStyles[style][wallPart[elementIndex]].bending;
		if (bending != 0)
			warn("Bending is not supported by placeCircularWall but a bending wall element is used: " + wallPart[elementIndex]);
	}
	// Setup number of wall parts
	let totalLength = maxAngle * radius;
	let wallPartLength = 0;
	for (let elementIndex = 0; elementIndex < wallPart.length; ++elementIndex)
		wallPartLength += wallStyles[style][wallPart[elementIndex]].width;
	let numParts = 0;
	if (endWithFirst)
		numParts = ceil((totalLength - wallStyles[style][wallPart[0]].width) / wallPartLength);
	else
		numParts = ceil(totalLength / wallPartLength);

	// Setup scale factor
	let scaleFactor = 1;
	if (endWithFirst)
		scaleFactor = totalLength / (numParts * wallPartLength + wallStyles[style][wallPart[0]].width);
	else
		scaleFactor = totalLength / (numParts * wallPartLength);

	// Place wall entities
	let actualAngle = orientation + (2*PI - maxAngle) / 2;
	let x = centerX + radius*cos(actualAngle);
	let y = centerY + radius*sin(actualAngle);
	for (let partIndex = 0; partIndex < numParts; ++partIndex)
		for (let elementIndex = 0; elementIndex < wallPart.length; ++elementIndex)
		{
			let wallEle = wallStyles[style][wallPart[elementIndex]];
			// Width correction
			let addAngle = scaleFactor * wallEle.width / radius;
			let targetX = centerX + radius * cos(actualAngle + addAngle);
			let targetY = centerY + radius * sin(actualAngle + addAngle);
			let placeX = x + (targetX - x)/2;
			let placeY = y + (targetY - y)/2;
			let placeAngle = actualAngle + addAngle/2;
			// Indent correction
			placeX -= wallEle.indent * cos(placeAngle);
			placeY -= wallEle.indent * sin(placeAngle);
			// Placement
			let entity = wallEle.entity;
			if (entity !== undefined)
				placeObject(placeX, placeY, entity, playerId, placeAngle + wallEle.angle);
			// Prepare for the next wall element
			actualAngle += addAngle;
			x = centerX + radius*cos(actualAngle);
			y = centerY + radius*sin(actualAngle);
		}

	if (endWithFirst)
	{
		let wallEle = wallStyles[style][wallPart[0]];
		let addAngle = scaleFactor * wallEle.width / radius;
		let targetX = centerX + radius * cos(actualAngle + addAngle);
		let targetY = centerY + radius * sin(actualAngle + addAngle);
		let placeX = x + (targetX - x)/2;
		let placeY = y + (targetY - y)/2;
		let placeAngle = actualAngle + addAngle/2;
		placeObject(placeX, placeY, wallEle.entity, playerId, placeAngle + wallEle.angle);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  placePolygonalWall
//
//	Place a polygonal wall of repeated wall elements given in the argument wallPart around centerX/centerY with the given radius
//
//	centerX/Y          Coordinates of the polygon's center
//	radius             How wide the circle should be in which the polygon fits
//	wallPart           Optional. An array of NON-BENDING wall element type strings. Default is ["wallLong", "tower"]
//	cornerWallElement  Optional. Wall element to be placed at the polygon's corners. Default is "tower"
//	style              Optional. Wall style string. Default is the civ of the given player, "palisades" for gaia
//	playerId           Optional. Integer number of the player. Default is 0 (gaia)
//	orientation        Optional. Angle from the center to the first linear wall part placed. Default is 0 (towards positive X/right)
//	numCorners         Optional. How many corners the polygon will have. Default is 8 (matching a civ centers territory)
//	skipFirstWall      Optional. Boolean. If the first linear wall part will be left opened as entrance. Default is true
//
//	NOTE: Don't use wall elements with bending like corners!
//	TODO: Replace skipFirstWall with firstWallPart to enable gate/defended entrance placement
//	TODO: Check some arguments
//	TODO: Add eccentricity and perhaps make it just call placeIrregularPolygonalWall with irregularity = 0
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placePolygonalWall(centerX, centerY, radius, wallPart, cornerWallElement, style, playerId, orientation, numCorners, skipFirstWall = true)
{
	// Setup optional arguments to the default
	wallPart = wallPart || ["wallLong", "tower"];
	cornerWallElement = cornerWallElement || "tower"; // Don't use wide elements for this. Not supported well...
	playerId = playerId || 0;

	if (!wallStyles.hasOwnProperty(style))
	{
		if (playerId == 0)
			style = style || "palisades";
		else
			style = getCivCode(playerId);
	}
	orientation = orientation || 0;
	numCorners = numCorners || 8;

	// Setup angles
	let angleAdd = 2*PI/numCorners;
	let angleStart = orientation - angleAdd/2;
	// Setup corners
	let corners = [];
	for (let i = 0; i < numCorners; ++i)
		corners.push([centerX + radius*cos(angleStart + i*angleAdd), centerY + radius*sin(angleStart + i*angleAdd)]);
	// Place Corners and walls
	for (let i = 0; i < numCorners; ++i)
	{
		let angleToCorner = getAngle(corners[i][0], corners[i][1], centerX, centerY);
		placeObject(corners[i][0], corners[i][1], wallStyles[style][cornerWallElement].entity, playerId, angleToCorner);
		if (!skipFirstWall || i != 0)
			placeLinearWall(
				// Adjustment to the corner element width (approximately)
				corners[i][0] + wallStyles[style][cornerWallElement].width/2 * sin(angleToCorner + angleAdd/2), // startX
				corners[i][1] - wallStyles[style][cornerWallElement].width/2 * cos(angleToCorner + angleAdd/2), // startY
				corners[(i+1)%numCorners][0] - wallStyles[style][cornerWallElement].width/2 * sin(angleToCorner + angleAdd/2), // targetX
				corners[(i+1)%numCorners][1] + wallStyles[style][cornerWallElement].width/2 * cos(angleToCorner + angleAdd/2), // targetY
				wallPart, style, playerId);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  placeIrregularPolygonalWall
//
//	Place an irregular polygonal wall of some wall parts to choose from around centerX/centerY with the given radius
//
//	centerX/Y            Coordinates of the polygon's center
//	radius               How wide the circle should be in which the polygon fits
//	cornerWallElement    Optional. Wall element to be placed at the polygon's corners. Default is "tower"
//	style                Optional. Wall style string. Default is the civ of the given player, "palisades" for gaia
//	playerId             Optional. Integer number of the player. Default is 0 (gaia)
//	orientation          Optional. Angle from the center to the first linear wall part placed. Default is 0 (towards positive X/right)
//	numCorners           Optional. How many corners the polygon will have. Default is 8 (matching a civ centers territory)
//	irregularity         Optional. How irregular the polygon will be. 0 means regular, 1 means VERY irregular. Default is 0.5
//	skipFirstWall        Optional. Boolean. If the first linear wall part will be left opened as entrance. Default is true
//	wallPartsAssortment  Optional. An array of wall part arrays to choose from for each linear wall connecting the corners. Default is hard to describe ^^
//
//	NOTE: wallPartsAssortment is put to the end because it's hardest to set
//	NOTE: Don't use wall elements with bending like corners!
//	TODO: Replace skipFirstWall with firstWallPart to enable gate/defended entrance placement
//	TODO: Check some arguments
//	TODO: Perhaps add eccentricity
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placeIrregularPolygonalWall(centerX, centerY, radius, cornerWallElement, style, playerId, orientation, numCorners, irregularity, skipFirstWall, wallPartsAssortment)
{
	// Setup optional arguments
	playerId = playerId || 0;
	if (!wallStyles.hasOwnProperty(style))
	{
		if (playerId == 0)
			style = style || "palisades";
		else
			style = getCivCode(playerId);
	}

	// Generating a generic wall part assortment with each wall part including 1 gate lengthened by walls and towers
	// NOTE: It might be a good idea to write an own function for that...
	let defaultWallPartsAssortment = [["wallShort"], ["wall"], ["wallLong"], ["gate", "tower", "wallShort"]];
	let centeredWallPart = ["gate"];
	let extandingWallPartAssortment = [["tower", "wallLong"], ["tower", "wall"]];
	defaultWallPartsAssortment.push(centeredWallPart);
	for (let i = 0; i < extandingWallPartAssortment.length; ++i)
	{
		let wallPart = centeredWallPart;
		for (let j = 0; j < radius; ++j)
		{
			if (j%2 == 0)
				wallPart = wallPart.concat(extandingWallPartAssortment[i]);
			else
			{
				extandingWallPartAssortment[i].reverse();
				wallPart = extandingWallPartAssortment[i].concat(wallPart);
				extandingWallPartAssortment[i].reverse();
			}
			defaultWallPartsAssortment.push(wallPart);
		}
	}
	// Setup optional arguments to the default
	wallPartsAssortment = wallPartsAssortment || defaultWallPartsAssortment;
	cornerWallElement = cornerWallElement || "tower"; // Don't use wide elements for this. Not supported well...
	style = style || "palisades";
	playerId = playerId || 0;
	orientation = orientation || 0;
	numCorners = numCorners || randIntInclusive(5, 7);
	irregularity = irregularity || 0.5;
	skipFirstWall = skipFirstWall || false;

	// Setup angles
	let angleToCover = 2*PI;
	let angleAddList = [];
	for (let i = 0; i < numCorners; ++i)
	{
		// Randomize covered angles. Variety scales down with raising angle though...
		angleAddList.push(angleToCover/(numCorners-i) * (1 + randFloat(-irregularity, irregularity)));
		angleToCover -= angleAddList[angleAddList.length - 1];
	}
	// Setup corners
	let corners = [];
	let angleActual = orientation - angleAddList[0]/2;
	for (let i = 0; i < numCorners; ++i)
	{
		corners.push([centerX + radius*cos(angleActual), centerY + radius*sin(angleActual)]);
		if (i < numCorners - 1)
			angleActual += angleAddList[i+1];
	}
	// Setup best wall parts for the different walls (a bit confusing naming...)
	let wallPartLengths = [];
	let maxWallPartLength = 0;
	for (let partIndex = 0; partIndex < wallPartsAssortment.length; ++partIndex)
	{
		let length = wallPartLengths[partIndex];
		wallPartLengths.push(getWallLength(wallPartsAssortment[partIndex], style));
		if (length > maxWallPartLength)
			maxWallPartLength = length;
	}
	let wallPartList = []; // This is the list of the wall parts to use for the walls between the corners, not to confuse with wallPartsAssortment!
	for (let i = 0; i < numCorners; ++i)
	{
		let bestWallPart = []; // This is a simpel wall part not a wallPartsAssortment!
		let bestWallLength = 99999999;
		// NOTE: This is not exactly like the length the wall will be in the end. Has to be tweaked...
		let wallLength = Math.euclidDistance2D(corners[i][0], corners[i][1], corners[(i + 1) % numCorners][0], corners[(i + 1) % numCorners][1]);
		let numWallParts = ceil(wallLength/maxWallPartLength);
		for (let partIndex = 0; partIndex < wallPartsAssortment.length; ++partIndex)
		{
			let linearWallLength = numWallParts*wallPartLengths[partIndex];
			if (linearWallLength < bestWallLength && linearWallLength > wallLength)
			{
				bestWallPart = wallPartsAssortment[partIndex];
				bestWallLength = linearWallLength;
			}
		}
		wallPartList.push(bestWallPart);
	}
	// Place Corners and walls
	for (let i = 0; i < numCorners; ++i)
	{
		let angleToCorner = getAngle(corners[i][0], corners[i][1], centerX, centerY);
		placeObject(corners[i][0], corners[i][1], wallStyles[style][cornerWallElement].entity, playerId, angleToCorner);
		if (!skipFirstWall || i != 0)
			placeLinearWall(
				// Adjustment to the corner element width (approximately)
				corners[i][0] + wallStyles[style][cornerWallElement].width/2 * sin(angleToCorner + angleAddList[i]/2), // startX
				corners[i][1] - wallStyles[style][cornerWallElement].width/2 * cos(angleToCorner + angleAddList[i]/2), // startY
				corners[(i+1)%numCorners][0] - wallStyles[style][cornerWallElement].width/2 * sin(angleToCorner + angleAddList[(i+1)%numCorners]/2), // targetX
				corners[(i+1)%numCorners][1] + wallStyles[style][cornerWallElement].width/2 * cos(angleToCorner + angleAddList[(i+1)%numCorners]/2), // targetY
				wallPartList[i], style, playerId, false);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  placeGenericFortress
//
//	Places a generic fortress with towers at the edges connected with long walls and gates (entries until gates work)
//	This is the default Iberian civ bonus starting wall
//
//	centerX/Y      The approximate center coordinates of the fortress
//	radius         The approximate radius of the wall to be placed
//	playerId       Optional. Integer number of the player. Default is 0 (gaia)
//	style          Optional. Wall style string. Default is the civ of the given player, "palisades" for gaia
//	irregularity   Optional. Float between 0 (circle) and 1 (very spiky), default is 1/2
//	gateOccurence  Optional. Integer number, every n-th walls will be a gate instead. Default is 3
//	maxTrys        Optional. How often the function tries to find a better fitting shape at max. Default is 100
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
function placeGenericFortress(centerX, centerY, radius, playerId, style, irregularity, gateOccurence, maxTrys)
{
	// Setup optional arguments
	radius = radius || 20;
	playerId = playerId || 0;
	if (!wallStyles.hasOwnProperty(style))
	{
		if (playerId == 0)
			style = style || "palisades";
		else
			style = getCivCode(playerId);
	}
	irregularity = irregularity || 1/2;
	gateOccurence = gateOccurence || 3;
	maxTrys = maxTrys || 100;

	// Setup some vars
	let startAngle = randFloat(0, 2*PI);
	let actualOffX = radius*cos(startAngle);
	let actualOffY = radius*sin(startAngle);
	let actualAngle = startAngle;
	let pointDistance = wallStyles[style].wallLong.width + wallStyles[style].tower.width;
	// Searching for a well fitting point derivation
	let tries = 0;
	let bestPointDerivation = undefined;
	let minOverlap = 1000;
	let overlap = undefined;
	while (tries < maxTrys && minOverlap > wallStyles[style].tower.width / 10)
	{
		let pointDerivation = [];
		let distanceToTarget = 1000;
		let targetReached = false;
		while (!targetReached)
		{
			let indent = randFloat(-irregularity*pointDistance, irregularity*pointDistance);
			let tmpAngle = getAngle(actualOffX, actualOffY,
				(radius + indent)*cos(actualAngle + (pointDistance / radius)),
				(radius + indent)*sin(actualAngle + (pointDistance / radius)));
			actualOffX += pointDistance*cos(tmpAngle);
			actualOffY += pointDistance*sin(tmpAngle);
			actualAngle = getAngle(0, 0, actualOffX, actualOffY);
			pointDerivation.push([actualOffX, actualOffY]);
			distanceToTarget = Math.euclidDistance2D(actualOffX, actualOffY, ...pointDerivation[0]);
			let numPoints = pointDerivation.length;
			if (numPoints > 3 && distanceToTarget < pointDistance) // Could be done better...
			{
				targetReached = true;
				overlap = pointDistance - Math.euclidDistance2D(...pointDerivation[numPoints - 1], ...pointDerivation[0]);
				if (overlap < minOverlap)
				{
					minOverlap = overlap;
					bestPointDerivation = pointDerivation;
				}
			}
		}
		+tries;
	}
	log("placeGenericFortress: Reduced overlap to " + minOverlap + " after " + tries + " tries");
	// Place wall
	for (let pointIndex = 0; pointIndex < bestPointDerivation.length; ++pointIndex)
	{
		let startX = centerX + bestPointDerivation[pointIndex][0];
		let startY = centerY + bestPointDerivation[pointIndex][1];
		let targetX = centerX + bestPointDerivation[(pointIndex + 1) % bestPointDerivation.length][0];
		let targetY = centerY + bestPointDerivation[(pointIndex + 1) % bestPointDerivation.length][1];
		let angle = getAngle(startX, startY, targetX, targetY);
		let wallElement = "wallLong";
		if ((pointIndex + 1) % gateOccurence == 0)
			wallElement = "gate";
		let entity = wallStyles[style][wallElement].entity;
		if (entity)
			placeObject(
				startX + (Math.euclidDistance2D(startX, startY, targetX, targetY) / 2) * Math.cos(angle),
				startY + (Math.euclidDistance2D(startX, startY, targetX, targetY) / 2) * Math.sin(angle),
				entity,
				playerId,
				angle - Math.PI / 2 + wallStyles[style][wallElement].angle);

		// Place tower
		startX = centerX + bestPointDerivation[(pointIndex + bestPointDerivation.length - 1) % bestPointDerivation.length][0];
		startY = centerY + bestPointDerivation[(pointIndex + bestPointDerivation.length - 1) % bestPointDerivation.length][1];
		angle = getAngle(startX, startY, targetX, targetY);
		placeObject(
			centerX + bestPointDerivation[pointIndex][0],
			centerY + bestPointDerivation[pointIndex][1],
			wallStyles[style].tower.entity,
			playerId,
			angle - PI/2 + wallStyles[style].tower.angle);
	}
}
