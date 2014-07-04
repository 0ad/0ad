var TILE_CENTERED_HEIGHT_MAP = false;
var WATER_LEVEL_CHANGED = false;

var g_Map;

var g_Environment = {
	SkySet: "default",
	SunColour: {r: 0.749020, g: 0.749020, b: 0.749020, a: 0},
	SunElevation: 0.785398,
	SunRotation: 5.49779,
	TerrainAmbientColour: {r: 0.501961, g: 0.501961, b: 0.501961, a: 0},
	UnitsAmbientColour: {r: 0.501961, g: 0.501961, b: 0.501961, a: 0},
	Water: {
		WaterBody: {
			Type: "ocean",
			Colour: {r: 0.3, g: 0.35, b: 0.7, a: 0},
			Tint: {r: 0.28, g: 0.3, b: 0.59, a: 0},
			Height: 5,
			Waviness: 8,
			Murkiness: 0.45,
			WindAngle: 0.0
		}
	},
	Fog: {
		FogFactor: 0.0,
		FogThickness: 0.5,
		FogColor: {r: 0.8, g: 0.8, b: 0.8, a: 0}
	},
	Postproc: {
		Brightness: 0.0,
		Contrast: 1.0,
		Saturation: 1.0,
		Bloom: 0.2,
		PostprocEffect: "default"
	}
};

var g_Camera = {
	Position: {x: 100, y: 150, z: -100},
	Rotation: 0,
	Declination: 0.523599
};

var g_CivData = {};

/////////////////////////////////////////////////////////////////////////////////////

function InitMap()
{
	if (g_MapSettings === undefined)
	{
		// Should never get this far, failed settings would abort prior to loading scripts
		throw("InitMapGen: settings missing");
	}
	
	// Get civ data as array of JSON strings
	var data = RMS.GetCivData();
	if (!data || !data.length)
	{
		throw("InitMapGen: error reading civ data");
	}
	for (var i = 0; i < data.length; ++i)
	{
		var civData = JSON.parse(data[i]);
		g_CivData[civData.Code] = civData;
	}
	
	// Create new map
	log("Creating new map...");
	var terrain = createTerrain(g_MapSettings.BaseTerrain);
	
	g_Map = new Map(g_MapSettings.Size, g_MapSettings.BaseHeight);
	g_Map.initTerrain(terrain);
}

function ExportMap()
{	// Wrapper for engine function
	log("Saving map...");
	
	// Get necessary data from map
	var data = g_Map.getMapData();
	
	// Add environment and camera settings
	if (!WATER_LEVEL_CHANGED)
	{
		g_Environment.Water.WaterBody.Height = SEA_LEVEL - 0.1;
	}
	data.Environment = g_Environment;
	
	// Adjust default cam to roughly center of the map - useful for Atlas
	g_Camera.Position = {x: g_MapSettings.Size*2, y: g_MapSettings.Size*2, z: -g_MapSettings.Size*2};
	data.Camera = g_Camera;
	
	RMS.ExportMap(data);
}
