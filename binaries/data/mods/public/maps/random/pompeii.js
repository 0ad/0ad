// Location: 40.942707, 14.370705
// Map Width: 80km

RMS.LoadLibrary("rmgen");
RMS.LoadLibrary("rmgen2");
RMS.LoadLibrary("rmbiome");

InitMap();

log("Initializing tile classes...");
setBiome("mediterranean");
initMapSettings();
initTileClasses(["decorative", "lava"]);

log("Initializing environment...");
setSunColor(0.8, 0.8, 0.8);

setWaterTint(0.5, 0.5, 0.5);
setWaterColor(0.3, 0.3, 0.3);
setWaterWaviness(8);
setWaterMurkiness(0.87);
setWaterType("lake");

setTerrainAmbientColor(0.3, 0.3, 0.3);
setUnitsAmbientColor(0.3, 0.3, 0.3);

setSunRotation(-1 * PI);
setSunElevation(PI / 6.25);

setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setSkySet("stormy");

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

log("Initializing biome...");
g_Terrains.mainTerrain = "ocean_rock_a";
g_Terrains.forestFloor1 = "dirt_burned";
g_Terrains.forestFloor2 = "shoreline_stoney_a";
g_Terrains.tier1Terrain = "rock_metamorphic";
g_Terrains.tier2Terrain = "fissures";
g_Terrains.tier3Terrain = "LavaTest06";
g_Terrains.tier4Terrain = "ocean_rock_b";
g_Terrains.roadWild = "road1";
g_Terrains.road = "road1";
g_Gaia.mainHuntableAnimal = "gaia/fauna_goat";
g_Gaia.secondaryHuntableAnimal =  "gaia/fauna_hawk";
g_Gaia.fruitBush = "gaia/fauna_chicken";
g_Gaia.fish = "gaia/fauna_fish";
g_Gaia.tree1 = "gaia/flora_tree_dead";
g_Gaia.tree2 = "gaia/flora_tree_oak_dead";
g_Gaia.tree3 = "gaia/flora_tree_dead";
g_Gaia.tree4 = "gaia/flora_tree_oak_dead";
g_Gaia.tree5 = "gaia/flora_tree_dead";
g_Gaia.stoneSmall = "gaia/geology_stone_alpine_a";
g_Decoratives.grass = "actor|props/flora/grass_field_parched_short.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_soft_dry_tuft_a.xml";
g_Decoratives.bushMedium = "actor|props/special/eyecandy/barrels_buried.xml";
g_Decoratives.bushSmall = "actor|props/special/eyecandy/handcart_1_broken.xml";
initBiome();
RMS.SetProgress(5);

log("Resetting terrain...");
resetTerrain(g_Terrains.mainTerrain, g_TileClasses.land, 1);
RMS.SetProgress(10);

log("Copying heightmap...");
var scale = paintHeightmap("pompeii", (tile, x, y) => {
	if (tile.indexOf("mud_slide") >= 0)
		addToClass(x, y, g_TileClasses.mountain);
	else if (tile.indexOf("Lava") >= 0)
		addToClass(x, y, g_TileClasses.lava);
});

log("Paint tile classes...");
paintTileClassBasedOnHeight(-100, -1, 3, g_TileClasses.water);
RMS.SetProgress(30);

log("Placing players...");
//Coordinate system of the heightmap
var singleBases = [
	[220,80],
	[70,140],
	[180,270],
	[280,280],
	[50,270]
];

if (g_MapInfo.mapSize >= 320 || g_MapInfo.numPlayers > singleBases.length)
	singleBases.push(
		[50,200],
		[125,190],
		[180,140]
	);

randomPlayerPlacementAt(singleBases, [], scale, 0.06);
RMS.SetProgress(40);

addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.lava, 2,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.lava, 2,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["normal"]
	}
]);
RMS.SetProgress(50);

