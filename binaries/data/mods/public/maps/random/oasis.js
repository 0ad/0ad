Engine.LoadLibrary("rmgen");

const tSand = ["desert_sand_dunes_100", "desert_dirt_cracks","desert_sand_smooth", "desert_dirt_rough", "desert_dirt_rough_2", "desert_sand_smooth"];
const tDune = ["desert_sand_dunes_50"];
const tForestFloor = "desert_forestfloor_palms";
const tDirt = ["desert_dirt_rough","desert_dirt_rough","desert_dirt_rough", "desert_dirt_rough_2", "desert_dirt_rocks_2"];
const tRoad = "desert_city_tile";;
const tRoadWild = "desert_city_tile";;
const tShoreBlend = "desert_sand_wet";
const tShore = "dirta";
const tWater = "desert_sand_wet";

const ePalmShort = "gaia/flora_tree_cretan_date_palm_short";
const ePalmTall = "gaia/flora_tree_cretan_date_palm_tall";
const eBush = "gaia/flora_bush_grapes";
const eCamel = "gaia/fauna_camel";
const eGazelle = "gaia/fauna_gazelle";
const eLion = "gaia/fauna_lion";
const eLioness = "gaia/fauna_lioness";
const eStoneMine = "gaia/geology_stonemine_desert_quarry";
const eMetalMine = "gaia/geology_metal_desert_slabs";

const aFlower1 = "actor|props/flora/decals_flowers_daisies.xml";
const aWaterFlower = "actor|props/flora/water_lillies.xml";
const aReedsA = "actor|props/flora/reeds_pond_lush_a.xml";
const aReedsB = "actor|props/flora/reeds_pond_lush_b.xml";
const aRock = "actor|geology/stone_desert_med.xml";
const aBushA = "actor|props/flora/bush_desert_dry_a.xml";
const aBushB = "actor|props/flora/bush_desert_dry_a.xml";
const aSand = "actor|particle/blowing_sand.xml";

const pForestMain = [tForestFloor + TERRAIN_SEPARATOR + ePalmShort, tForestFloor + TERRAIN_SEPARATOR + ePalmTall, tForestFloor];
const pOasisForestLight = [tForestFloor + TERRAIN_SEPARATOR + ePalmShort, tForestFloor + TERRAIN_SEPARATOR + ePalmTall, tForestFloor,tForestFloor,tForestFloor
					,tForestFloor,tForestFloor,tForestFloor,tForestFloor];

InitMap();

const numPlayers = getNumPlayers();
const mapSize = getMapSize();

var clPlayer = createTileClass();
var clHill = createTileClass();
var clForest = createTileClass();
var clWater = createTileClass();
var clPassage = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clBaseResource = createTileClass();

initTerrain(tSand);

var [playerIDs, playerX, playerZ] = radialPlayerPlacement();

var placer = undefined;
var fx = 0; var fz = 0;
var ix =0; var iz = 0;
for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");

	var radius = scaleByMapSize(15,25);
	var elevation = 20;

	// get the x and z in tiles
	fx = fractionToTiles(playerX[i]);
	fz = fractionToTiles(playerZ[i]);
	ix = Math.round(fx);
	iz = Math.round(fz);
	addCivicCenterAreaToClass(ix, iz, clPlayer);

	// create the city patch
	var cityRadius = radius/3;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([tRoadWild, tRoad], [1]);
	createArea(placer, painter, null);

	placeCivDefaultEntities(fx, fz, id);

	placeDefaultChicken(fx, fz, clBaseResource);

	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 12;
	var bbX = Math.round(fx + bbDist * cos(bbAngle));
	var bbZ = Math.round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(eBush, 5,5, 0,3)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);

	// create metal mine
	var mAngle = bbAngle;
	while (Math.abs(mAngle - bbAngle) < Math.PI / 3)
	{
		mAngle = randFloat(0, TWO_PI);
	}
	var mDist = radius*1.3;
	var mX = Math.round(fx + mDist * cos(mAngle));
	var mZ = Math.round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(eMetalMine, 1,1, 0,0),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3),new SimpleObject(ePalmShort, 2,2, 2,3),new SimpleObject(ePalmTall, 1,1, 2,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	mX = Math.round(fx + mDist*1.5 * cos(mAngle + PI/1.578));
	mZ = Math.round(fz + mDist*1.5 * sin(mAngle + PI/1.578));
	group = new SimpleGroup(
							[new SimpleObject(eMetalMine, 1,1, 0,0),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3),new SimpleObject(ePalmShort, 2,2, 2,3),new SimpleObject(ePalmTall, 1,1, 2,2)],
							true, clBaseResource, mX, mZ
							);
	createObjectGroup(group, 0);

	// create stone mines
	mAngle += randFloat(PI/8, PI/4);
	mX = Math.round(fx + mDist * cos(mAngle));
	mZ = Math.round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(eStoneMine, 1,1, 0,2),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3),new SimpleObject(ePalmShort, 2,2, 2,3),new SimpleObject(ePalmTall, 1,1, 2,2)],
		true, clBaseResource, mX, mZ
	);
	createObjectGroup(group, 0);
	mX = Math.round(fx + mDist * 1.4 * cos(mAngle - PI /2.46));
	mZ = Math.round(fz + mDist * 1.4 * sin(mAngle - PI /2.46));
	group = new SimpleGroup(
							[new SimpleObject(eStoneMine, 1,1, 0,2),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 3,3),new SimpleObject(ePalmShort, 2,2, 3,3),new SimpleObject(ePalmTall, 1,1, 3,3)],
							true, clBaseResource, mX, mZ
							);
	createObjectGroup(group, 0);
}

