Engine.LoadLibrary("rmgen");

const WATER_WIDTH = 0.1;

const tOceanDepths = "medit_sea_depths";
const tOceanRockDeep = "medit_sea_coral_deep";
const tOceanRockShallow = "medit_rocks_wet";
const tOceanCoral = "medit_sea_coral_plants";
const tBeachWet = "medit_sand_wet";
const tBeachDry = "medit_sand";
const tBeachGrass = "medit_rocks_grass";
const tBeachCliff = "medit_dirt";
const tCity = "medit_city_tile";
const tGrassDry = ["medit_grass_field_brown", "medit_grass_field_dry", "medit_grass_field_b"];
const tGrass = ["medit_grass_field_dry", "medit_grass_field_brown", "medit_grass_field_b"];
const tGrassShrubs = ["medit_grass_shrubs", "medit_grass_flowers"];
const tGrassRock = ["medit_rocks_grass"];
const tDirt = "medit_dirt";
const tDirtCliff = "medit_cliff_italia";
const tGrassCliff = "medit_cliff_italia_grass";
const tCliff = ["medit_cliff_italia", "medit_cliff_italia", "medit_cliff_italia_grass"];
const tForestFloor = "medit_grass_wild";

const oBeech = "gaia/flora_tree_euro_beech";
const oBerryBush = "gaia/flora_bush_berry";
const oCarob = "gaia/flora_tree_carob";
const oCypress1 = "gaia/flora_tree_cypress";
const oCypress2 = "gaia/flora_tree_cypress";
const oLombardyPoplar = "gaia/flora_tree_poplar_lombardy";
const oPalm = "gaia/flora_tree_medit_fan_palm";
const oPine = "gaia/flora_tree_aleppo_pine";
const oDeer = "gaia/fauna_deer";
const oFish = "gaia/fauna_fish";
const oSheep = "gaia/fauna_sheep";
const oStoneLarge = "gaia/geology_stonemine_medit_quarry";
const oStoneSmall = "gaia/geology_stone_mediterranean";
const oMetalLarge = "gaia/geology_metal_mediterranean_slabs";

const aBushMedDry = "actor|props/flora/bush_medit_me_dry.xml";
const aBushMed = "actor|props/flora/bush_medit_me.xml";
const aBushSmall = "actor|props/flora/bush_medit_sm.xml";
const aBushSmallDry = "actor|props/flora/bush_medit_sm_dry.xml";
const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
const aGrassDry = "actor|props/flora/grass_soft_dry_large_tall.xml";
const aRockLarge = "actor|geology/stone_granite_large.xml";
const aRockMed = "actor|geology/stone_granite_med.xml";
const aRockSmall = "actor|geology/stone_granite_small.xml";

const pPalmForest = [tForestFloor+TERRAIN_SEPARATOR+oPalm, tGrass];
const pPineForest = [tForestFloor+TERRAIN_SEPARATOR+oPine, tGrass];
const pPoplarForest = [tForestFloor+TERRAIN_SEPARATOR+oLombardyPoplar, tGrass];
const pMainForest = [tForestFloor+TERRAIN_SEPARATOR+oCarob, tForestFloor+TERRAIN_SEPARATOR+oBeech, tGrass, tGrass];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();
const mapCenter = getMapCenter();

var clWater = createTileClass();
var clCliff = createTileClass();
var clForest = createTileClass();
var clMetal = createTileClass();
var clRock = createTileClass();
var clFood = createTileClass();
var clPlayer = createTileClass();
var clBaseResource = createTileClass();

log("Creating players...");
var [playerIDs, playerPosition] = playerPlacementLine(false, mapCenter, fractionToTiles(randFloat(0.42, 0.46)));

function distanceToPlayers(x, z)
{
	var r = 10000;
	for (let i = 0; i < numPlayers; ++i)
	{
		var dx = x - playerPosition[i].x;
		var dz = z - playerPosition[i].y;
		r = Math.min(r, Math.square(dx) + Math.square(dz));
	}
	return Math.sqrt(r);
}

function playerNearness(x, z)
{
	var d = fractionToTiles(distanceToPlayers(x,z));

	if (d < 13)
		return 0;

	if (d < 19)
		return (d-13)/(19-13);

	return 1;
}

