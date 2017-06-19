const PI = Math.PI;
const TWO_PI = 2 * Math.PI;
const TERRAIN_SEPARATOR = "|";
const SEA_LEVEL = 20.0;
const CELL_SIZE = 4;
const HEIGHT_UNITS_PER_METRE = 92;
const MIN_MAP_SIZE = 128;
const MAX_MAP_SIZE = 512;
const MAP_BORDER_WIDTH = 3;
const FALLBACK_CIV = "athen";
/**
 * Constants needed for heightmap_manipulation.js
 */
const MAX_HEIGHT_RANGE = 0xFFFF / HEIGHT_UNITS_PER_METRE; // Engine limit, Roughly 700 meters
const MIN_HEIGHT = - SEA_LEVEL;
const MAX_HEIGHT = MAX_HEIGHT_RANGE - SEA_LEVEL;
// Default angle for buildings
const BUILDING_ORIENTATION = - PI / 4;

function fractionToTiles(f)
{
	return g_Map.size * f;
}

function tilesToFraction(t)
{
	return t / g_Map.size;
}

function fractionToSize(f)
{
	return getMapArea() * f;
}

function sizeToFraction(s)
{
	return s / getMapArea();
}

function scaleByMapSize(min, max)
{
	return min + (max - min) * (g_Map.size - MIN_MAP_SIZE) / (MAX_MAP_SIZE - MIN_MAP_SIZE);
}

function cos(x)
{
	return Math.cos(x);
}

function sin(x)
{
	return Math.sin(x);
}

function abs(x) {
	return Math.abs(x);
}

function round(x)
{
	return Math.round(x);
}

function lerp(a, b, t)
{
	return a + (b-a) * t;
}

function sqrt(x)
{
	return Math.sqrt(x);
}

function ceil(x)
{
	return Math.ceil(x);
}

function floor(x)
{
	return Math.floor(x);
}

function max(a, b)
{
	return a > b ? a : b;
}

function min(a, b)
{
	return a < b ? a : b;
}

/**
 * Retries the given function with those arguments as often as specified.
 */
function retryPlacing(placeFunc, placeArgs, retryFactor, amount, getResult)
{
	let maxFail = amount * retryFactor;

	let results = [];
	let good = 0;
	let bad = 0;

	while (good < amount && bad <= maxFail)
	{
		let result = placeFunc(placeArgs);

		if (result !== undefined)
		{
			++good;
			if (getResult)
				results.push(result);
		}
		else
			++bad;
	}
	return getResult ? results : good;
}

/**
 * Helper function for randomly placing areas and groups on the map.
 */
function randomizePlacerCoordinates(placer, halfMapSize)
{
	if (!!g_MapSettings.CircularMap)
	{
		// Polar coordinates
		// Uniformly distributed on the disk
		let r = halfMapSize * Math.sqrt(randFloat(0, 1));
		let theta = randFloat(0, 2 * PI);
		placer.x = Math.floor(r * Math.cos(theta)) + halfMapSize;
		placer.z = Math.floor(r * Math.sin(theta)) + halfMapSize;
	}
	else
	{
		// Rectangular coordinates
		placer.x = randIntExclusive(0, g_Map.size);
		placer.z = randIntExclusive(0, g_Map.size);
	}
}

/**
 * Helper function for randomly placing areas and groups in the given areas.
 */
function randomizePlacerCoordinatesFromAreas(placer, areas)
{
	let pt = pickRandom(pickRandom(areas).points);
	placer.x = pt.x;
	placer.z = pt.z;
}

/**
 * Attempts to place the given number of areas in random places of the map.
 * Returns actually placed areas.
 */
function createAreas(centeredPlacer, painter, constraint, amount, retryFactor = 10)
{
	let placeFunc = function (args) {
		randomizePlacerCoordinates(args.placer, args.halfMapSize);
		return g_Map.createArea(args.placer, args.painter, args.constraint);
	};

	let args = {
		"placer": centeredPlacer,
		"painter": painter,
		"constraint": constraint,
		"halfMapSize": g_Map.size / 2
	};

	return retryPlacing(placeFunc, args, retryFactor, amount, true);
}

/**
 * Attempts to place the given number of areas in random places of the given areas.
 * Returns actually placed areas.
 */
function createAreasInAreas(centeredPlacer, painter, constraint, amount, retryFactor, areas)
{
	if (!areas.length)
		return [];

	let placeFunc = function (args) {
		randomizePlacerCoordinatesFromAreas(args.placer, args.areas);
		return g_Map.createArea(args.placer, args.painter, args.constraint);
	};

	let args = {
		"placer": centeredPlacer,
		"painter": painter,
		"constraint": constraint,
		"areas": areas,
		"halfMapSize": g_Map.size / 2
	};

	return retryPlacing(placeFunc, args, retryFactor, amount, true);
}

/**
 * Attempts to place the given number of groups in random places of the map.
 * Returns the number of actually placed groups.
 */