log("Creating small oasis near the players...")
var forestDist = scaleByMapSize(15, 25) * 1.2;
for (let i = 0; i < numPlayers; ++i)
{
	let fx = fractionToTiles(playerX[i]);
	let fz = fractionToTiles(playerZ[i]);

	// Create starting batches of wood
	let forestX = 0;
	let forestY = 0;
	let forestAngle = 0

	do {
		forestAngle = Math.PI / 3 * randFloat(1, 2);
		forestX = Math.round(fx + forestDist * Math.cos(forestAngle));
		forestY = Math.round(fz + forestDist * Math.sin(forestAngle));
	} while (
		!createArea(
			new ClumpPlacer(70, 1, 0.5, 10, forestX, forestY),
			[
				new LayeredPainter([tForestFloor, pForestMain], [0]),
				paintClass(clBaseResource)
			],
			avoidClasses(clBaseResource, 0)));

	// Creating the water patch explaining the forest
	do {
		var watAngle = forestAngle + randFloat((PI/3), (5*PI/3));
		var watX = Math.round(forestX + 6 * cos(watAngle));
		var watY = Math.round(forestY + 6 * sin(watAngle));

		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(aFlower1, 1, 5, 0, 3)],
				true,
				undefined,
				Math.round(forestX + 3 * Math.cos(watAngle)),
				Math.round(forestY + 3 * Math.sin(watAngle))),
			0);

		createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(aReedsA, 1, 3, 0, 0)],
				true,
				undefined,
				Math.round(forestX + 5 * Math.cos(watAngle)),
				Math.round(forestY + 5 * Math.sin(watAngle))),
			0);

	} while (
		!createArea(
			new ClumpPlacer(60, 0.9, 0.4, 5, watX, watY),
			[
				new LayeredPainter([tShore, tShoreBlend], [1]),
				new SmoothElevationPainter(ELEVATION_MODIFY, -5, 3)
			],
			avoidClasses(clBaseResource, 0)));
}
Engine.SetProgress(20);

log("Creating bumps...");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, 1),
	new SmoothElevationPainter(ELEVATION_MODIFY, 4, 3),
	avoidClasses(clPlayer, 10, clBaseResource, 6),
	scaleByMapSize(30, 70));

log("Creating dirt Patches...");
createAreas(
	new ClumpPlacer(80, 0.3, 0.06, 1),
	new TerrainPainter(tDirt),
	avoidClasses(clPlayer, 10, clBaseResource, 6),
	scaleByMapSize(15, 50));

log("Creating Dunes...");
createAreas(
	new ClumpPlacer(120, 0.3, 0.06, 1),
	[
		new TerrainPainter(tDune),
		new SmoothElevationPainter(ELEVATION_MODIFY, 18, 30)
	],
	avoidClasses(clPlayer, 10, clBaseResource, 6),
	scaleByMapSize(15, 50));

log("Creating actual oasis...");
var fx = fractionToTiles(0.5);
var fz = fractionToTiles(0.5);
createArea(
	new ClumpPlacer(Math.square(mapSize * 0.2) * 1.1, 0.8, 0.2, 10, Math.round(fx), Math.round(fz)),
	[
		new LayeredPainter([pOasisForestLight,tShoreBlend, tWater, tWater, tWater], [scaleByMapSize(6, 20), 3, 5, 2]),
		new SmoothElevationPainter(ELEVATION_SET, -3, 15),
		paintClass(clWater)
	],
	null);
Engine.SetProgress(50);

if (mapSize > 150 && randBool())
{
	log("Creating path though the oasis...");
	var pAngle = randFloat(0, TWO_PI);
	var px = Math.round(fx) + Math.round(fractionToTiles(0.13 * cos(pAngle)));
	var py = Math.round(fz) + Math.round(fractionToTiles(0.13 * sin(pAngle)));
	var pex = Math.round(fx) + Math.round(fractionToTiles(0.13 * -cos(pAngle)));
	var pey = Math.round(fz) + Math.round(fractionToTiles(0.13 * sin(pAngle + PI)));
	createArea(
		new PathPlacer(px, py, pex, pey, scaleByMapSize(7, 18), 0.4, 1, 0.2, 0),
		[
			new TerrainPainter(tSand),
			new SmoothElevationPainter(ELEVATION_MODIFY, 4, 5),
			paintClass(clPassage)
		],
		null);
}
log("Creating some straggler trees around the Passage...");
var group = new SimpleGroup([new SimpleObject(ePalmTall, 1,1, 0,0),new SimpleObject(ePalmShort, 1, 2, 1, 2), new SimpleObject(aBushA, 0,2, 1,3)], true, clForest);
createObjectGroupsDeprecated(group, 0, stayClasses(clPassage, 1), scaleByMapSize(60, 250), 100);

