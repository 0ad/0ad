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

function rBiomeT1()
{
	return g_Terrains.mainTerrain;
}

function rBiomeT2()
{
	return g_Terrains.forestFloor1;
}

function rBiomeT3()
{
	return g_Terrains.forestFloor2;
}

function rBiomeT4()
{
	return g_Terrains.cliff;
}

function rBiomeT5()
{
	return g_Terrains.tier1Terrain;
}

function rBiomeT6()
{
	return g_Terrains.tier2Terrain;
}

function rBiomeT7()
{
	return g_Terrains.tier3Terrain;
}

function rBiomeT8()
{
	return g_Terrains.hill;
}

function rBiomeT9()
{
	return g_Terrains.dirt;
}

function rBiomeT10()
{
	return g_Terrains.road;
}

function rBiomeT11()
{
	return g_Terrains.roadWild;
}

function rBiomeT12()
{
	return g_Terrains.tier4Terrain;
}

function rBiomeT13()
{
	return g_Terrains.shoreBlend;
}

function rBiomeT14()
{
	return g_Terrains.shore;
}

function rBiomeT15()
{
	return g_Terrains.water;
}

function rBiomeE1()
{
	return g_Gaia.tree1;
}

function rBiomeE2()
{
	return g_Gaia.tree2;
}

function rBiomeE3()
{
	return g_Gaia.tree3;
}

function rBiomeE4()
{
	return g_Gaia.tree4;
}

function rBiomeE5()
{
	return g_Gaia.tree5;
}

function rBiomeE6()
{
	return g_Gaia.fruitBush;
}

function rBiomeE7()
{
	return g_Gaia.chicken;
}

function rBiomeE8()
{
	return g_Gaia.mainHuntableAnimal;
}

function rBiomeE9()
{
	return g_Gaia.fish;
}

function rBiomeE10()
{
	return g_Gaia.secondaryHuntableAnimal;
}

function rBiomeE11()
{
	return g_Gaia.stoneLarge;
}

function rBiomeE12()
{
	return g_Gaia.stoneSmall;
}

function rBiomeE13()
{
	return g_Gaia.metalLarge;
}

function rBiomeA1()
{
	return g_Decoratives.grass;
}

function rBiomeA2()
{
	return g_Decoratives.grassShort;
}

function rBiomeA3()
{
	return g_Decoratives.reeds;
}

function rBiomeA4()
{
	return g_Decoratives.lillies;
}

function rBiomeA5()
{
	return g_Decoratives.rockLarge;
}

function rBiomeA6()
{
	return g_Decoratives.rockMedium;
}

function rBiomeA7()
{
	return g_Decoratives.bushMedium;
}

function rBiomeA8()
{
	return g_Decoratives.bushSmall;
}

function rBiomeA9()
{
	return g_Decoratives.tree;
}