addElements(shuffleArray([
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 10,
			g_TileClasses.metal, 20,
			g_TileClasses.lava, 5,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 10,
			g_TileClasses.lava, 5,
			g_TileClasses.water, 5
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 18,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 5,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			//g_TileClasses.lava, 10,
			g_TileClasses.water, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));
RMS.SetProgress(60);

addElements(shuffleArray([
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.lava, 10,
			g_TileClasses.water, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addFish,
		"avoid": [
			g_TileClasses.fish, 12,
			g_TileClasses.player, 8
		],
		"stay": [g_TileClasses.water, 4],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 7,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 1,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.lava, 5,
			g_TileClasses.water, 5
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
RMS.SetProgress(65);

log("Adding smoke...");
var smokeActors = [
	new Entity("actor|particle/smoke_volcano.xml", 0, 178, 112, 0),
	new Entity("actor|particle/smoke_volcano.xml", 0, 179, 112, 0),
	new Entity("actor|particle/smoke_volcano.xml", 0, 180, 111, 0),
	new Entity("actor|particle/smoke_volcano.xml", 0, 177, 111, 0),
	new Entity("actor|particle/smoke_curved.xml", 0, 176, 111, 0),
	new Entity("actor|particle/smoke_volcano.xml", 0, 177, 112, 0),
	new Entity("actor|particle/smoke_curved.xml", 0, 181, 111, 0),
	new Entity("actor|particle/smoke_volcano.xml", 0, 180, 112, 0),
];

for (let smoke of smokeActors)
{
	smoke.position.x = Math.floor(smoke.position.x / scale);
	smoke.position.z = Math.floor(smoke.position.z / scale);
	g_Map.addObject(smoke);
}
RMS.SetProgress(70);

log("Adding gatherable stone ruins...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject("gaia/special_ruins_stone_statues_roman", 1, 1, 1, 4)],
		true,
		g_TileClasses.metal
	),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 20,
		g_TileClasses.mountain, 3,
		g_TileClasses.forest, 2,
		g_TileClasses.lava, 5,
		g_TileClasses.metal, 20
	),
	5 * scaleByMapSize(1, 4),
	500
);
RMS.SetProgress(75);

log("Adding stone ruins...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[
			new SimpleObject("other/unfinished_greek_temple", 0, 1, 1, 4),
			new SimpleObject("gaia/special_ruins_column_doric", 1, 1, 1, 4)
		],
		true,
		g_TileClasses.decorative
	),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 20,
		g_TileClasses.mountain, 5,
		g_TileClasses.forest, 2,
		g_TileClasses.lava, 5,
		g_TileClasses.decorative, 20
	),
	10 * scaleByMapSize(1, 4),
	200
);
RMS.SetProgress(80);

log("Adding shipwrecks...");
var shipwrecks = [
	"shipwreck_hull",
	"shipwreck_ram_side",
	"shipwreck_sail_boat",
	"shipwreck_sail_boat_cut",
	"barrels_floating"
].map(shipwreck => new SimpleObject("actor|props/special/eyecandy/" + shipwreck + ".xml", 0, 1, 1, 20));

createObjectGroupsDeprecated(
	new SimpleGroup(shipwrecks, true, g_TileClasses.decorative),
	0,
	[
		avoidClasses(g_TileClasses.decorative, 20),
		stayClasses(g_TileClasses.water, 0)
	],
	6 * scaleByMapSize(1, 4),
	200
);
RMS.SetProgress(85);

log("Adding more ruins...");
var ruins = [
	"statue_aphrodite_huge",
	"sele_colonnade",
	"well_1_b",
	"anvil",
	"wheel_laying",
	"vase_rome_a"
].map(ruin => new SimpleObject("actor|props/special/eyecandy/" + ruin + ".xml", 0, 1, 1, 20));

createObjectGroupsDeprecated(
	new SimpleGroup(ruins, true, g_TileClasses.decorative),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 20,
		g_TileClasses.mountain, 2,
		g_TileClasses.forest, 2,
		g_TileClasses.lava, 5,
		g_TileClasses.decorative, 20
	),
	10 * scaleByMapSize(1, 4),
	200
);
RMS.SetProgress(90);

log("Adding bodies...");
createObjectGroupsDeprecated(
	new SimpleGroup(
		[new SimpleObject("actor|props/special/eyecandy/skeleton.xml", 3, 10, 1, 7)],
		true,
		g_TileClasses.dirt
	),
	0,
	avoidClasses(
		g_TileClasses.water, 2,
		g_TileClasses.player, 10,
		g_TileClasses.mountain, 2,
		g_TileClasses.forest, 2,
		g_TileClasses.decorative, 2
	),
	30 * scaleByMapSize(1, 4),
	200
);
RMS.SetProgress(95);

ExportMap();
