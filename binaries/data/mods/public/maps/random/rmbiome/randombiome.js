var g_BiomeID;

var g_Terrains = {};
var g_Gaia = {};
var g_Decoratives = {};
var g_ResourceCounts = {};
var g_Heights = {};

function currentBiome()
{
	return g_BiomeID;
}

function setSelectedBiome()
{
	// TODO: Replace ugly default for atlas by a dropdown
	setBiome(g_MapSettings.Biome || "generic/alpine");
}

function setBiome(biomeID)
{
	RandomMapLogger.prototype.printDirectly("Setting biome " + biomeID + ".\n");

	loadBiomeFile("defaultbiome");

	setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
	setSunRotation(randomAngle());
	setSunElevation(Math.PI * randFloat(1/6, 1/3));

	g_BiomeID = biomeID;

	loadBiomeFile(biomeID);

	Engine.LoadLibrary("rmbiome/" + biomeID.slice(0, biomeID.lastIndexOf("/")));
	let setupBiomeFunc = global["setupBiome_" + basename(biomeID)];
	if (setupBiomeFunc)
		setupBiomeFunc();
}

/**
 * Copies JSON contents to defined global variables.
 */
function loadBiomeFile(file)
{
	let path = "maps/random/rmbiome/" + file + ".json";

	if (!Engine.FileExists(path))
	{
		error("Could not load biome file '" + file + "'");
		return;
	}

	let biome = Engine.ReadJSONFile(path)

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
	{
		if (rmsGlobal == "Description")
			continue;

		if (!global["g_" + rmsGlobal])
			throw new Error(rmsGlobal + " not defined!");

		copyProperties(biome[rmsGlobal], global["g_" + rmsGlobal]);
	}
}

function rBiomeTreeCount(multiplier = 1)
{
	return [
		g_ResourceCounts.trees.min * multiplier,
		g_ResourceCounts.trees.max * multiplier,
		g_ResourceCounts.trees.forestProbability
	];
}
