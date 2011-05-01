
/////////////////////////////////////////////////////////////////////////////////////////////
//	Constant definitions
/////////////////////////////////////////////////////////////////////////////////////////////

const PI = Math.PI;
const TWO_PI = 2 * Math.PI;
const TERRAIN_SEPARATOR = "|";
const SEA_LEVEL = 20.0;
const CELL_SIZE = 4;
const HEIGHT_UNITS_PER_METRE = 732;
const MIN_MAP_SIZE = 128;
const MAX_MAP_SIZE = 512;

/////////////////////////////////////////////////////////////////////////////////////////////
//	Utility functions
/////////////////////////////////////////////////////////////////////////////////////////////

function fractionToTiles(f)
{
	return getMapSize() * f;
}

function tilesToFraction(t)
{
	return t / getMapSize();
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
	return min + ((max-min) * (getMapSize()-MIN_MAP_SIZE) / (MAX_MAP_SIZE-MIN_MAP_SIZE));
}

function cos(x)
{
	return Math.cos(x);
}

function sin(x)
{
	return Math.sin(x);
}

function tan(x)
{
	return Math.tan(x);
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

function println(x)
{
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
		throw("chooseRand: requires at least 1 argument");
	}
	var ar = argsToArray(arguments);
	return ar[randInt(ar.length)];
}

function createAreas(centeredPlacer, painter, constraint, num, retryFactor)
{
	if (retryFactor === undefined)
	{
		retryFactor = 10;
	}
	
	var maxFail = num * retryFactor;
	var good = 0;
	var bad = 0;
	var result = [];
	var halfSize = getMapSize()/2;
	
	while(good < num && bad <= maxFail)
	{
		if (isCircularMap())
		{	// Polar coordinates
			var r = halfSize * Math.sqrt(randFloat());	// uniform distribution
			var theta = randFloat(0, 2 * PI);
			centeredPlacer.x = Math.floor(r * Math.cos(theta)) + halfSize;
			centeredPlacer.z = Math.floor(r * Math.sin(theta)) + halfSize;
		}
		else
		{	// Rectangular coordinates
			centeredPlacer.x = randInt(getMapSize());
			centeredPlacer.z = randInt(getMapSize());
		}
		
		var area = g_Map.createArea(centeredPlacer, painter, constraint);
		if (area !== undefined)
		{
			good++;
			result.push(area);
		}
		else
		{
			bad++;
		}
	}
	return result;
}

function createAreasInAreas(centeredPlacer, painter, constraint, num, retryFactor, areas)
{
	if (retryFactor === undefined)
	{
		retryFactor = 10;
	}
	
	var maxFail = num * retryFactor;
	var good = 0;
	var bad = 0;
	var result = [];
	var numAreas = areas.length;
	
	while(good < num && bad <= maxFail && numAreas)
	{
		// Choose random point from area
		var i = randInt(numAreas);
		var size = areas[i].points.length;
		var pt = areas[i].points[randInt(size)];
		centeredPlacer.x = pt.x;
		centeredPlacer.z = pt.z;
		
		var area = g_Map.createArea(centeredPlacer, painter, constraint);
		if (area !== undefined)
		{
			good++;
			result.push(area);
		}
		else
		{
			bad++;
		}
	}
	return result;
}

function createObjectGroups(placer, player, constraint, num, retryFactor)
{
	if (retryFactor === undefined)
	{
		retryFactor = 10;
	}
	
	var maxFail = num * retryFactor;
	var good = 0;
	var bad = 0;
	var halfSize = getMapSize()/2;
	while(good < num && bad <= maxFail)
	{
		if (isCircularMap())
		{	// Polar coordinates
			var r = halfSize * Math.sqrt(randFloat());	// uniform distribution
			var theta = randFloat(0, 2 * PI);
			placer.x = Math.floor(r * Math.cos(theta)) + halfSize;
			placer.z = Math.floor(r * Math.sin(theta)) + halfSize;
		}
		else
		{	// Rectangular coordinates
			placer.x = randInt(getMapSize());
			placer.z = randInt(getMapSize());
		}
		
		var result = createObjectGroup(placer, player, constraint);
		if (result !== undefined)
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

function createObjectGroupsByAreas(placer, player, constraint, num, retryFactor, areas)
{
	if (retryFactor === undefined)
	{
		retryFactor = 10;
	}
	
	var maxFail = num * retryFactor;
	var good = 0;
	var bad = 0;
	var numAreas = areas.length;
	
	while(good < num && bad <= maxFail && numAreas)
	{
		// Choose random point from area
		var i = randInt(numAreas);
		var size = areas[i].points.length;
		var pt = areas[i].points[randInt(size)];
		placer.x = pt.x;
		placer.z = pt.z;
		
		var result = createObjectGroup(placer, player, constraint);
		if (result !== undefined)
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
		{
			terrainList.push(createTerrain(terrain[i]));
		}
		
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
		throw("createSimpleTerrain expects string as input, received "+terrain);
	}
}

function placeObject(x, z, type, player, angle)
{
	g_Map.addObject(new Entity(type, player, x, z, angle));
}

function placeTerrain(x, z, terrain)
{
	// convert terrain param into terrain object
	g_Map.placeTerrain(x, z, createTerrain(terrain));
	
}

function isCircularMap()
{
	return (g_MapSettings.CircularMap ? true : false);
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
	if (!g_Map.validClass(id))
	{
		return undefined;
	}
	
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

function getHeight(x, z)
{
	return g_Map.getHeight(x, z);
}

function setHeight(x, z, height)
{
	g_Map.setHeight(x, z, height);
}

/////////////////////////////////////////////////////////////////////////////////////////////
//	Utility functions for classes
/////////////////////////////////////////////////////////////////////////////////////////////


// Add point to given class by id
function addToClass(x, z, id)
{
	var tileClass = getTileClass(id);
	
	if (tileClass !== null)
	{
		tileClass.add(x, z);
	}
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
	for (var i = 0; i < arguments.length/2; i++)
	{
		ar[i] = new AvoidTileClassConstraint(arguments[2*i], arguments[2*i+1]);
	}
	
	// Return single constraint
	if (ar.length == 1)
	{
		return ar[0];
	}
	else
	{
		return new AndConstraint(ar);
	}
}

// Create a stay constraint for the given classes by the given distances
function stayClasses(/*class1, dist1, class2, dist2, etc*/)
{
	var ar = new Array(arguments.length/2);
	for (var i = 0; i < arguments.length/2; i++)
	{
		ar[i] = new StayInTileClassConstraint(arguments[2*i], arguments[2*i+1]);
	}
	
	// Return single constraint
	if (ar.length == 1)
	{
		return ar[0];
	}
	else
	{
		return new AndConstraint(ar);
	}
}

// Create a border constraint for the given classes by the given distances
function borderClasses(/*class1, idist1, odist1, class2, idist2, odist2, etc*/)
{
	var ar = new Array(arguments.length/3);
	for (var i = 0; i < arguments.length/3; i++)
	{
		ar[i] = new BorderTileClassConstraint(arguments[3*i], arguments[3*i+1], arguments[3*i+2]);
	}
	
	// Return single constraint
	if (ar.length == 1)
	{
		return ar[0];
	}
	else
	{
		return new AndConstraint(ar);
	}
}
