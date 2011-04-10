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
			Type: "default",
			Colour: {r: 0.3, g: 0.35, b: 0.7, a: 0},
			Height: 5,
			Shininess: 150,
			Waviness: 8,
			Murkiness: 0.45,
			Tint: {r: 0.28, g: 0.3, b: 0.59, a: 0},
			ReflectionTint: {r: 0.28, g: 0.3, b: 0.59, a: 0},
			ReflectionTintStrength: 0.0
		}
	}
};

var g_Camera = {
	Position: {x: 100, y: 150, z: -100},
	Rotation: 0,
	Declination: 0.523599
};

/////////////////////////////////////////////////////////////////////////////////////

function InitMap()
{
	if (g_MapSettings === undefined)
	{
		// Should never get this far, failed settings would abort prior to loading scripts
		error("InitMapGen: settings missing");
	}
	
	// Create new map
	log("Creating new map...");
	var terrain = createTerrain(g_MapSettings.BaseTerrain);
	
	// XXX: Temporary hack to keep typed arrays from complaining about invalid arguments,
	//		until SpiderMonkey gets upgraded
	g_MapSettings.Size = Math.floor(g_MapSettings.Size);
	
	g_Map = new Map(g_MapSettings.Size * TILES_PER_PATCH, g_MapSettings.BaseHeight);
	g_Map.initTerrain(terrain);
}

function ExportMap()
{	// Wrapper for engine function
	log("Saving map...");
	
	// Get necessary data from map
	var data = g_Map.getMapData();
	
	// Add environment and camera settings
	g_Environment.Water.WaterBody.Height = SEA_LEVEL - 0.1;
	data.Environment = g_Environment;
	data.Camera = g_Camera;
	
	RMS.ExportMap(data);
}
