
////////////////////////////////////////////////////////////////////////////
//	Sky + lighting
////////////////////////////////////////////////////////////////////////////

// Set skyset
function setSkySet(set)
{
	g_Environment.SkySet = set;
}

// Set sun colour RGB
function setSunColour(r, g, b)
{
	g_Environment.SunColour = { "r" : r, "g" : g, "b" : b, "a" : 0};
}

// Set sun elevation
function setSunElevation(e)
{
	g_Environment.SunElevation = e;
}

// Set sun rotation
function setSunRotation(r)
{
	g_Environment.SunRotation = r;
}

// Set terrain ambient colour RGB (0-1)
function setTerrainAmbientColour(r, g, b)
{
	g_Environment.TerrainAmbientColour = { "r" : r, "g" : g, "b" : b, "a" : 0};
}

// Set terrain ambient colour RGB (0-1)
function setUnitsAmbientColour(r, g, b)
{
	g_Environment.UnitsAmbientColour = { "r" : r, "g" : g, "b" : b, "a" : 0};
}

////////////////////////////////////////////////////////////////////////////
// 	Water
////////////////////////////////////////////////////////////////////////////
			
// Set water colour RGB (0,1)
function setWaterColour(r, g, b)
{
	g_Environment.Water.WaterBody.Colour = { "r" : r, "g" : g, "b" : b, "a" : 0};
}

// Set water height
function setWaterHeight(h)
{
	g_Environment.Water.WaterBody.Height = h;
	WATER_LEVEL_CHANGED = true;
}

// Set water shininess
function setWaterShininess(s)
{
	g_Environment.Water.WaterBody.Shininess = s;
}

// Set water waviness
function setWaterWaviness(w)
{
	g_Environment.Water.WaterBody.Waviness = w;
}

// Set water murkiness
function setWaterMurkiness(m)
{
	g_Environment.Water.WaterBody.Murkiness = m;
}

// Set water tint RGB (0,1)
function setWaterTint(r, g, b)
{
	g_Environment.Water.WaterBody.Tint = { "r" : r, "g" : g, "b" : b, "a" : 0};
}

// Set water reflection tint RGB (0,1)
function setWaterReflectionTint(r, g, b)
{
	g_Environment.Water.WaterBody.WaterReflectionTint = { "r" : r, "g" : g, "b" : b, "a" : 0};
}

// Set water reflection tint strength (0,1)
function setWaterReflectionTintStrength(s)
{
	g_Environment.Water.WaterBody.WaterReflectionTintStrength = s;
}
