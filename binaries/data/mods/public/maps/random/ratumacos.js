/**
 * Heightmap image source:
 * Imagery by Jesse Allen, NASA's Earth Observatory,
 * using data from the General Bathymetric Chart of the Oceans (GEBCO)
 * produced by the British Oceanographic Data Centre.
 * https://visibleearth.nasa.gov/view.php?id=73934
 *
 * Licensing: Public Domain, https://visibleearth.nasa.gov/useterms.php
 *
 * The heightmap image is reproduced using:
 * wget https://eoimages.gsfc.nasa.gov/images/imagerecords/73000/73934/gebco_08_rev_elev_C1_grey_geo.tif
 * lat=49.31; lon=1.1; width=1
 * lat1=$(bc <<< ";scale=5;$lat-$width/2"); lon1=$(bc <<< ";scale=5;$lon+$width/2"); lat2=$(bc <<< ";scale=5;$lat+$width/2"); lon2=$(bc <<< ";scale=5;$lon-$width/2")
 * gdal_translate -projwin $lon2 $lat2 $lon1 $lat1 gebco_08_rev_elev_C1_grey_geo.tif ratumacos.tif
 * convert ratumacos.tif -resize 512 -contrast-stretch 0 ratumacos.png
 * No further changes should be applied to the image to keep it easily interchangeable.
 */

Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");
Engine.LoadLibrary("rmgen2");
Engine.LoadLibrary("rmbiome");

setBiome("generic/alpine");

g_Terrains.mainTerrain = "new_alpine_grass_d";
g_Terrains.forestFloor1 = "alpine_grass_d";
g_Terrains.forestFloor2 = "alpine_grass_c";
g_Terrains.tier1Terrain = "new_alpine_grass_c";
g_Terrains.tier2Terrain = "new_alpine_grass_b";
g_Terrains.tier3Terrain = "alpine_grass_a";
g_Terrains.tier4Terrain = "new_alpine_grass_e";

g_Gaia.mainHuntableAnimal = "gaia/fauna_deer";
g_Gaia.secondaryHuntableAnimal = "gaia/fauna_pig";
g_Gaia.fish = "gaia/fauna_fish_tilapia";
g_Gaia.tree1 = "gaia/flora_tree_poplar";
g_Gaia.tree2 = "gaia/flora_tree_toona";
g_Gaia.tree3 = "gaia/flora_tree_apple";
g_Gaia.tree4 = "gaia/flora_tree_acacia";
g_Gaia.tree5 = "gaia/flora_tree_carob";

g_Decoratives.grass = "actor|props/flora/grass_soft_large.xml";
g_Decoratives.grassShort = "actor|props/flora/grass_tufts_a.xml";
g_Decoratives.rockLarge = "actor|geology/stone_granite_med.xml";
g_Decoratives.rockMedium = "actor|geology/stone_granite_small.xml";
g_Decoratives.bushMedium = "actor|props/flora/bush_tempe_a.xml";
g_Decoratives.bushSmall = "actor|props/flora/bush_tempe_b.xml";
g_Decoratives.reeds = "actor|props/flora/reeds_pond_lush_a.xml";
g_Decoratives.lillies = "actor|props/flora/water_lillies.xml";

const heightScale = num => num * g_MapSettings.Size / 320;

const heightReedsMin = heightScale(-2);
const heightShallow = heightScale(-1);
const heightWaterLevel = heightScale(0);
const heightShoreline = heightScale(3);
const heightPlayer = heightScale(10);

const g_Map = new RandomMap(0, g_Terrains.mainTerrain);
const mapBounds = g_Map.getBounds();
const mapCenter = g_Map.getCenter();

const riverAngle = 0.65 * Math.PI;

initTileClasses(["shoreline", "shallows"]);

g_Map.LoadHeightmapImage("ratumacos.png", -3, 20);
Engine.SetProgress(15);

g_Map.log("Smoothing heightmap");
createArea(
	new MapBoundsPlacer(),
	new SmoothingPainter(1, 0.1, 1));
Engine.SetProgress(25);

g_Map.log("Creating shallows...");
for (let i = 0; i < scaleByMapSize(5, 12); ++i)
{
	let x = fractionToTiles(randFloat(0, 1));
	createPassage({
		"start": new Vector2D(x, mapBounds.bottom).rotateAround(riverAngle + Math.PI / 2 * randFloat(0.8, 1.2), mapCenter),
		"end": new Vector2D(x, mapBounds.top).rotateAround(riverAngle + Math.PI / 2 * randFloat(0.8, 1.2), mapCenter),
		"startWidth": scaleByMapSize(8, 12),
		"endWidth": scaleByMapSize(8, 12),
		"smoothWidth": 2,
		"startHeight": heightShallow,
		"endHeight": heightShallow,
		"constraint": new HeightConstraint(-Infinity, heightShallow)
	});
}

g_Map.log("Painting water");
createArea(
	new MapBoundsPlacer(),
	[
		new TerrainPainter(g_Terrains.water),
		new TileClassPainter(g_TileClasses.water)
	],
	new HeightConstraint(-Infinity, heightWaterLevel));
Engine.SetProgress(30);

g_Map.log("Marking land");
createArea(
	new MapBoundsPlacer(),
	new TileClassPainter(g_TileClasses.land),
	avoidClasses(g_TileClasses.water, 0));
Engine.SetProgress(35);

g_Map.log("Painting shoreline");
createArea(
	new MapBoundsPlacer(),
	[
		new TerrainPainter(g_Terrains.shore),
		new TileClassPainter(g_TileClasses.shoreline)
	],
	new HeightConstraint(heightWaterLevel, heightShoreline));
