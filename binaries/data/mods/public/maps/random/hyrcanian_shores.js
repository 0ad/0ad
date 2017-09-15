RMS.LoadLibrary("rmgen");

const tGrass = ["temp_grass_clovers"];
const tGrassPForest = "temp_plants_bog";
const tGrassDForest = "alpine_dirt_grass_50";
const tCliff = ["temp_cliff_a", "temp_cliff_b"];
const tGrassA = "temp_grass_d";
const tGrassB = "temp_grass_c";
const tGrassC = "temp_grass_clovers_2";
const tHill = ["temp_highlands", "temp_grass_long_b"];
const tDirt = ["temp_dirt_gravel", "temp_dirt_gravel_b"];
const tRoad = "temp_road";
const tRoadWild = "temp_road_overgrown";
const tGrassPatch = "temp_grass_plants";
const tShoreBlend = "temp_mud_plants";
const tShore = "medit_sand_wet";
const tWater = "medit_sand_wet";

const oPoplar = "gaia/flora_tree_poplar";
const oPalm = "gaia/flora_tree_cretan_date_palm_short";
const oApple = "gaia/flora_tree_apple";
const oOak = "gaia/flora_tree_oak";
const oBerryBush = "gaia/flora_bush_berry";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oGoat = "gaia/fauna_goat";
const oBoar = "gaia/fauna_boar";
const oStoneLarge = "gaia/geology_stonemine_temperate_quarry";
const oStoneSmall = "gaia/geology_stone_temperate";
const oMetalLarge = "gaia/geology_metal_temperate_slabs";

const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassShort = "actor|props/flora/grass_soft_large.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMedium = "actor|geology/stone_granite_med.xml";
const aBushMedium = "actor|props/flora/bush_medit_me_lush.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm_lush.xml";

// terrain + entity (for painting)
const pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oPoplar, tGrassDForest];
const pForestP = [tGrassPForest + TERRAIN_SEPARATOR + oOak, tGrassPForest];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapArea = mapSize*mapSize;

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();
var clSettlement = createTileClass();
var clSea = createTileClass();
var clHighlands = createTileClass();
var clFlatlands = createTileClass();

var playerIDs = sortAllPlayers();

for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);
	var cliffRadius = 2;
	var elevation = 20;

	// get the x and z in tiles
	var fx = fractionToTiles((i + 1) / (numPlayers + 1));
	var fz = fractionToTiles(0.4 + 0.2 * (i % 2));
	var ix = round(fx);
	var iz = round(fz);

	addCivicCenterAreaToClass(ix, iz, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	var placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(oBerryBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI/3)
		mAngle = randFloat(0, TWO_PI);

	var mDist = 12;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oMetalLarge, 1,1, 0,0)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(oStoneLarge, 1,1, 0,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);

	// create starting trees
	var num = 2;
	var tAngle = randFloat(-PI/3, 4*PI/3);
	var tDist = randFloat(11, 13);
	var tX = round(fx + tDist * cos(tAngle));
	var tZ = round(fz + tDist * sin(tAngle));
	group = new SimpleGroup(
		[new SimpleObject(oOak, num, num, 0,5)],
		false, clBaseResource, tX, tZ
	);
	createObjectGroup(group, 0, avoidClasses(clBaseResource,2));

	placeDefaultDecoratives(fx, fz, aGrassShort, clBaseResource, radius);
}
RMS.SetProgress(10);

paintRiver({
	"horizontal": true,
	"parallel": true,
	"position": 1,
	"width": 0.5,
	"fadeDist": 0.05,
	"deviation": 0,
	"waterHeight": -3,
	"landHeight": 1,
	"meanderShort": 20,
	"meanderLong": 0,
	"waterFunc": (ix, iz, height) => {

		if (height < 0)
			addToClass(ix, iz, clWater);

		if (height < -1.5)
			placeTerrain(ix, iz, tWater);
		else
			placeTerrain(ix, iz, tShore);
	},
	"landFunc": (ix, iz, shoreDist1, shoreDist2) => {
		addToClass(ix, iz, clHighlands);
	}
});

RMS.SetProgress(20);

log("Creating fish...");

num = scaleByMapSize(10, 20);
for (var i=0; i < num; i++){
group = new SimpleGroup(
	[new SimpleObject(oFish, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	[stayClasses(clWater, 2), avoidClasses(clFood, 3)],
	numPlayers, 50
);
}

RMS.SetProgress(25);

log("Creating bumps...");
placer = new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1);
painter = new SmoothElevationPainter(ELEVATION_MODIFY, 4, 3);
createAreas(
	placer,
	painter,
	stayClasses(clHighlands, 1),
	scaleByMapSize(300, 600)
);

RMS.SetProgress(30);

log("Creating hills...");
placer = new ClumpPlacer(scaleByMapSize(20, 150), 0.2, 0.1, 1);
var terrainPainter = new LayeredPainter(
	[tCliff, tHill],		// terrains
	[2]								// widths
);
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 15, 2);
createAreas(
	placer,
	[terrainPainter, elevationPainter, paintClass(clHill)],
	avoidClasses(clSea, 5, clPlayer, 20, clWater, 5, clHill, 15, clHighlands, 5),
	scaleByMapSize(1, 4) * numPlayers
);