function createObjectGroups(placer, player, constraint, amount, retryFactor = 10)
{
	let placeFunc = function (args) {
		randomizePlacerCoordinates(args.placer, args.halfMapSize);
		return createObjectGroup(args.placer, args.player, args.constraint);
	};

	let args = {
		"placer": placer,
		"player": player,
		"constraint": constraint,
		"halfMapSize": g_Map.size / 2 - 3
	};

	return retryPlacing(placeFunc, args, retryFactor, amount, false);
}

/**
 * Attempts to place the given number of groups in random places of the given areas.
 * Returns the number of actually placed groups.
 */
function createObjectGroupsByAreas(placer, player, constraint, amount, retryFactor, areas)
{
	if (!areas.length)
		return 0;

	let placeFunc = function (args) {
		randomizePlacerCoordinatesFromAreas(args.placer, args.areas);
		return createObjectGroup(args.placer, args.player, args.constraint);
	};

	let args = {
		"placer": placer,
		"player": player,
		"constraint": constraint,
		"areas": areas
	};

	return retryPlacing(placeFunc, args, retryFactor, amount, false);
}

function createTerrain(terrain)
{
	if (!(terrain instanceof Array))
		return createSimpleTerrain(terrain);

	return new RandomTerrain(terrain.map(t => createTerrain(t)));
}

function createSimpleTerrain(terrain)
{
	if (typeof(terrain) != "string")
		throw("createSimpleTerrain expects string as input, received "+terrain);

	// Split string by pipe | character, this allows specifying terrain + tree type in single string
	let params = terrain.split(TERRAIN_SEPARATOR, 2);

	if (params.length != 2)
		return new SimpleTerrain(terrain);

	return new SimpleTerrain(params[0], params[1]);
}

function placeObject(x, z, type, player, angle)
{
	if (g_Map.validT(x, z, MAP_BORDER_WIDTH))
		g_Map.addObject(new Entity(type, player, x, z, angle));
}

function placeTerrain(x, z, terrain)
{
	// convert terrain param into terrain object
	g_Map.placeTerrain(x, z, createTerrain(terrain));
}

function isCircularMap()
{
	return !!g_MapSettings.CircularMap;
}

function getMapBaseHeight()
{
	return g_MapSettings.BaseHeight || 0;
}

function createTileClass()
{
	return g_Map.createTileClass();
}

function getTileClass(id)
{
	if (!g_Map.validClass(id))
		return undefined;

	return g_Map.tileClasses[id];
}

function createArea(placer, painter, constraint)
{
	return g_Map.createArea(placer, painter, constraint);
}

function createObjectGroup(placer, player, constraint)
{
	return g_Map.createObjectGroup(placer, player, constraint);
}

function getMapSize()
{
	return g_Map.size;
}

function getMapArea()
{
	return g_Map.size * g_Map.size;
}

function getNumPlayers()
{
	return g_MapSettings.PlayerData.length - 1;
}

function getCivCode(player)
{
	if (g_MapSettings.PlayerData[player+1].Civ)
		return g_MapSettings.PlayerData[player+1].Civ;

	warn("undefined civ specified for player " + (player + 1) + ", falling back to '" + FALLBACK_CIV + "'");
	return FALLBACK_CIV;
}

function areAllies(player1, player2)
{
	if (g_MapSettings.PlayerData[player1+1].Team === undefined ||
		g_MapSettings.PlayerData[player2+1].Team === undefined ||
		g_MapSettings.PlayerData[player2+1].Team == -1 ||
		g_MapSettings.PlayerData[player1+1].Team == -1)
		return false;

	return g_MapSettings.PlayerData[player1+1].Team === g_MapSettings.PlayerData[player2+1].Team;
}

function getPlayerTeam(player)
{
	if (g_MapSettings.PlayerData[player+1].Team === undefined)
		return -1;

	return g_MapSettings.PlayerData[player+1].Team;
}

/**
 * Sorts an array of player IDs by team index. Players without teams come first.
 * Randomize order for players of the same team.
 */
function sortPlayers(playerIndices)
{
	return shuffleArray(playerIndices).sort((p1, p2) => getPlayerTeam(p1 - 1) - getPlayerTeam(p2 - 1));
}

function primeSortPlayers(playerIndices)
{
	if (!playerIndices.length)
		return [];

	let prime = [];
	for (let i = 0; i < Math.ceil(playerIndices.length / 2); ++i)
	{
		prime.push(playerIndices[i]);
		prime.push(playerIndices[playerIndices.length - 1 - i]);
	}

	return prime;
}

function getStartingEntities(player)
{
	let civ = getCivCode(player);

	if (!g_CivData[civ] || !g_CivData[civ].StartEntities || !g_CivData[civ].StartEntities.length)
	{
		warn("Invalid or unimplemented civ '"+civ+"' specified, falling back to '" + FALLBACK_CIV + "'");
		civ = FALLBACK_CIV;
	}

	return g_CivData[civ].StartEntities;
}

function getHeight(x, z)
{
	return g_Map.getHeight(x, z);
}

function setHeight(x, z, height)
{
	g_Map.setHeight(x, z, height);
}


/**
 *	Utility functions for classes
 */

/**
 * Add point to given class by id
 */
