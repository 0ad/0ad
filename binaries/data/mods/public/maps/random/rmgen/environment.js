var g_Environment = {
	"SkySet": "default", // textures for the skybox, subdirectory name of art/textures/skies
	"SunColor": { "r": 0.749020, "g": 0.749020, "b": 0.749020, "a": 0 }, // all rgb from 0 to 1
	"SunElevation": 0.785398, // 0 to 2pi
	"SunRotation": 5.49779, // 0 to 2pi
	"TerrainAmbientColor": { "r": 0.501961, "g": 0.501961, "b": 0.501961, "a": 0 },
	"UnitsAmbientColor": { "r": 0.501961, "g": 0.501961, "b": 0.501961, "a": 0 },
	"Water": {
		"WaterBody": {
			"Type": "ocean", // Subdirectory name of art/textures/animated/water
			"Color": { "r": 0.3, "g": 0.35, "b": 0.7, "a": 0 },
			"Tint": { "r": 0.28, "g": 0.3, "b": 0.59, "a": 0 },
			"Height": 5, // TODO: The true default is 19.9, see ExportMap() in mapgen.js and  SEA_LEVEL in library.js
			"Waviness": 8, // typically from 0 to 10
			"Murkiness": 0.45, // 0 to 1, amount of tint to blend in with the refracted color
			"WindAngle": 0.0 // 0 to 2pi, direction of waves
		}
	},
	"Fog": {
		"FogFactor": 0.0,
		"FogThickness": 0.5,
		"FogColor": { "r": 0.8, "g": 0.8, "b": 0.8, "a": 0 }
	},
	"Postproc": {
		"Brightness": 0.0,
		"Contrast": 1.0,
		"Saturation": 1.0,
		"Bloom": 0.2,
		"PostprocEffect": "default" // "default", "hdr" or "DOF"
	}
};

////////////////////////////////////////////////////////////////////////////
// Sun, Sky and Terrain
////////////////////////////////////////////////////////////////////////////

function setSkySet(set)
{
	g_Environment.SkySet = set;
}

function setSunColor(r, g, b)
{
	g_Environment.SunColor = { "r" : r, "g" : g, "b" : b, "a" : 0 };
}

function setSunElevation(e)
{
	g_Environment.SunElevation = e;
}

function setSunRotation(r)
{
	g_Environment.SunRotation = r;
}

function setTerrainAmbientColor(r, g, b)
{
	g_Environment.TerrainAmbientColor = { "r" : r, "g" : g, "b" : b, "a" : 0 };
}

function setUnitsAmbientColor(r, g, b)
{
	g_Environment.UnitsAmbientColor = { "r" : r, "g" : g, "b" : b, "a" : 0 };
}

////////////////////////////////////////////////////////////////////////////
// Water
////////////////////////////////////////////////////////////////////////////

function setWaterColor(r, g, b)
{
	g_Environment.Water.WaterBody.Color = { "r" : r, "g" : g, "b" : b, "a" : 0 };
}

function setWaterTint(r, g, b)
{
	g_Environment.Water.WaterBody.Tint = { "r" : r, "g" : g, "b" : b, "a" : 0 };
}

function setWaterHeight(h)
{
	g_Environment.Water.WaterBody.Height = h;
	WATER_LEVEL_CHANGED = true;
}

function setWaterWaviness(w)
{
	g_Environment.Water.WaterBody.Waviness = w;
}

function setWaterMurkiness(m)
{
	g_Environment.Water.WaterBody.Murkiness = m;
}

function setWaterType(m)
{
	g_Environment.Water.WaterBody.Type = m;
}

function setWindAngle(m)
{
	g_Environment.Water.WaterBody.WindAngle = m;
}

////////////////////////////////////////////////////////////////////////////
// Fog (numerical arguments between 0 and 1)
////////////////////////////////////////////////////////////////////////////

function setFogFactor(s)
{
	g_Environment.Fog.FogFactor = s / 100.0;
}

function setFogThickness(thickness)
{
	g_Environment.Fog.FogThickness = thickness;
}

function setFogColor(r, g, b)
{
	g_Environment.Fog.FogColor = { "r" : r, "g" : g, "b" : b, "a" : 0 };
}

////////////////////////////////////////////////////////////////////////////
// Post Processing (numerical arguments between 0 and 1)
////////////////////////////////////////////////////////////////////////////

function setPPBrightness(s)
{
	g_Environment.Postproc.Brightness = s - 0.5;
}

function setPPContrast(s)
{
	g_Environment.Postproc.Contrast = s + 0.5;
}

function setPPSaturation(s)
{
	g_Environment.Postproc.Saturation = s * 2;
}

function setPPBloom(s)
{
	g_Environment.Postproc.Bloom = (1 - s) * 0.2;
}

function setPPEffect(s)
{
	g_Environment.Postproc.PostprocEffect = s;
}