Engine.SetProgress(40);

placePlayerBases({
	"PlayerPlacement": playerPlacementRiver(riverAngle, fractionToTiles(0.6)),
	"PlayerTileClass": g_TileClasses.player,
	"BaseResourceClass": g_TileClasses.baseResource,
	"Walls": "towers",
	"CityPatch": {
		"outerTerrain": g_Terrains.roadWild,
		"innerTerrain": g_Terrains.road,
		"painters": [
			new SmoothElevationPainter(ELEVATION_SET, heightPlayer, 2)
		]
	},
	"Chicken": {
	},
	"Berries": {
		"template": g_Gaia.fruitBush
	},
	"Mines": {
		"types": [
			{ "template": g_Gaia.metalLarge },
			{ "template": g_Gaia.stoneLarge }
		]
	},
	"Trees": {
		"template": g_Gaia.tree1,
		"count": 2
	},
	"Decoratives": {
		"template": g_Decoratives.rockMedium
	}
});

addElements([
	{
		"func": addLayeredPatches,
		"avoid": [
			g_TileClasses.dirt, 5,
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3,
			g_TileClasses.shoreline, 3
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	},
	{
		"func": addDecoration,
		"avoid": [
			g_TileClasses.forest, 2,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 12,
			g_TileClasses.water, 3,
			g_TileClasses.shoreline, 3
		],
		"sizes": ["normal"],
		"mixes": ["normal"],
		"amounts": ["many"]
	}
]);

addElements(shuffleArray([
	{
		"func": addSmallMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 20,
			g_TileClasses.metal, 30,
			g_TileClasses.water, 3,
			g_TileClasses.shoreline, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["few"]
	},
	{
		"func": addMetal,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 30,
			g_TileClasses.metal, 20,
			g_TileClasses.water, 3,
			g_TileClasses.shoreline, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["normal"]
	},
	{
		"func": addStone,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 30,
			g_TileClasses.rock, 30,
			g_TileClasses.metal, 20,
			g_TileClasses.water, 3,
			g_TileClasses.shoreline, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["normal"]
	},
	{
		"func": addForests,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 8,
			g_TileClasses.metal, 3,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 3,
			g_TileClasses.water, 2,
			g_TileClasses.shoreline, 2
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	}
]));

addElements(shuffleArray([
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 3,
			g_TileClasses.shoreline, 3
		],
		"sizes": ["huge"],
		"mixes": ["similar"],
		"amounts": ["tons"]
	},
	{
		"func": addAnimals,
		"avoid": [
			g_TileClasses.animals, 20,
			g_TileClasses.forest, 2,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 3,
			g_TileClasses.shoreline, 3
		],
		"sizes": ["huge"],
		"mixes": ["similar"],
		"amounts": ["tons"]
	},
	{
		"func": addBerries,
		"avoid": [
			g_TileClasses.berries, 30,
			g_TileClasses.forest, 5,
			g_TileClasses.metal, 10,
			g_TileClasses.mountain, 2,
			g_TileClasses.player, 20,
			g_TileClasses.rock, 10,
			g_TileClasses.water, 3,
			g_TileClasses.shoreline, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["many"]
	},
	{
		"func": addStragglerTrees,
		"avoid": [
			g_TileClasses.berries, 5,
			g_TileClasses.forest, 4,
			g_TileClasses.metal, 2,
			g_TileClasses.mountain, 6,
			g_TileClasses.player, 12,
			g_TileClasses.rock, 2,
			g_TileClasses.water, 5,
			g_TileClasses.shoreline, 3
		],
		"sizes": ["normal"],
		"mixes": ["same"],
		"amounts": ["tons"]
	}
]));
Engine.SetProgress(80);

createDecoration(
	[
		[new SimpleObject(g_Decoratives.reeds, 1, 3, 0, 1)],
		[new SimpleObject(g_Decoratives.lillies, 1, 2, 0, 1)]
	],
	[
		200 * Math.pow(scaleByMapSize(3, 12), 2),
		100 * Math.pow(scaleByMapSize(3, 12), 2)
	],
	new HeightConstraint(heightReedsMin, heightShoreline)
);
Engine.SetProgress(90);

g_Map.log("Placing fish...");
addElements([
	{
		"func": addFish,
		"avoid": [
			g_TileClasses.fish, 10,
		],
		"stay": [g_TileClasses.water, 4],
		"sizes": ["normal"],
		"mixes": ["similar"],
		"amounts": ["few"]
	}
]);
Engine.SetProgress(85);

placePlayersNomad(
	g_Map.createTileClass(),
	[
		stayClasses(g_TileClasses.land, 4),
		avoidClasses(
			g_TileClasses.forest, 1,
			g_TileClasses.rock, 4,
			g_TileClasses.metal, 4,
			g_TileClasses.animals, 1)
	]);

setSunColor(0.733, 0.746, 0.574);

setWaterHeight(20 + heightWaterLevel);
setWaterTint(0.224, 0.271, 0.270);
setWaterColor(0.224, 0.271, 0.270);
setWaterWaviness(8);
setWaterMurkiness(0.87);
setWaterType("clap");

setTerrainAmbientColor(0.521, 0.475, 0.322);

setSunRotation(-Math.PI);
setSunElevation(Math.PI / 6.25);

setFogFactor(0);
setFogThickness(0);
setFogColor(0.69, 0.616, 0.541);

setPPEffect("hdr");
setPPContrast(0.67);
setPPSaturation(0.42);
setPPBloom(0.23);

g_Map.ExportMap();
