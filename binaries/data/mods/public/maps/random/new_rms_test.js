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

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = shuffleArray(playerIDs);

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
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// get the x and z in tiles
	var fx = fractionToTiles(playerX[i]);
	var fz = fractionToTiles(playerZ[i]);
	var ix = round(fx);
	var iz = round(fz);

	// get civ specific starting entities
	var civEntities = getStartingEntities(id-1);
	
	// create the TC
	var group = new SimpleGroup(	// elements (type, min/max count, min/max distance, min/max angle)
		[new SimpleObject(civEntities[0].Template, 1,1, 0,0, BUILDING_ANGlE, BUILDING_ANGlE)],
		true, null, ix, iz
	);
	createObjectGroup(group, id);
	
	// create starting units
	var uDist = 6;
	var uSpace = 2;
	for (var j = 1; j < civEntities.length; ++j)
	{
		var uAngle = -BUILDING_ANGlE + PI * (j - 1) / 2;
		var count = (civEntities[j].Count !== undefined ? civEntities[j].Count : 1);
		for (var numberofentities = 0; numberofentities < count; numberofentities++)
		{
			var ux = fx + uDist * cos(uAngle) + numberofentities * uSpace * cos(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * cos(uAngle + PI/2));
			var uz = fz + uDist * sin(uAngle) + numberofentities * uSpace * sin(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * sin(uAngle + PI/2));
			placeObject(ux, uz, civEntities[j].Template, id, (j % 2 - 1) * PI + uAngle);
		}
	}
}

// Export map data
ExportMap();
