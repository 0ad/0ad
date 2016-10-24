var TILE_CENTERED_HEIGHT_MAP = false;
var WATER_LEVEL_CHANGED = false;

var g_Map;

var g_Camera = {
	"Position": { "x": 100, "y": 150, "z": -100 },
	"Rotation": 0,
	"Declination": 0.523599
};

var g_CivData = {};

function InitMap()
{
	// Should never get this far, failed settings would abort prior to loading scripts
	if (g_MapSettings === undefined)
		throw("InitMapGen: settings missing");

	// Get civ data as array of JSON strings
	var data = RMS.GetCivData();
	if (!data || !data.length)
		throw("InitMapGen: error reading civ data");

	for (var i = 0; i < data.length; ++i)
	{
		var civData = JSON.parse(data[i]);
		g_CivData[civData.Code] = civData;
	}

	log("Creating new map...");
	var terrain = createTerrain(g_MapSettings.BaseTerrain);

	g_Map = new Map(g_MapSettings.Size, g_MapSettings.BaseHeight);
	g_Map.initTerrain(terrain);
}

function ExportMap()
{
	log("Saving map...");

	var data = g_Map.getMapData();

	// Add environment and camera settings
	if (!WATER_LEVEL_CHANGED)
		g_Environment.Water.WaterBody.Height = SEA_LEVEL - 0.1;

	data.Environment = g_Environment;

	// Adjust default cam to roughly center of the map - useful for Atlas
	g_Camera.Position = {
		"x":  g_MapSettings.Size * 2,
		"y":  g_MapSettings.Size * 2,
		"z": -g_MapSettings.Size * 2
	};

	data.Camera = g_Camera;

	RMS.ExportMap(data);
}
