
/////////////////////////////////////////////////////////////////////////////////////////////
//	Constant definitions
/////////////////////////////////////////////////////////////////////////////////////////////

const PI = Math.PI;

const SEA_LEVEL = 20.0;

const TERRAIN_SEPARATOR = "|";

const TILES_PER_PATCH = 16;

/////////////////////////////////////////////////////////////////////////////////////////////
//	Utility functions
/////////////////////////////////////////////////////////////////////////////////////////////

function fractionToTiles(f) {
	return getMapSize() * f;
}

function tilesToFraction(t) {
	return t / getMapSize();
}

function fractionToSize(f) {
	return getMapSizeSqr() * f;
}

function sizeToFraction(s) {
	return s / getMapSizeSqr();
}

function cos(x) {
	return Math.cos(x);
}

function sin(x) {
	return Math.sin(x);
}

function tan(x) {
	return Math.tan(x);
}

function abs(x) {
	return Math.abs(x);
}

function round(x) {
	return Math.round(x);
}

function lerp(a, b, t) {
	return a + (b-a) * t;
}

function sqrt(x) {
	return Math.sqrt(x);
}

function ceil(x) {
	return Math.ceil(x);
}

function floor(x) {
	return Math.floor(x);
}

function max(x, y) {
	return x > y ? x : y;
}

function min(x, y) {
	return x < y ? x : y;
}

function println(x) {
	print(x);
	print("\n");
}

function argsToArray(x)
{
	var numArgs = x.length;
	if (numArgs != 1)
	{
		var ret = new Array(numArgs);
		for (var i=0; i < numArgs; i++)
		{
			ret[i] = x[i];
		}
		return ret;
	}
	else
	{
		return x[0];
	}
}

function chooseRand()
{
	if (arguments.length==0)
	{
		error("chooseRand: requires at least 1 argument");
	}
	var ar = argsToArray(arguments);
	return ar[randInt(ar.length)];
}

function createAreas(centeredPlacer, painter, constraint, num, retryFactor)
{
	if (retryFactor === undefined)
		retryFactor = 10;
	
	var maxFail = num * retryFactor;
	var good = 0;
	var bad = 0;
	var ret = [];
	while(good < num && bad <= maxFail)
	{
		centeredPlacer.x = randInt(getMapSize());
		centeredPlacer.y = randInt(getMapSize());
		var r = g_Map.createArea(centeredPlacer, painter, constraint);
		if (r !== undefined)
		{
			good++;
			ret.push(r);
		}
		else
		{
			bad++;
		}
	}
	
	return ret;
}

function createObjectGroups(placer, player, constraint, num, retryFactor)
{
	if (retryFactor === undefined)
		retryFactor = 10;
	
	var maxFail = num * retryFactor;
	var good = 0;
	var bad = 0;
	while(good < num && bad <= maxFail)
	{
		placer.x = randInt(getMapSize());
		placer.y = randInt(getMapSize());
		var r = createObjectGroup(placer, player, constraint);
		
		if (r !== undefined)
		{
			good++;
		}
		else
		{
			bad++;
		}
	}
	return good;
}

function createTerrain(terrain)
{
	if (terrain instanceof Array)
	{
		var terrainList = [];
		
		for (var i = 0; i < terrain.length; ++i)
			terrainList.push(createTerrain(terrain[i]));
		
		return new RandomTerrain(terrainList);
	}
	else
	{
		return createSimpleTerrain(terrain);
	}
}

function createSimpleTerrain(terrain)
{
	if (typeof(terrain) == "string")
	{	// Split string by pipe | character, this allows specifying terrain + tree type in single string
		var params = terrain.split(TERRAIN_SEPARATOR, 2);
		
		if (params.length != 2)
		{
			return new SimpleTerrain(terrain);
		}
		else
		{
			return new SimpleTerrain(params[0], params[1]);
		}
	}
	else
	{
		error("createSimpleTerrain expects string as input, received "+terrain);
		return undefined;
	}
}

function placeObject(type, player, x, y, angle)
{
	g_Map.addObjects(new Entity(type, player, x, y, angle));
}

function placeTerrain(x, y, terrain)
{
	// convert terrain param into terrain object
	g_Map.placeTerrain(x, y, createTerrain(terrain));
	
}

/////////////////////////////////////////////////////////////////////////////////////////////
//	Access global map variable
/////////////////////////////////////////////////////////////////////////////////////////////

function createTileClass()
{
	return g_Map.createTileClass();
}

function getTileClass(id)
{
	// Check for valid class id
	if (id < 1 || id > g_Map.tileClasses.length)
	{
		//error("Invalid tile class id: "+id);
		return null;
	}
	
	return g_Map.tileClasses[id - 1];
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

function getMapSizeSqr()
{
	return g_Map.size*g_Map.size;
}

function getNumPlayers()
{
	return g_MapSettings.PlayerData.length;
}

function getCivCode(player)
{
	return g_MapSettings.PlayerData[player].Civ;
}

function getHeight(x, y)
{
	g_Map.getHeight(x, y);
}

function setHeight(x, y, height)
{
	g_Map.setHeight(x, y, height);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//	Utility functions for classes
/////////////////////////////////////////////////////////////////////////////////////////////


// Add point to given class by id
function addToClass(x, y, id)
{
	var tileClass = getTileClass(id);
	
	if (tileClass !== null)
		tileClass.add(x, y);
}

// Create a painter for the given class
function paintClass(id)
{
	return new TileClassPainter(getTileClass(id));
}

// Create an avoid constraint for the given classes by the given distances
function avoidClasses(/*class1, dist1, class2, dist2, etc*/)
{
	var ar = new Array(arguments.length/2);
	for (var i=0; i < arguments.length/2; i++)
	{
		ar[i] = new AvoidTileClassConstraint(arguments[2*i], arguments[2*i+1]);
	}
	// Return single constraint
	return new AndConstraint(ar);
}

// Create a stay constraint for the given classes by the given distances
function stayClasses(/*class1, dist1, class2, dist2, etc*/)
{
	var ar = new Array(arguments.length/2);
	for (var i=0; i < arguments.length/2; i++)
	{
		ar[i] = new StayInTileClassConstraint(arguments[2*i], arguments[2*i+1]);
	}
	// Return single constraint
	return new AndConstraint(ar);
}
