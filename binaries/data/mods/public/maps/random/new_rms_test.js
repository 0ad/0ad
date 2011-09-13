RMS.LoadLibrary("rmgen");

const BUILDING_ANGlE = 0.75*PI;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();

// create tile classes

var clPlayer = createTileClass();
var clPath = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clRock = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

// place players

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var startAngle = randFloat() * 2 * PI;
for (var i=0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*2*PI/numPlayers;
	playerX[i] = 0.5 + 0.39*cos(playerAngle[i]);
	playerZ[i] = 0.5 + 0.39*sin(playerAngle[i]);
}

for (var i=0; i < numPlayers; i++)
{
	log("Creating base for player " + (i + 1) + "...");
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	// get civ specific starting entities
	var civEntities = getStartingEntities(i);
	
	// create the TC
	var group = new SimpleGroup(	// elements (type, min/max count, min/max distance, min/max angle)
		[new SimpleObject(civEntities[0].Template, 1,1, 0,0, BUILDING_ANGlE, BUILDING_ANGlE)],
		true, null, ix, iz
	);
	createObjectGroup(group, i+1);
	
	// create starting units
	var uDist = 8;
	var uAngle = -BUILDING_ANGlE + randFloat(-PI/8, PI/8);
	for (var j = 1; j < civEntities.length; ++j)
	{
		var count = (civEntities[j].Count !== undefined ? civEntities[j].Count : 1);
		var ux = round(fx + uDist * cos(uAngle));
		var uz = round(fz + uDist * sin(uAngle));
		group = new SimpleGroup(	// elements (type, min/max count, min/max distance)
			[new SimpleObject(civEntities[j].Template, count,count, 1,ceil(count/2))],
			true, null, ux, uz
		);
		createObjectGroup(group, i+1);
		uAngle += PI/4;
	}
}

// Export map data
ExportMap();
