RMS.LoadLibrary("rmbiome/biomes");

var g_BiomeID;

var g_Terrains = {};
var g_Gaia = {};
var g_Decoratives = {};
var g_TreeCount = {};

function currentBiome()
{
	return g_BiomeID;
}

function setSelectedBiome()
{
	setBiome(g_MapSettings.Biome);
}

function setBiome(biomeID)
{
	loadBiomeFile("defaultbiome");

	setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
	setSunRotation(randFloat(0, TWO_PI));
	setSunElevation(randFloat(PI/ 6, PI / 3));

	g_BiomeID = biomeID;

	loadBiomeFile("biomes/" + biomeID);

	let setupBiomeFunc = global["setupBiome_" + biomeID];
	if (setupBiomeFunc)
		setupBiomeFunc();
}

function loadBiomeFile(file)
{
	let path = "maps/random/rmbiome/" + file + ".json";

	if (!RMS.FileExists(path))
	{
		error("Could not load biome file '" + file + "'");
		return;
	}

	let biome = RMS.ReadJSONFile(path)

	let copyProperties = (from, to) => {
		for (let prop in from)
		{
			if (typeof from[prop] == "object" && !Array.isArray(from[prop]))
			{
				if (!to[prop])
					to[prop] = {};

				copyProperties(from[prop], to[prop]);
			}
			else
				to[prop] = from[prop];
		}
	};

	for (let rmsGlobal in biome)
		copyProperties(biome[rmsGlobal], global["g_" + rmsGlobal]);
}

function rBiomeTreeCount(multiplier = 1)
{
	return [
		g_TreeCount.minTrees * multiplier,
		g_TreeCount.maxTrees * multiplier,
		g_TreeCount.forestProbability
	];
}