function addToClass(x, z, id)
{
	let tileClass = getTileClass(id);

	if (tileClass !== null)
		tileClass.add(x, z);
}

/**
 * Remove point from the given class by id
 */
function removeFromClass(x, z, id)
{
	let tileClass = getTileClass(id);

	if (tileClass !== null)
		tileClass.remove(x, z);
}

/**
 * Create a painter for the given class
 */
function paintClass(id)
{
	return new TileClassPainter(getTileClass(id));
}

/**
 * Create a painter for the given class
 */
function unPaintClass(id)
{
	return new TileClassUnPainter(getTileClass(id));
}

/**
 * Create an avoid constraint for the given classes by the given distances
 */
function avoidClasses(/*class1, dist1, class2, dist2, etc*/)
{
	let ar = [];
	for (let i = 0; i < arguments.length/2; ++i)
		ar.push(new AvoidTileClassConstraint(arguments[2*i], arguments[2*i+1]));

	// Return single constraint
	if (ar.length == 1)
		return ar[0];

	return new AndConstraint(ar);
}

/**
 * Create a stay constraint for the given classes by the given distances
 */
function stayClasses(/*class1, dist1, class2, dist2, etc*/)
{
	let ar = [];
	for (let i = 0; i < arguments.length/2; ++i)
		ar.push(new StayInTileClassConstraint(arguments[2*i], arguments[2*i+1]));

	// Return single constraint
	if (ar.length == 1)
		return ar[0];

	return new AndConstraint(ar);
}

/**
 * Create a border constraint for the given classes by the given distances
 */
function borderClasses(/*class1, idist1, odist1, class2, idist2, odist2, etc*/)
{
	let ar = [];
	for (let i = 0; i < arguments.length/3; ++i)
		ar.push(new BorderTileClassConstraint(arguments[3*i], arguments[3*i+1], arguments[3*i+2]));

	// Return single constraint
	if (ar.length == 1)
		return ar[0];

	return new AndConstraint(ar);
}

/**
 * Checks if the given tile is in class "id"
 */
function checkIfInClass(x, z, id)
{
	let tileClass = getTileClass(id);
	if (tileClass === null)
		return 0;

	let members = tileClass.countMembersInRadius(x, z, 1);
	if (members === null)
		return 0;

	return members;
}

/**
 * Returns the distance between 2 points
 */
function getDistance(x1, z1, x2, z2)
{
	return Math.pow(Math.pow(x1 - x2, 2) + Math.pow(z1 - z2, 2), 1/2);
}

/**
 * Returns the angle of the vector between point 1 and point 2.
 * The angle is counterclockwise from the positive x axis.
 */
function getAngle(x1, z1, x2, z2)
{
	return Math.atan2(z2 - z1, x2 - x1);
}

/**
 * Returns the gradient of the line between point 1 and 2 in the form dz/dx
 */
function getGradient(x1, z1, x2, z2)
{
	if (x1 == x2 && z1 == z2)
		return 0;

	return (z1-z2)/(x1-x2);
}

function getTerrainTexture(x, y)
{
	return g_Map.getTexture(x, y);
}

/**
 * Returns the order to go through the points for the shortest closed path (array of indices)
 * @param {array} [points] - Points to be sorted of the form { "x": x_value, "y": y_value }
 */
function getOrderOfPointsForShortestClosePath(points)
{
	let order = [];
	let distances = [];
	if (points.length <= 3)
	{
		for (let i = 0; i < points.length; ++i)
			order.push(i);

		return order;
	}

	// Just add the first 3 points
	let pointsToAdd = deepcopy(points);
	for (let i = 0; i < 3; ++i)
	{
		order.push(i);
		pointsToAdd.shift(i);
		if (i)
			distances.push(getDistance(points[order[i]].x, points[order[i]].y, points[order[i - 1]].x, points[order[i - 1]].y));
	}

	distances.push(getDistance(
		points[order[0]].x,
		points[order[0]].y,
		points[order[order.length - 1]].x,
		points[order[order.length - 1]].y));

	// Add remaining points so the path lengthens the least
	let numPointsToAdd = pointsToAdd.length;
	for (let i = 0; i < numPointsToAdd; ++i)
	{
		let indexToAddTo;
		let minEnlengthen = Infinity;
		let minDist1 = 0;
		let minDist2 = 0;
		for (let k = 0; k < order.length; ++k)
		{
			let dist1 = getDistance(pointsToAdd[0].x, pointsToAdd[0].y, points[order[k]].x, points[order[k]].y);
			let dist2 = getDistance(pointsToAdd[0].x, pointsToAdd[0].y, points[order[(k + 1) % order.length]].x, points[order[(k + 1) % order.length]].y);
			let enlengthen = dist1 + dist2 - distances[k];
			if (enlengthen < minEnlengthen)
			{
				indexToAddTo = k;
				minEnlengthen = enlengthen;
				minDist1 = dist1;
				minDist2 = dist2;
			}
		}
		order.splice(indexToAddTo + 1, 0, i + 3);
		distances.splice(indexToAddTo, 1, minDist1, minDist2);
		pointsToAdd.shift();
	}

	return order;
}