log("Creating stone mines...");
group = new SimpleGroup([new SimpleObject(eStoneMine, 1,1, 0,0),new SimpleObject(ePalmShort, 1,2, 3,3),new SimpleObject(ePalmTall, 0,1, 3,3)
						 ,new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 10, clForest, 1, clPlayer, 30, clRock, 10,clBaseResource, 2, clHill, 1),
	scaleByMapSize(6,25), 100
);

log("Creating metal mines...");
group = new SimpleGroup([new SimpleObject(eMetalMine, 1,1, 0,0),new SimpleObject(ePalmShort, 1,2, 2,3),new SimpleObject(ePalmTall, 0,1, 2,2)
						 ,new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 10, clForest, 1, clPlayer, 30, clMetal, 10,clBaseResource, 2, clRock, 10, clHill, 1),
	scaleByMapSize(6,25), 100
);
Engine.SetProgress(65);

log("Creating small decorative rocks...");
group = new SimpleGroup( [new SimpleObject(aRock, 2,4, 0,2)], true, undefined );
createObjectGroupsDeprecated(group, 0, avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20), 30, scaleByMapSize(10,50) );

Engine.SetProgress(70);

log("Creating Camels...");
group = new SimpleGroup(
	[new SimpleObject(eCamel, 1,2, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	1 * numPlayers, 50
);
Engine.SetProgress(75);

log("Creating Gazelles...");
group = new SimpleGroup(
	[new SimpleObject(eGazelle, 2,4, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 3, clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	1 * numPlayers, 50
);
Engine.SetProgress(85);

log("Creating Oasis Animals...");
for (var p = 0; p < scaleByMapSize(5,30); p++)
{
	var aAngle = randFloat(0, TWO_PI);
	var aDist = fractionToTiles(0.11);
	var animX = Math.round(fx + aDist * cos(aAngle));
	var animY = Math.round(fz + aDist * sin(aAngle));
	group = new RandomGroup(
		[new SimpleObject(eLion, 1,2, 0,4),new SimpleObject(eLioness, 1,2, 2,4),new SimpleObject(eGazelle, 4,6, 1,5),new SimpleObject(eCamel, 1,2, 1,5)], true, clFood, animX,animY);
	createObjectGroup(group, 0);
}
Engine.SetProgress(90);

log("Creating bushes...");
group = new SimpleGroup(
	[new SimpleObject(aBushB, 1,2, 0,2), new SimpleObject(aBushA, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clWater, 2, clHill, 1, clPlayer, 1, clPassage, 1),
	scaleByMapSize(10, 40), 20
);
log ("Creating Sand blows and beautifications");
for (var sandx = 0; sandx < mapSize; sandx += 4)
	for (var sandz = 0; sandz < mapSize; sandz += 4)
	{
		if (getHeight(sandx,sandz) > 3.4)
		{
			if (randBool((getHeight(sandx,sandz) - 3.4) / 1.4))
			{
				group = new SimpleGroup( [new SimpleObject(aSand, 0,1, 0,2)], true, undefined, sandx,sandz );
				createObjectGroup(group, 0);
			}
		}
		else if (getHeight(sandx, sandz) > -2.5 && getHeight(sandx,sandz) < -1)
		{
			if (randBool(0.4))
			{
				group = new SimpleGroup( [new SimpleObject(aWaterFlower, 1,4, 1,2)], true, undefined, sandx,sandz );
				createObjectGroup(group, 0);
			}
			else if (randBool(0.7) && getHeight(sandx,sandz) < -1.9)
			{
				group = new SimpleGroup( [new SimpleObject(aReedsA, 5,12, 0,2),new SimpleObject(aReedsB, 5,12, 0,2)], true, undefined, sandx,sandz );
				createObjectGroup(group, 0);
			}

			if (getTileClass(clPassage).countInRadius(sandx,sandz,2,true) > 0) {
				if (randBool(0.4))
				{
					group = new SimpleGroup( [new SimpleObject(aWaterFlower, 1,4, 1,2)], true, undefined, sandx,sandz );
					createObjectGroup(group, 0);
				}
				else if (randBool(0.7) && getHeight(sandx,sandz) < -1.9)
				{
					group = new SimpleGroup( [new SimpleObject(aReedsA, 5,12, 0,2),new SimpleObject(aReedsB, 5,12, 0,2)], true, undefined, sandx,sandz );
					createObjectGroup(group, 0);
				}
			}
		}
	}

setSkySet("sunny");
setSunColor(0.914,0.827,0.639);
setSunRotation(PI/3);
setSunElevation(0.5);
setWaterColor(0, 0.227, 0.843);
setWaterTint(0, 0.545, 0.859);
setWaterWaviness(1.0);
setWaterType("clap");
setWaterMurkiness(0.5);
setTerrainAmbientColor(0.45, 0.5, 0.6);
setUnitsAmbientColor(0.501961, 0.501961, 0.501961);

ExportMap();
