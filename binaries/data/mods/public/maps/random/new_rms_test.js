RMS.LoadLibrary("rmgen");

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
var playerY = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var startAngle = randFloat() * 2 * PI;
for (var i=0; i < numPlayers; i++)
{
	playerAngle[i] = startAngle + i*2*PI/numPlayers;
	playerX[i] = 0.5 + 0.39*cos(playerAngle[i]);
	playerY[i] = 0.5 + 0.39*sin(playerAngle[i]);
}

for (var i=0; i < numPlayers; i++)
{
	log("Creating base for player " + (i + 1) + "...");
	
	// get the x and y in tiles
	var fx = fractionToTiles(playerX[i]);
	var fy = fractionToTiles(playerY[i]);
	var ix = round(fx);
	var iy = round(fy);

	// create the TC and citizens
	var civ = getCivCode(i);
	var group = new SimpleGroup(
		[							// elements (type, count, distance)
			new SimpleObject("structures/"+civ+"_civil_centre", 1,1, 0,0),
			new SimpleObject("units/"+civ+"_support_female_citizen", 3,3, 5,5)
		],
		true, null,	ix, iy
	);
	createObjectGroup(group, i+1);
}

// Export map data
ExportMap();