log("Painting elevation...");

var noise0 = new Noise2D(scaleByMapSize(4, 16));
var noise1 = new Noise2D(scaleByMapSize(8, 32));
var noise2 = new Noise2D(scaleByMapSize(15, 60));

var noise2a = new Noise2D(scaleByMapSize(20, 80));
var noise2b = new Noise2D(scaleByMapSize(35, 140));

var noise3 = new Noise2D(scaleByMapSize(4, 16));
var noise4 = new Noise2D(scaleByMapSize(6, 24));
var noise5 = new Noise2D(scaleByMapSize(11, 44));

for (var ix = 0; ix <= mapSize; ix++)
	for (var iz = 0; iz <= mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
		var pn = playerNearness(x, z);

		var h = 0;
		var distToWater = 0;

		h = 32 * (x - 0.5);

		// add the rough shape of the water
		if (x < WATER_WIDTH)
			h = Math.max(-16, -28 * (WATER_WIDTH - x) / WATER_WIDTH);
		else if (x > 1.0-WATER_WIDTH)
			h = Math.max(-16, -28 * (x - (1 - WATER_WIDTH)) / WATER_WIDTH);
		else
		{
			distToWater = (0.5 - WATER_WIDTH - Math.abs(x - 0.5));
			var u = 1 - Math.abs(x - 0.5) / (0.5 - WATER_WIDTH);
			h = 12*u;
		}

		// add some base noise
		var baseNoise = 16*noise0.get(x,z) + 8*noise1.get(x,z) + 4*noise2.get(x,z) - (16+8+4)/2;
		if ( baseNoise < 0 )
		{
			baseNoise *= pn;
			baseNoise *= Math.max(0.1, distToWater / (0.5 - WATER_WIDTH));
		}
		var oldH = h;
		h += baseNoise;

		// add some higher-frequency noise on land
		if ( oldH > 0 )
			h += (0.4 * noise2a.get(x,z) + 0.2 * noise2b.get(x,z)) * Math.min(oldH / 10, 1);

		// create cliff noise
		if ( h > -10 )
		{
			var cliffNoise = (noise3.get(x,z) + 0.5*noise4.get(x,z)) / 1.5;
			if (h < 1)
			{
				var u = 1 - 0.3*((h-1)/-10);
				cliffNoise *= u;
			}
			cliffNoise += 0.05 * distToWater / (0.5 - WATER_WIDTH);
			if (cliffNoise > 0.6)
			{
				var u = 0.8 * (cliffNoise - 0.6);
				cliffNoise += u * noise5.get(x,z);
				cliffNoise /= (1 + u);
			}
			cliffNoise -= 0.59;
			cliffNoise *= pn;
			if (cliffNoise > 0)
				h += 19 * Math.min(cliffNoise, 0.045) / 0.045;
		}
		setHeight(ix, iz, h);
	}
Engine.SetProgress(15);

log("Painting terrain...");
var noise6 = new Noise2D(scaleByMapSize(10, 40));
var noise7 = new Noise2D(scaleByMapSize(20, 80));
var noise8 = new Noise2D(scaleByMapSize(13, 52));
var noise9 = new Noise2D(scaleByMapSize(26, 104));
var noise10 = new Noise2D(scaleByMapSize(50, 200));