RMS.SetProgress(35);

// calculate desired number of trees for map (based on size)
const MIN_TREES = 500;
const MAX_TREES = 2500;
const P_FOREST = 0.7;

var totalTrees = scaleByMapSize(MIN_TREES, MAX_TREES);
var numForest = totalTrees * P_FOREST;
var numStragglers = totalTrees * (1.0 - P_FOREST);

log("Creating mainland forests...");
var types = [
	[[tGrassDForest, tGrass, pForestD], [tGrassDForest, pForestD]]
];	// some variation
var size = numForest*1.3 / (scaleByMapSize(2,8) * numPlayers);
var num = floor(0.7*size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ClumpPlacer(numForest / num, 0.1, 0.1, 1);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)],
		avoidClasses(clPlayer, 20, clWater, 3, clForest, 10, clHill, 0, clSea, 6, clBaseResource, 3),
		num
	);
}
RMS.SetProgress(45);

log("Creating highland forests...");
var types = [
	[[tGrassDForest, tGrass, pForestP], [tGrassDForest, pForestP]]
];	// some variation
var size = numForest / (scaleByMapSize(2,8) * numPlayers);
var num = floor(size / types.length);
for (var i = 0; i < types.length; ++i)
{
	placer = new ClumpPlacer(numForest / num, 0.1, 0.1, 1);
	painter = new LayeredPainter(
		types[i],		// terrains
		[2]											// widths
		);
	createAreas(
		placer,
		[painter, paintClass(clForest)],
		avoidClasses(clPlayer, 20, clWater, 3, clForest, 2, clHill, 0, clSea, 6, clFlatlands, 3),
		num
	);
}
RMS.SetProgress(70);

log("Creating dirt patches...");
var sizes = [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter(
		[[tGrass,tGrassA],[tGrassA,tGrassB], [tGrassB,tGrassC]], 		// terrains
		[1,1]															// widths
	);
	createAreas(
		placer,
		[painter, paintClass(clDirt)],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 4, clSea, 0),
		scaleByMapSize(15, 45)
	);
}
RMS.SetProgress(75);

log("Creating grass patches...");
var sizes = [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)];
for (var i = 0; i < sizes.length; i++)
{
	placer = new ClumpPlacer(sizes[i], 0.3, 0.06, 0.5);
	painter = new LayeredPainter([tGrassC, tGrassPatch], [2]);
	createAreas(
		placer,
		painter,
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 4, clSea, 0),
		scaleByMapSize(15, 45)
	);
}

RMS.SetProgress(80);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clRock, 10, clSea, 2, clHill, 2)],
	scaleByMapSize(4,16), 100
);

log("Creating small stone quarries...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clRock, 10, clSea, 2, clHill, 2)],
	scaleByMapSize(4,16), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	[avoidClasses(clWater, 0, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clSea, 0, clHill, 2)],
	scaleByMapSize(4,16), 100
);

RMS.SetProgress(85);

log("Creating small decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clSea, 0),
	scaleByMapSize(16, 262), 50
);
RMS.SetProgress(90);

log("Creating large decorative rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 0, clHill, 0, clSea, 0),
	scaleByMapSize(8, 131), 50
);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 0, clFood, 5, clSea, 0, clFlatlands, 0),
	6 * numPlayers, 50
);

log("Creating sheep...");
group = new SimpleGroup(
	[new SimpleObject(oGoat, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 0, clFood, 20, clSea, 0),
	3 * numPlayers, 50
);

log("Creating berry bush...");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 6, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

log("Creating boar...");
group = new SimpleGroup(
	[new SimpleObject(oBoar, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 0, clForest, 0, clPlayer, 20, clHill, 0, clFood, 20, clSea, 0, clFlatlands, 0),
	2 * numPlayers, 50
);

log("Creating straggler trees...");
var types = [oPoplar, oPalm, oApple];	// some variation
var num = floor(numStragglers / types.length);
for (var i = 0; i < types.length; ++i)
{
	group = new SimpleGroup(
		[new SimpleObject(types[i], 1,1, 0,3)],
		true, clForest
	);
	createObjectGroupsDeprecated(group, 0,
		avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 10, clMetal, 6, clRock, 6, clSea, 1, clHighlands, 25),
		num
	);
}

log("Creating small grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clHill, 2, clPlayer, 2, clDirt, 0, clSea, 1),
	scaleByMapSize(13, 200)
);

log("Creating large grass tufts...");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -PI/8,PI/8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -PI/8,PI/8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0, clSea, 1),
	scaleByMapSize(13, 200)
);
RMS.SetProgress(95);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 1, clHill, 1, clPlayer, 1, clDirt, 1, clSea, 1),
	scaleByMapSize(13, 200), 50
);

setSkySet("cirrus");
setWaterColor(0.114, 0.192, 0.463);
setWaterTint(0.255, 0.361, 0.651);
setWaterWaviness(2.0);
setWaterType("ocean");
setWaterMurkiness(0.83);

ExportMap();
