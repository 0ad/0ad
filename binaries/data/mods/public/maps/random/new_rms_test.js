Engine.LoadLibrary("rmgen");

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

var [playerIDs, playerX, playerZ] = radialPlayerPlacement(0.39);

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
