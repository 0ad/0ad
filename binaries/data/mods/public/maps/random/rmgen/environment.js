
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

// Set water tint RGB (0,1)
function setWaterTint(r, g, b)
{
	g_Environment.Water.WaterBody.Tint = { "r" : r, "g" : g, "b" : b, "a" : 0};
}

// Set water height
function setWaterHeight(h)
{
	g_Environment.Water.WaterBody.Height = h;
	WATER_LEVEL_CHANGED = true;
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

// Set water type
function setWaterType(m)
{
	g_Environment.Water.WaterBody.Type = m;
}

// Set wind angle for water (thus direction of waves)
function setWindAngle(m)
{
	g_Environment.Water.WaterBody.WindAngle = m;
}

// Set fog factor (0,1)
function setFogFactor(s)
{
	g_Environment.Fog.FogFactor = s / 100.0;
}

// Set fog thickness (0,1)
function setFogThickness(s)
{
	g_Environment.Fog.FogThickness = s;
}

// Set fog color RGB (0,1)
function setFogColor(r, g, b)
{
	g_Environment.Fog.FogColor = { "r" : r, "g" : g, "b" : b, "a" : 0};
}

// Set postproc brightness (0,1)
function setPPBrightness(s)
{
	g_Environment.Postproc.Brightness = s - 0.5;
}

// Set postproc contrast (0,1)
function setPPContrast(s)
{
	g_Environment.Postproc.Contrast = s + 0.5;
}

// Set postproc saturation (0,1)
function setPPSaturation(s)
{
	g_Environment.Postproc.Saturation = s * 2;
}

// Set postproc bloom (0,1)
function setPPBloom(s)
{
	g_Environment.Postproc.Bloom = (1 - s) * 0.2;
}

// Set postproc effect ("default", "hdr", "DOF")
function setPPEffect(s)
{
	g_Environment.Postproc.PostprocEffect = s;
}