var g_Map;

var g_Environment = {
	SkySet: "cirrus",
	SunColour: {r: 1.47461, g: 1.47461, b: 1.47461},
	SunElevation: 0.951868,
	SunRotation: -0.532844,
	TerrainAmbientColour: {r: 0.337255, g: 0.403922, b: 0.466667},
	UnitsAmbientColour: {r: 0.501961, g: 0.501961, b: 0.501961},
	Water: {
		WaterBody: {
			Type: "default",
			Colour: {r: 0.294118, g: 0.34902, b: 0.694118},
			Height: 17.6262,
			Shininess: 150,
			Waviness: 8,
			Murkiness: 0.458008,
			Tint: {r: 0.447059, g: 0.411765, b: 0.321569},
			ReflectionTint: {r: 0.619608, g: 0.584314, b: 0.47451},
			ReflectionTintStrength: 0.298828
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
	if (g_MapSettings === undefined || g_MapSettings == {})
	{	// If settings missing, warn and use some defaults
		warn("InitMapGen: settings missing");
		g_MapSettings = {
			Size : 13,
			BaseTerrain: "grass1_spring",
			BaseHeight: 0,
			PlayerData : [ {}, {} ]
		};
	}
	
	// Create new map
	log("Creating new map...");
	var terrain = createTerrain(g_MapSettings.BaseTerrain);
	
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
