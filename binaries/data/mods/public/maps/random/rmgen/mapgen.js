var TILE_CENTERED_HEIGHT_MAP = false;
var WATER_LEVEL_CHANGED = false;

var g_Map;

/**
 * Camera location in case there are no player entities.
 */
var g_Camera = {
	"Position": { "x": 256, "y": 150, "z": 256 },
	"Rotation": 0,
	"Declination": 0.523599
};

function InitMap(baseHeight, baseTerrain)
{
	g_Map = new RandomMap(g_MapSettings.Size, baseHeight);

	createArea(
		new MapBoundsPlacer(),
		new TerrainPainter(baseTerrain));
}

function ExportMap()
{
	log("Saving map...");

	if (!WATER_LEVEL_CHANGED)
		g_Environment.Water.WaterBody.Height = SEA_LEVEL - 0.1;

	Engine.ExportMap({
		"entities": g_Map.exportEntityList(),
		"height": g_Map.exportHeightData(),
		"seaLevel": SEA_LEVEL,
		"size": g_Map.size,
		"textureNames": g_Map.IDToName,
		"tileData": g_Map.exportTerrainTextures(),
		"Camera": g_Camera,
		"Environment": g_Environment
	});
}