for (var ix = 0; ix < mapSize; ix++)
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
		var pn = playerNearness(x, z);

		// get heights of surrounding vertices
		var h00 = getHeight(ix, iz);
		var h01 = getHeight(ix, iz+1);
		var h10 = getHeight(ix+1, iz);
		var h11 = getHeight(ix+1, iz+1);

		// find min and max height
		var maxH = Math.max(h00, h01, h10, h11);
		var minH = Math.min(h00, h01, h10, h11);
		var diffH = maxH - minH;

		// figure out if we're at the top of a cliff using min adjacent height
		var minAdjHeight = minH;
		if (maxH > 15)
		{
			var maxNx = Math.min(ix + 2, mapSize);
			var maxNz = Math.min(iz + 2, mapSize);
			for (let nx = Math.max(ix - 1, 0); nx <= maxNx; ++nx)
				for (let nz = Math.max(iz - 1, 0); nz <= maxNz; ++nz)
					minAdjHeight = Math.min(minAdjHeight, getHeight(nx, nz));
		}

		// choose a terrain based on elevation
		var t = tGrass;

		// water
		if (maxH < -12)
			t = tOceanDepths;
		else if (maxH < -8.8)
			t = tOceanRockDeep;
		else if (maxH < -4.7)
			t = tOceanCoral;
		else if (maxH < -2.8)
			t = tOceanRockShallow;
		else if (maxH < 0.9 && minH < 0.35)
			t = tBeachWet;
		else if (maxH < 1.5 && minH < 0.9)
			t = tBeachDry;
		else if (maxH < 2.3 && minH < 1.3)
			t = tBeachGrass;

		if (minH < 0)
			addToClass(ix, iz, clWater);

		// cliffs
		if (diffH > 2.9 && minH > -7)
		{
			t = tCliff;
			addToClass(ix, iz, clCliff);
		}
		else if (diffH > 2.5 && minH > -5 || maxH - minAdjHeight > 2.9 && minH > 0)
		{
			if (minH < -1)
				t = tCliff;
			else if (minH < 0.5)
				t = tBeachCliff;
			else
				t = [tDirtCliff, tGrassCliff, tGrassCliff, tGrassRock, tCliff];

			addToClass(ix, iz, clCliff);
		}

		// Don't place resources onto potentially impassable mountains
		if (minH >= 20)
			addToClass(ix, iz, clCliff);

		// forests
		if (getHeight(ix, iz) < 11 && diffH < 2 && minH > 1)
		{
			var forestNoise = (noise6.get(x,z) + 0.5*noise7.get(x,z)) / 1.5 * pn - 0.59;

			// Thin out trees a bit
			if (forestNoise > 0 && randBool())
			{
				if (minH < 11 && minH >= 4)
				{
					var typeNoise = noise10.get(x,z);

					if (typeNoise < 0.43 && forestNoise < 0.05)
						t = pPoplarForest;
					else if (typeNoise < 0.63)
						t = pMainForest;
					else
						t = pPineForest;

					addToClass(ix, iz, clForest);
				}
				else if (minH < 4)
				{
					t = pPalmForest;
					addToClass(ix, iz, clForest);
				}
			}
		}

		// grass variations
		if (t == tGrass)
		{
			var grassNoise = (noise8.get(x,z) + 0.6*noise9.get(x,z)) / 1.6;
			if (grassNoise < 0.3)
				t = (diffH > 1.2) ? tDirtCliff : tDirt;
			else if (grassNoise < 0.34)
			{
				t = (diffH > 1.2) ? tGrassCliff : tGrassDry;
				if (diffH < 0.5 && randBool(0.02))
					placeObject(randFloat(ix, ix + 1), randFloat(iz, iz + 1), aGrassDry, 0, randomAngle());
			}
			else if (grassNoise > 0.61)
			{
				t = (diffH > 1.2 ? tGrassRock : tGrassShrubs);
			}
			else if (diffH < 0.5 && randBool(0.02))
				placeObject(randFloat(ix, ix + 1), randFloat(iz, iz + 1), aGrass, 0, randomAngle());
		}

		placeTerrain(ix, iz, t);
	}

Engine.SetProgress(30);

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"baseResourceConstraint": avoidClasses(clCliff, 4),
	"CityPatch": {
		"radius": 11,
		"outerTerrain": tGrass,
		"innerTerrain": tCity,
		"width": 4,
		"painters": [
			new SmoothElevationPainter(ELEVATION_SET, 5, 2)
		]
	},
	"Chicken": {
	},
	"Berries": {
		"template": oBerryBush,
		"distance": 9
	},
	"Mines": {
		"types": [
			{ "template": oMetalLarge },
			{ "template": oStoneLarge }
		]
	},
	"Trees": {
		"template": oPalm,
		"count": 5,
		"minDist": 10,
		"maxDist": 11
	}
	// No decoratives
});
Engine.SetProgress(40);

