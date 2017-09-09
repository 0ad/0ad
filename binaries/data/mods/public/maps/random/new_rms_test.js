RMS.LoadLibrary("rmgen");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();

var clPlayer = createTileClass();
var clPath = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clRock = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

var playerIDs = sortAllPlayers();

var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var startAngle = randFloat(0, 2 * PI);
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

	placeCivDefaultEntities(fx, fz, id);
}

ExportMap();
