Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

function* GenerateMap()
{
	const tPrimary = "temp_grass_long";
	const tGrass = ["temp_grass", "temp_grass", "temp_grass_d"];
	const tGrassDForest = "temp_plants_bog";
	const tGrassA = "temp_grass_plants";
	const tGrassB = "temp_plants_bog";
	const tGrassC = "temp_mud_a";
	const tHill = ["temp_highlands", "temp_grass_long_b"];
	const tCliff = ["temp_cliff_a", "temp_cliff_b"];
	const tRoad = "temp_road";
	const tRoadWild = "temp_road_overgrown";
	const tGrassPatchBlend = "temp_grass_long_b";
	const tGrassPatch = ["temp_grass_d", "temp_grass_clovers"];
	const tShore = "temp_dirt_gravel";
	const tWater = "temp_dirt_gravel_b";

	const oBeech = "gaia/tree/euro_beech";
	const oPoplar = "gaia/tree/poplar";
	const oApple = "gaia/fruit/apple";
	const oOak = "gaia/tree/oak";
	const oBerryBush = "gaia/fruit/berry_01";
	const oDeer = "gaia/fauna_deer";
	const oFish = "gaia/fish/generic";
	const oGoat = "gaia/fauna_goat";
	const oBoar = "gaia/fauna_boar";
	const oStoneLarge = "gaia/rock/temperate_large";
	const oStoneSmall = "gaia/rock/temperate_small";
	const oMetalLarge = "gaia/ore/temperate_large";

	const aGrass = "actor|props/flora/grass_soft_large_tall.xml";
	const aGrassShort = "actor|props/flora/grass_soft_large.xml";
	const aRockLarge = "actor|geology/stone_granite_large.xml";
	const aRockMedium = "actor|geology/stone_granite_med.xml";
	const aBushMedium = "actor|props/flora/bush_medit_me_lush.xml";
	const aBushSmall = "actor|props/flora/bush_medit_sm_lush.xml";
	const aReeds = "actor|props/flora/reeds_pond_lush_a.xml";
	const aLillies = "actor|props/flora/water_lillies.xml";

	const pForestD = [tGrassDForest + TERRAIN_SEPARATOR + oBeech, tGrassDForest];

	const heightSeaGround = -4;
	const heightShore = 1;
	const heightLand = 3;

	globalThis.g_Map = new RandomMap(heightShore, tPrimary);

	const numPlayers = getNumPlayers();
	const mapCenter = g_Map.getCenter();
	const mapBounds = g_Map.getBounds();

	const clPlayer = g_Map.createTileClass();
	const clHill = g_Map.createTileClass();
	const clForest = g_Map.createTileClass();
	const clWater = g_Map.createTileClass();
	const clDirt = g_Map.createTileClass();
	const clRock = g_Map.createTileClass();
	const clMetal = g_Map.createTileClass();
	const clFood = g_Map.createTileClass();
	const clBaseResource = g_Map.createTileClass();
	const clShallow = g_Map.createTileClass();

	const startAngle = randomAngle();

	placePlayerBases({
		"PlayerPlacement": playerPlacementRiver(startAngle + Math.PI / 2, fractionToTiles(0.6)),
		"PlayerTileClass": clPlayer,
		"BaseResourceClass": clBaseResource,
		"CityPatch": {
			"outerTerrain": tRoadWild,
			"innerTerrain": tRoad
		},
		"StartingAnimal": {
		},
		"Berries": {
			"template": oBerryBush
		},
		"Mines": {
			"types": [
				{ "template": oMetalLarge },
				{ "template": oStoneLarge }
			]
		},
		"Trees": {
			"template": oOak,
			"count": 2
		},
		"Decoratives": {
			"template": aGrassShort
		}
	});
	yield 10;

	paintRiver({
		"parallel": false,
		"start": new Vector2D(mapBounds.left, mapCenter.y).rotateAround(startAngle, mapCenter),
		"end": new Vector2D(mapBounds.right, mapCenter.y).rotateAround(startAngle, mapCenter),
		"width": fractionToTiles(0.25),
		"fadeDist": scaleByMapSize(3, 10),
		"deviation": 0,
		"heightRiverbed": heightSeaGround,
		"heightLand": heightLand,
		"meanderShort": 20,
		"meanderLong": 0,
		"waterFunc": (position, height, riverFraction) => {
			createTerrain(height < -1.5 ? tWater : tShore).place(position);
		},
		"landFunc": (position, shoreDist1, shoreDist2) => {
			g_Map.setHeight(position, heightLand + 0.1);
		}
	});

	yield 20;

	createTributaryRivers(
		startAngle,
		randIntInclusive(9, scaleByMapSize(13, 21)),
		scaleByMapSize(10, 20),
		heightSeaGround,
		[-6, -1.5],
		Math.PI / 5,
		clWater,
		clShallow,
		avoidClasses(clPlayer, 8, clBaseResource, 4));

	paintTerrainBasedOnHeight(-5, 1, 1, tWater);
	paintTerrainBasedOnHeight(1, heightLand, 1, tShore);
	paintTileClassBasedOnHeight(-6, 0.5, 1, clWater);
	yield 25;

	createBumps(avoidClasses(clWater, 5, clPlayer, 20));
	yield 30;

	createHills([tCliff, tCliff, tHill], avoidClasses(clPlayer, 20, clHill, 15, clWater, 5), clHill,
		scaleByMapSize(1, 4) * numPlayers);
	yield 50;

	const [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
	createForests(
		[tGrass, tGrassDForest, tGrassDForest, pForestD, pForestD],
		avoidClasses(clPlayer, 20, clForest, 17, clHill, 0, clWater, 6),
		clForest,
		forestTrees);
	yield 70;

	g_Map.log("Creating dirt patches");
	createLayeredPatches(
		[scaleByMapSize(3, 6), scaleByMapSize(5, 10), scaleByMapSize(8, 21)],
		[[tGrass, tGrassA], tGrassB, [tGrassB, tGrassC]],
		[1, 1],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 6),
		scaleByMapSize(15, 45),
		clDirt);

	g_Map.log("Creating grass patches");
	createPatches(
		[scaleByMapSize(2, 4), scaleByMapSize(3, 7), scaleByMapSize(5, 15)],
		[tGrassPatchBlend, tGrassPatch],
		[1],
		avoidClasses(clWater, 1, clForest, 0, clHill, 0, clDirt, 5, clPlayer, 6),
		scaleByMapSize(15, 45),
		clDirt);
	yield 80;

	g_Map.log("Creating stone mines");
	createMines(
		[
			[
				new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1),
				new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)
			],
			[new SimpleObject(oStoneSmall, 2, 5, 1, 3)]
		],
		avoidClasses(clWater, 2, clForest, 1, clPlayer, 20, clRock, 10, clHill, 2),
		clRock);

	g_Map.log("Creating metal mines");
	createMines(
		[
			[new SimpleObject(oMetalLarge, 1, 1, 0, 4)]
		],
		avoidClasses(clWater, 2, clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 2),
		clMetal
	);
	yield 85;

	createDecoration(
		[
			[new SimpleObject(aRockMedium, 1, 3, 0, 1)],
			[new SimpleObject(aRockLarge, 1, 2, 0, 1), new SimpleObject(aRockMedium, 1, 3, 0, 2)],
			[new SimpleObject(aGrassShort, 1, 2, 0, 1)],
			[new SimpleObject(aGrass, 2, 4, 0, 1.8), new SimpleObject(aGrassShort, 3, 6, 1.2, 2.5)],
			[new SimpleObject(aBushMedium, 1, 2, 0, 2), new SimpleObject(aBushSmall, 2, 4, 0, 2)]
		],
		[
			scaleByMapAreaAbsolute(16),
			scaleByMapAreaAbsolute(8),
			scaleByMapAreaAbsolute(13),
			scaleByMapAreaAbsolute(13),
			scaleByMapAreaAbsolute(13)
		],
		avoidClasses(clWater, 1, clForest, 0, clPlayer, 0, clHill, 0));

	createDecoration(
		[
			[new SimpleObject(aReeds, 1, 3, 0, 1)],
			[new SimpleObject(aLillies, 1, 2, 0, 1)]
		],
		[
			scaleByMapSize(800, 12800),
			scaleByMapSize(800, 12800)
		],
		stayClasses(clShallow, 0));

	createFood(
		[
			[new SimpleObject(oDeer, 5, 7, 0, 4)],
			[new SimpleObject(oGoat, 2, 3, 0, 2)],
			[new SimpleObject(oBoar, 2, 3, 0, 2)]
		],
		[
			3 * numPlayers,
			3 * numPlayers,
			3 * numPlayers
		],
		avoidClasses(clWater, 1, clForest, 0, clPlayer, 20, clHill, 0, clFood, 15),
		clFood);

	createFood(
		[
			[new SimpleObject(oBerryBush, 5, 7, 0, 4)]
		],
		[
			randIntInclusive(1, 4) * numPlayers + 2
		],
		avoidClasses(clWater, 3, clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
		clFood);

	createFood(
		[
			[new SimpleObject(oFish, 2, 3, 0, 2)]
		],
		[scaleByMapSize(3, 25) * numPlayers],
		[avoidClasses(clFood, 6), stayClasses(clWater, 4)],
		clFood);

	createStragglerTrees(
		[oBeech, oPoplar, oApple],
		avoidClasses(clWater, 1, clForest, 1, clHill, 1, clPlayer, 8, clMetal, 6, clRock, 6),
		clForest,
		stragglerTrees);

	setSkySet("cirrus");
	setWaterColor(0.114, 0.192, 0.463);
	setWaterTint(0.255, 0.361, 0.651);
	setWaterWaviness(3.0);
	setWaterType("ocean");
	setWaterMurkiness(0.83);

	setFogThickness(0.35);
	setFogFactor(0.55);

	setPPEffect("hdr");
	setPPSaturation(0.62);
	setPPContrast(0.62);
	setPPBloom(0.37);

	placePlayersNomad(clPlayer,
		avoidClasses(clWater, 4, clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

	return g_Map;
}
