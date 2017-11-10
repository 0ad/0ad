const PI = Math.PI;
const TWO_PI = 2 * Math.PI;
const TERRAIN_SEPARATOR = "|";
const SEA_LEVEL = 20.0;
const HEIGHT_UNITS_PER_METRE = 92;
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

function scaleByMapSize(min, max, minMapSize = 128, maxMapSize = 512)
{
	return min + (max - min) * (g_Map.size - minMapSize) / (maxMapSize - minMapSize);
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
function retryPlacing(placeFunc, placeArgs, retryFactor, amount, getResult, behaveDeprecated = false)
{
	if (behaveDeprecated && !(placeArgs.placer instanceof SimpleGroup || placeArgs.placer instanceof RandomGroup))
		warn("Deprecated version of createFoo should only be used for SimpleGroup and RandomGroup placers!");

	let maxFail = amount * retryFactor;

	let results = [];
	let good = 0;
	let bad = 0;

	while (good < amount && bad <= maxFail)
	{
		let result = placeFunc(placeArgs);

		if (result !== undefined || behaveDeprecated)
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

// TODO this is a hack to simulate the old behaviour of those functions
// until all old maps are changed to use the correct version of these functions
function createObjectGroupsDeprecated(placer, player, constraint, amount, retryFactor = 10)
{
	return createObjectGroups(placer, player, constraint, amount, retryFactor, true);
}

function createObjectGroupsByAreasDeprecated(placer, player, constraint, amount, retryFactor, areas)
{
	return createObjectGroupsByAreas(placer, player, constraint, amount, retryFactor, areas, true);
}

/**
 * Attempts to place the given number of areas in random places of the map.
 * Returns actually placed areas.
 */
function createAreas(centeredPlacer, painter, constraint, amount, retryFactor = 10, behaveDeprecated = false)
{
	let placeFunc = function (args) {
		randomizePlacerCoordinates(args.placer, args.halfMapSize);
		return createArea(args.placer, args.painter, args.constraint);
	};

	let args = {
		"placer": centeredPlacer,
		"painter": painter,
		"constraint": constraint,
		"halfMapSize": g_Map.size / 2
	};

	return retryPlacing(placeFunc, args, retryFactor, amount, true, behaveDeprecated);
}

/**
 * Attempts to place the given number of areas in random places of the given areas.
 * Returns actually placed areas.
 */
function createAreasInAreas(centeredPlacer, painter, constraint, amount, retryFactor, areas, behaveDeprecated = false)
{
	if (!areas.length)
		return [];

	let placeFunc = function (args) {
		randomizePlacerCoordinatesFromAreas(args.placer, args.areas);
		return createArea(args.placer, args.painter, args.constraint);
	};

	let args = {
		"placer": centeredPlacer,
		"painter": painter,
		"constraint": constraint,
		"areas": areas,
		"halfMapSize": g_Map.size / 2
	};

	return retryPlacing(placeFunc, args, retryFactor, amount, true, behaveDeprecated);
}

/**
 * Attempts to place the given number of groups in random places of the map.
 * Returns the number of actually placed groups.
 */
function createObjectGroups(placer, player, constraint, amount, retryFactor = 10, behaveDeprecated = false)
{
	let placeFunc = function (args) {
		randomizePlacerCoordinates(args.placer, args.halfMapSize);
		return createObjectGroup(args.placer, args.player, args.constraint);
	};

	let args = {
		"placer": placer,
		"player": player,
		"constraint": constraint,
		"halfMapSize": getMapSize() / 2 - MAP_BORDER_WIDTH
	};

	return retryPlacing(placeFunc, args, retryFactor, amount, false, behaveDeprecated);
}

/**
 * Attempts to place the given number of groups in random places of the given areas.
 * Returns the number of actually placed groups.
 */
function createObjectGroupsByAreas(placer, player, constraint, amount, retryFactor, areas, behaveDeprecated = false)
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

	return retryPlacing(placeFunc, args, retryFactor, amount, false, behaveDeprecated);
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
		throw new Error("createSimpleTerrain expects string as input, received " + uneval(terrain));

	// Split string by pipe | character, this allows specifying terrain + tree type in single string
	let params = terrain.split(TERRAIN_SEPARATOR, 2);

	if (params.length != 2)
		return new SimpleTerrain(terrain);

	return new SimpleTerrain(params[0], params[1]);
}

function placeObject(x, z, type, player, angle)
{
	if (g_Map.validT(x, z))
		g_Map.addObject(new Entity(type, player, x, z, angle));
}

function placeTerrain(x, z, terrainNames)
{
	createTerrain(terrainNames).place(x, z);
}

function initTerrain(terrainNames)
{
	let terrain = createTerrain(terrainNames);

	for (let x = 0; x < getMapSize(); ++x)
		for (let z = 0; z < getMapSize(); ++z)
			terrain.place(x, z);
}

function isCircularMap()
{
	return !!g_MapSettings.CircularMap;
}

function getMapBaseHeight()
{
	return g_MapSettings.BaseHeight;
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

/**
 * Constructs a new Area shaped by the Placer meeting the Constraint and calls the Painters there.
 * Supports both Centered and Non-Centered Placers.
 */
function createArea(placer, painter, constraint)
{
	if (!constraint)
		constraint = new NullConstraint();
	else if (constraint instanceof Array)
		constraint = new AndConstraint(constraint);

	let points = placer.place(constraint);
	if (!points)
		return undefined;

	let area = g_Map.createArea(points);

	if (painter instanceof Array)
		painter = new MultiPainter(painter);

	painter.paint(area);

	return area;
}

/**
 * @param mode is one of the HeightPlacer constants determining whether to exclude the min/max elevation.
 */
function paintTerrainBasedOnHeight(minHeight, maxHeight, mode, terrain)
{
	createArea(
		new HeightPlacer(mode, minHeight, maxHeight),
		new TerrainPainter(terrain));
}

function paintTileClassBasedOnHeight(minHeight, maxHeight, mode, tileClass)
{
	createArea(
		new HeightPlacer(mode, minHeight, maxHeight),
		new TileClassPainter(getTileClass(tileClass)));
}

function unPaintTileClassBasedOnHeight(minHeight, maxHeight, mode, tileClass)
{
	createArea(
		new HeightPlacer(mode, minHeight, maxHeight),
		new TileClassUnPainter(getTileClass(tileClass)));
}

/**
 * Places the Entities of the given Group if they meet the Constraint
 * and sets the given player as the owner.
 */
function createObjectGroup(group, player, constraint)
{
	if (!constraint)
		constraint = new NullConstraint();
	else if (constraint instanceof Array)
		constraint = new AndConstraint(constraint);

	return group.place(player, constraint);
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

function getHeight(x, z)
{
	return g_Map.getHeight(x, z);
}

function setHeight(x, z, height)
{
	g_Map.setHeight(x, z, height);
}

function initHeight(height)
{
	g_Map.initHeight(height);
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

function getTerrainTexture(x, y)
{
	return g_Map.getTexture(x, y);
}
