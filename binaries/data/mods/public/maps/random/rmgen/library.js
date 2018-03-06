/**
 * A Centered Placer generates a shape (array of Vector2D points) around a variable center location satisfying a Constraint.
 * The center can be modified externally using setCenterPosition, typically called by createAreas.
 */
Engine.LoadLibrary("rmgen/placer/centered");

/**
 * A Non-Centered Placer generates a shape (array of Vector2D points) at a fixed location meeting a Constraint and
 * is typically called by createArea.
 * Since this type of Placer has no x and z property, its location cannot be randomized using createAreas.
 */
Engine.LoadLibrary("rmgen/placer/noncentered");

/**
 * A Painter modifies an arbitrary feature in a given Area, for instance terrain textures, elevation or calling other painters on that Area.
 * Typically the area is determined by a Placer called from createArea or createAreas.
 */
Engine.LoadLibrary("rmgen/painter");

const TERRAIN_SEPARATOR = "|";
const SEA_LEVEL = 20.0;
const HEIGHT_UNITS_PER_METRE = 92;

/**
 * Number of impassable, unexplorable tiles at the map border.
 */
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

const g_ActorPrefix = "actor|";

/**
 * Sets whether setHeight operates on the center of a tile or on the vertices.
 */
var TILE_CENTERED_HEIGHT_MAP = false;

function actorTemplate(templateName)
{
	return g_ActorPrefix + templateName + ".xml";
}

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

function randomPositionOnTile(tilePosition)
{
	return Vector2D.add(tilePosition, new Vector2D(randFloat(0, 1), randFloat(0, 1)));
}

/**
 * Retries the given function with those arguments as often as specified.
 */
function retryPlacing(placeFunc, retryFactor, amount, behaveDeprecated = false)
{
	let maxFail = amount * retryFactor;

	let results = [];
	let bad = 0;

	while (results.length < amount && bad <= maxFail)
	{
		let result = placeFunc();

		if (result !== undefined || behaveDeprecated)
			results.push(result);
		else
			++bad;
	}

	return results;
}

// TODO this is a hack to simulate the old behaviour of those functions
// until all old maps are changed to use the correct version of these functions
function createObjectGroupsDeprecated(group, player, constraints, amount, retryFactor = 10)
{
	return createObjectGroups(group, player, constraints, amount, retryFactor, true);
}

function createObjectGroupsByAreasDeprecated(group, player, constraints, amount, retryFactor, areas)
{
	return createObjectGroupsByAreas(group, player, constraints, amount, retryFactor, areas, true);
}

/**
 * Attempts to place the given number of areas in random places of the map.
 * Returns actually placed areas.
 */
function createAreas(centeredPlacer, painter, constraints, amount, retryFactor = 10)
{
	let placeFunc = function() {
		centeredPlacer.setCenterPosition(g_Map.randomCoordinate(false));
		return createArea(centeredPlacer, painter, constraints);
	};

	return retryPlacing(placeFunc, retryFactor, amount, false);
}

/**
 * Attempts to place the given number of areas in random places of the given areas.
 * Returns actually placed areas.
 */
function createAreasInAreas(centeredPlacer, painter, constraints, amount, retryFactor, areas)
{
	let placeFunc = function() {
		centeredPlacer.setCenterPosition(pickRandom(pickRandom(areas).getPoints()));
		return createArea(centeredPlacer, painter, constraints);
	};

	return retryPlacing(placeFunc, retryFactor, amount, false);
}

/**
 * Attempts to place the given number of groups in random places of the map.
 * Returns the number of actually placed groups.
 */
function createObjectGroups(group, player, constraints, amount, retryFactor = 10, behaveDeprecated = false)
{
	let placeFunc = function() {
		group.setCenterPosition(g_Map.randomCoordinate(true));
		return createObjectGroup(group, player, constraints);
	};

	return retryPlacing(placeFunc, retryFactor, amount, behaveDeprecated);
}

/**
 * Attempts to place the given number of groups in random places of the given areas.
 * Returns the number of actually placed groups.
 */
function createObjectGroupsByAreas(group, player, constraints, amount, retryFactor, areas, behaveDeprecated = false)
{
	let placeFunc = function() {
		group.setCenterPosition(pickRandom(pickRandom(areas).getPoints()));
		return createObjectGroup(group, player, constraints);
	};

	return retryPlacing(placeFunc, retryFactor, amount, behaveDeprecated);
}

function createTerrain(terrain)
{
	return typeof terrain == "string" ?
		new SimpleTerrain(...terrain.split(TERRAIN_SEPARATOR)) :
		new RandomTerrain(terrain.map(t => createTerrain(t)));
}

/**
 * Constructs a new Area shaped by the Placer meeting the Constraints and calls the Painters there.
 * Supports both Centered and Non-Centered Placers.
 */
function createArea(placer, painters, constraints)
{
	let points = placer.place(new AndConstraint(constraints));
	if (!points)
		return undefined;

	let area = new Area(points);

	new MultiPainter(painters).paint(area);

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
		new TileClassPainter(tileClass));
}

function unPaintTileClassBasedOnHeight(minHeight, maxHeight, mode, tileClass)
{
	createArea(
		new HeightPlacer(mode, minHeight, maxHeight),
		new TileClassUnPainter(tileClass));
}

/**
 * Places the Entities of the given Group if they meet the Constraints
 * and sets the given player as the owner.
 */
function createObjectGroup(group, player, constraints)
{
	return group.place(player, new AndConstraint(constraints));
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
 * Returns a subset of the given heightmap.
 */
function extractHeightmap(heightmap, topLeft, size)
{
	let result = [];
	for (let x = 0; x < size; ++x)
	{
		result[x] = new Float32Array(size);
		for (let y = 0; y < size; ++y)
			result[x][y] = heightmap[x + topLeft.x][y + topLeft.y];
	}
	return result;
}

function convertHeightmap1Dto2D(heightmap)
{
	let result = [];
	let hmSize = Math.sqrt(heightmap.length);
	for (let x = 0; x < hmSize; ++x)
	{
		result[x] = new Float32Array(hmSize);
		for (let y = 0; y < hmSize; ++y)
			result[x][y] = heightmap[y * hmSize + x];
	}
	return result;
}