log("Creating bushes...");
var group = new SimpleGroup(
	[new SimpleObject(aBushSmall, 0,2, 0,2), new SimpleObject(aBushSmallDry, 0,2, 0,2),
	new SimpleObject(aBushMed, 0,1, 0,2), new SimpleObject(aBushMedDry, 0,1, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 4, clCliff, 2),
	scaleByMapSize(9, 146), 50
);
Engine.SetProgress(45);

log("Creating rocks...");
group = new SimpleGroup(
	[new SimpleObject(aRockSmall, 0,3, 0,2), new SimpleObject(aRockMed, 0,2, 0,2),
	new SimpleObject(aRockLarge, 0,1, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clCliff, 1),
	scaleByMapSize(9, 146), 50
);
Engine.SetProgress(50);

log("Creating large stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 0,2, 0,4), new SimpleObject(oStoneLarge, 1,1, 0,4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 1, clForest, 4, clPlayer, 40, clRock, 60, clMetal, 10, clCliff, 3),
	scaleByMapSize(4,16), 100
);

log("Creating small stone mines...");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroups(group, 0,
	avoidClasses(clForest, 4, clWater, 1, clPlayer, 40, clRock, 30, clMetal, 10, clCliff, 3),
	scaleByMapSize(4,16), 100
);
log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,2)], true, clMetal);
createObjectGroups(group, 0,
	avoidClasses(clForest, 4, clWater, 1, clPlayer, 40, clMetal, 50, clCliff, 3),
	scaleByMapSize(4,16), 100
);
Engine.SetProgress(60);

createStragglerTrees(
	[oCarob, oBeech, oLombardyPoplar, oLombardyPoplar, oPine],
	avoidClasses(clWater, 5, clCliff, 4, clForest, 2, clPlayer, 15, clMetal, 6, clRock, 6),
	clForest,
	scaleByMapSize(10, 190));

Engine.SetProgress(70);

log("Creating straggler cypresses...");
group = new SimpleGroup(
	[new SimpleObject(oCypress2, 1,3, 0,3), new SimpleObject(oCypress1, 0,2, 0,2)],
	true
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 5, clCliff, 4, clForest, 2, clPlayer, 15, clMetal, 6, clRock, 6),
	scaleByMapSize(5, 75), 50
);
Engine.SetProgress(80);

log("Creating sheep...");
group = new SimpleGroup([new SimpleObject(oSheep, 2,4, 0,2)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 5, clForest, 2, clCliff, 1, clPlayer, 20, clMetal, 6, clRock, 6, clFood, 8),
	3 * numPlayers, 50
);
Engine.SetProgress(85);

log("Creating fish...");
var num = scaleByMapSize(4, 16);
var offsetX = mapSize * WATER_WIDTH/2;
for (let i = 0; i < num; ++i)
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oFish, 1, 1, 0, 1)],
			true,
			clFood,
			randIntInclusive(offsetX / 2, offsetX * 3/2),
			Math.round((i + 0.5) * mapSize / num)),
		0);

for (let i = 0; i < num; ++i)
	createObjectGroup(
		new SimpleGroup(
			[new SimpleObject(oFish, 1, 1, 0, 1)],
			true,
			clFood,
			randIntInclusive(mapSize - offsetX * 3/2, mapSize - offsetX / 2),
			Math.round((i + 0.5) * mapSize / num)),
		0);

Engine.SetProgress(90);

log("Creating deer...");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 5, clForest, 2, clCliff, 1, clPlayer, 20, clMetal, 6, clRock, 6, clFood, 8),
	3 * numPlayers, 50
);
Engine.SetProgress(95);

log("Creating berry bushes...");
group = new SimpleGroup([new SimpleObject(oBerryBush, 5,7, 0,3)], true, clFood);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 5, clForest, 2, clCliff, 1, clPlayer, 20, clMetal, 6, clRock, 6, clFood, 8),
	1.5 * numPlayers, 100
);

placePlayersNomad(clPlayer, avoidClasses(clWater, 4, clCliff, 2, clForest, 1, clMetal, 4, clRock, 4, clFood, 2));

setSkySet("sunny");
setWaterColor(0.024,0.262,0.224);
setWaterTint(0.133, 0.325,0.255);
setWaterWaviness(2.5);
setWaterType("ocean");
setWaterMurkiness(0.8);

ExportMap();
