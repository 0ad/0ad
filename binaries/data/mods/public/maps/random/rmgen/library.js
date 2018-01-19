const TERRAIN_SEPARATOR = "|";
const SEA_LEVEL = 20.0;
const HEIGHT_UNITS_PER_METRE = 92;
const MAP_BORDER_WIDTH = 3;

const g_DamageTypes = new DamageTypes();

/**
 * Constants needed for heightmap_manipulation.js
 */
const MAX_HEIGHT_RANGE = 0xFFFF / HEIGHT_UNITS_PER_METRE; // Engine limit, Roughly 700 meters
const MIN_HEIGHT = - SEA_LEVEL;

/**
 * Length of one tile of the terrain grid in metres.
 * Useful to transform footprint sizes of templates to the coordinate system used by getMapSize.
 */
const TERRAIN_TILE_SIZE = Engine.GetTerrainTileSize();

const MAX_HEIGHT = MAX_HEIGHT_RANGE - SEA_LEVEL;

/**
 * Default angle for buildings.
 */
const BUILDING_ORIENTATION = -1/4 * Math.PI;

const g_CivData = deepfreeze(loadCivFiles(false));

function fractionToTiles(f)
{
	return g_MapSettings.Size * f;
}

function tilesToFraction(t)
{
	return t / g_MapSettings.Size;
}

function scaleByMapSize(min, max, minMapSize = 128, maxMapSize = 512)
{
	return min + (max - min) * (g_MapSettings.Size - minMapSize) / (maxMapSize - minMapSize);
}

/**
 * Retries the given function with those arguments as often as specified.
 */
function retryPlacing(placeFunc, retryFactor, amount, getResult, behaveDeprecated = false)
{
	let maxFail = amount * retryFactor;

	let results = [];
	let good = 0;
	let bad = 0;

	while (good < amount && bad <= maxFail)
	{
		let result = placeFunc();

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
 * Sets the x and z property of the given object (typically a Placer or Group) to a random point on the map.
 * @param passableOnly - Should be true for entity placement and false for terrain or elevation operations.
 */
function randomizeCoordinates(obj, passableOnly)
{
	let border = passableOnly ? MAP_BORDER_WIDTH : 0;
	if (g_MapSettings.CircularMap)
	{
		// Polar coordinates
		// Uniformly distributed on the disk
		let halfMapSize = g_Map.size / 2 - border;
		let r = halfMapSize * Math.sqrt(randFloat(0, 1));
		let theta = randomAngle();
		obj.x = Math.floor(r * Math.cos(theta)) + halfMapSize;
		obj.z = Math.floor(r * Math.sin(theta)) + halfMapSize;
	}
	else
	{
		// Rectangular coordinates
		obj.x = randIntExclusive(border, g_Map.size - border);
		obj.z = randIntExclusive(border, g_Map.size - border);
	}
}

/**
 * Sets the x and z property of the given JS object (typically a Placer or Group) to a random point of the area.
 */
function randomizeCoordinatesFromAreas(obj, areas)
{
	let pt = pickRandom(pickRandom(areas).points);
	obj.x = pt.x;
	obj.z = pt.z;
}

// TODO this is a hack to simulate the old behaviour of those functions
// until all old maps are changed to use the correct version of these functions
function createObjectGroupsDeprecated(group, player, constraint, amount, retryFactor = 10)
{
	return createObjectGroups(group, player, constraint, amount, retryFactor, true);
}

function createObjectGroupsByAreasDeprecated(group, player, constraint, amount, retryFactor, areas)
{
	return createObjectGroupsByAreas(group, player, constraint, amount, retryFactor, areas, true);
}

/**
 * Attempts to place the given number of areas in random places of the map.
 * Returns actually placed areas.
 */
function createAreas(centeredPlacer, painter, constraint, amount, retryFactor = 10)
{
	let placeFunc = function() {
		randomizeCoordinates(centeredPlacer, false);
		return createArea(centeredPlacer, painter, constraint);
	};

	return retryPlacing(placeFunc, retryFactor, amount, true, false);
}

/**
 * Attempts to place the given number of areas in random places of the given areas.
 * Returns actually placed areas.
 */
function createAreasInAreas(centeredPlacer, painter, constraint, amount, retryFactor, areas)
{
	let placeFunc = function() {
		randomizeCoordinatesFromAreas(centeredPlacer, areas);
		return createArea(centeredPlacer, painter, constraint);
	};

	return retryPlacing(placeFunc, retryFactor, amount, true, false);
}

/**
 * Attempts to place the given number of groups in random places of the map.
 * Returns the number of actually placed groups.
 */
function createObjectGroups(group, player, constraint, amount, retryFactor = 10, behaveDeprecated = false)
{
	let placeFunc = function() {
		randomizeCoordinates(group, true);
		return createObjectGroup(group, player, constraint);
	};

	return retryPlacing(placeFunc, retryFactor, amount, false, behaveDeprecated);
}

/**
 * Attempts to place the given number of groups in random places of the given areas.
 * Returns the number of actually placed groups.
 */
function createObjectGroupsByAreas(group, player, constraint, amount, retryFactor, areas, behaveDeprecated = false)
{
	let placeFunc = function() {
		randomizeCoordinatesFromAreas(group, areas);
		return createObjectGroup(group, player, constraint);
	};

	return retryPlacing(placeFunc, retryFactor, amount, false, behaveDeprecated);
}

function createTerrain(terrain)
{
	return typeof terrain == "string" ?
		new SimpleTerrain(...terrain.split(TERRAIN_SEPARATOR)) :
		new RandomTerrain(terrain.map(t => createTerrain(t)));
}

function placeObject(x, z, type, player, angle)
{
	if (g_Map.validT(x, z))
		g_Map.addObject(new Entity(type, player, x, z, angle));
}

function isCircularMap()
{
	return !!g_MapSettings.CircularMap;
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
 * Constructs a new Area shaped by the Placer meeting the Constraints and calls the Painters there.
 * Supports both Centered and Non-Centered Placers.
 */
function createArea(placer, painter, constraints)
{
	let points = placer.place(new AndConstraint(constraints));
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
 * Places the Entities of the given Group if they meet the Constraints
 * and sets the given player as the owner.
 */
function createObjectGroup(group, player, constraints)
{
	return group.place(player, new AndConstraint(constraints));
}

function getMapSize()
{
	return g_Map.size;
}

function getMapCenter()
{
	return deepfreeze(new Vector2D(g_Map.size / 2, g_Map.size / 2));
}

function getMapBounds()
{
	return deepfreeze({
		"left": fractionToTiles(0),
		"right": fractionToTiles(1),
		"top": fractionToTiles(1),
		"bottom": fractionToTiles(0)
	});
}

function isNomad()
{
	return !!g_MapSettings.Nomad;
}

function getNumPlayers()
{
	return g_MapSettings.PlayerData.length - 1;
}

function getCivCode(playerID)
{
	return g_MapSettings.PlayerData[playerID].Civ;
}

function areAllies(playerID1, playerID2)
{
	return (
		g_MapSettings.PlayerData[playerID1].Team !== undefined &&
		g_MapSettings.PlayerData[playerID2].Team !== undefined &&
		g_MapSettings.PlayerData[playerID1].Team != -1 &&
		g_MapSettings.PlayerData[playerID2].Team != -1 &&
		g_MapSettings.PlayerData[playerID1].Team === g_MapSettings.PlayerData[playerID2].Team);
}

function getPlayerTeam(playerID)
{
	if (g_MapSettings.PlayerData[playerID].Team === undefined)
		return -1;

	return g_MapSettings.PlayerData[playerID].Team;
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
