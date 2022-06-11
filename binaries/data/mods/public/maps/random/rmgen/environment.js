/**
 * @file The environment settings govern the appearance of the Sky, Fog, Water and Post-Processing effects.
 */

var g_Environment = {
	"SkySet": "default", // textures for the skybox, subdirectory name of art/textures/skies
	"SunColor": { "r": 1.03162, "g": 0.99521, "b": 0.865752, "a": 0 }, // all rgb from 0 to 1
	"SunElevation": 0.7, // 0 to 2pi
	"SunRotation": -0.909, // 0 to 2pi
	"AmbientColor": { "r": 0.364706, "g": 0.376471, "b": 0.419608, "a": 0 },
	"Water": {
		"WaterBody": {
			"Type": "ocean", // Subdirectory name of art/textures/animated/water
			"Color": { "r": 0.3, "g": 0.35, "b": 0.7, "a": 0 },
			"Tint": { "r": 0.28, "g": 0.3, "b": 0.59, "a": 0 },
			"Height": undefined, // TODO: default shouldn't be set in ExportMap
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

/**
 * Camera location in case there are no player entities.
 */
var g_Camera = {
	"Position": { "x": 256, "y": 150, "z": 256 },
	"Rotation": 0,
	"Declination": 0.523599
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

function setAmbientColor(r, g, b)
{
	g_Environment.AmbientColor = { "r" : r, "g" : g, "b" : b, "a" : 0 };
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
