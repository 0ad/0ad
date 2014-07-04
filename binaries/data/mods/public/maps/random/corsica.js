RMS.LoadLibrary("rmgen");

var tGrass = ["medit_grass_field", "medit_grass_field_b", "temp_grass_c"];
var tDune = ["medit_grass_field_brown"];
var tBigDune = ["medit_grass_field_brown"];
var tGrassPForest = "forestfloor_dirty";
var tForestFloor = "medit_grass_shrubs";
var tGrassA = ["desert_dirt_persia_1", "desert_dirt_persia_2"];
var tGrassB = "dirta";
var tLushGrass = ["medit_grass_field","medit_grass_field_a"];

var tSteepCliffs = ["temp_cliff_b", "temp_cliff_a"];
var tCliffs = ["temp_cliff_b", "medit_cliff_italia", "medit_cliff_italia_grass"];
var tHill = ["medit_cliff_italia_grass","medit_cliff_italia_grass", "medit_grass_field", "medit_grass_field", "temp_grass"];
var tMountain = ["medit_cliff_italia_grass","medit_cliff_italia"];
var tMountainTop = ["medit_cliff_italia"];

var tDirt = ["medit_dirt", "medit_dirt_b"];
var tRoad = ["medit_city_tile","medit_rocks_grass","medit_grass_field_b"];
var tRoadWild = ["medit_rocks_grass","medit_grass_field_b"];
var tGrassPatch = "medit_dirt_b";

var tShoreBlend = ["medit_sand_wet","medit_rocks_wet"];
var tShore = ["medit_rocks","medit_sand","medit_sand"];
var tSandTransition = ["medit_sand","medit_rocks_grass","medit_rocks_grass","medit_rocks_grass"]
var tVeryDeepWater = ["medit_sea_depths","medit_sea_coral_deep"];
var tDeepWater = ["medit_sea_coral_deep","tropic_ocean_coral"];
var tCreekWater = "medit_sea_coral_plants";

// gaia entities
var ePine = "gaia/flora_tree_aleppo_pine";
var ePalmTall = "gaia/flora_tree_cretan_date_palm_tall";
var eFanPalm = "gaia/flora_tree_medit_fan_palm";
var eCypress = "gaia/flora_tree_cypress";
var eApple = "gaia/flora_tree_apple"
var eBush = "gaia/flora_bush_berry";
var eChicken = "gaia/fauna_chicken";
var eFish = "gaia/fauna_fish";
var ePig = "gaia/fauna_pig";
var eStoneMine = "gaia/geology_stonemine_medit_quarry";
var eMetalMine = "gaia/geology_metal_mediterranean_slabs";

// decorative props
var aFlower1 = "actor|props/flora/decals_flowers_daisies.xml";
var aWaterFlower = "actor|props/flora/water_lillies.xml";
var aReedsA = "actor|props/flora/reeds_pond_lush_a.xml";
var aReedsB = "actor|props/flora/reeds_pond_lush_b.xml";
var aRock = "actor|geology/stone_granite_med.xml";
var aLargeRock = "actor|geology/stone_granite_large.xml";
var aBushA = "actor|props/flora/bush_medit_sm_lush.xml";
var aBushB = "actor|props/flora/bush_medit_me_lush.xml";
var aPlantA = "actor|props/flora/plant_medit_artichoke.xml";
var aPlantB = "actor|props/flora/grass_tufts_a.xml";
var aPlantC = "actor|props/flora/grass_soft_tuft_a.xml";
var aShorePlantA = "actor|props/flora/reeds_beach.xml";
var aShorePlantB = "actor|props/flora/grass_temp_field_brown.xml";

var aStandingStone = "actor|props/special/eyecandy/standing_stones.xml";

var tForestNicae = [tForestFloor + TERRAIN_SEPARATOR + ePine,tForestFloor + TERRAIN_SEPARATOR + ePine,tForestFloor + TERRAIN_SEPARATOR + ePine,tForestFloor + TERRAIN_SEPARATOR + ePine,tForestFloor + TERRAIN_SEPARATOR + eFanPalm, tForestFloor + TERRAIN_SEPARATOR + ePalmTall, tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor];
var tForestNicaeLight = [tForestFloor + TERRAIN_SEPARATOR + ePine,tForestFloor + TERRAIN_SEPARATOR + ePine,tForestFloor + TERRAIN_SEPARATOR + eFanPalm, tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor];
var tForestNicaeScarce = [tForestFloor + TERRAIN_SEPARATOR + ePine,tForestFloor + TERRAIN_SEPARATOR + ePine,tForestFloor + TERRAIN_SEPARATOR + eFanPalm, tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor,tForestFloor];

const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();
var mapArea = mapSize*mapSize;

// create tile classes
var clCorsica = createTileClass();
var clSardinia = createTileClass();
var clCreek = createTileClass();

var clWater = createTileClass();

var clCliffs = createTileClass();
var clForest = createTileClass();
var clPeak = createTileClass();

var clShore = createTileClass();
var clPathToShore = createTileClass();
var clPlayer = createTileClass();
var clBaseResource = createTileClass();
var clPassage = createTileClass();
var clHill = createTileClass();
var clWater = createTileClass();
var clDirt = createTileClass();
var clRock = createTileClass();
var clMetal = createTileClass();
var clFood = createTileClass();
var clSettlement = createTileClass();
var clDune = createTileClass();

// on every pixel of the map, set wet sand
for (var ix = 0; ix < mapSize; ix++)
{
	for (var iz = 0; iz < mapSize; iz++)
	{
		var x = ix / (mapSize + 1.0);
		var z = iz / (mapSize + 1.0);
			placeTerrain(ix, iz, tVeryDeepWater);
		//addToClass(ix,iz,clWater);
	}
}

// let's decide if we swap
var swap = Math.round(Math.random());	// should return about 50/50 a 0 or a 1
// let's create Corsica
log("Creating Corsica");
var CorsicaX = fractionToTiles(0.99);
var CorsicaZ = fractionToTiles(0.9);
if (swap)
	CorsicaX = fractionToTiles(0.01);

// Okay so the thing here is that we'll make a sort of jagged circle. To achieve this, I'll make a few islands
// that will basically be put together
// first, let's make a big round island in the corner.
// okay so actually subdivided cleverly to make it work and give jagedness with the multiple islands
var llx = round(CorsicaX);
var llz = round(CorsicaZ);
// okay so the circle reaches close to a third of the map
var placer = new ClumpPlacer(fractionToSize(0.3)*1.8, 1.0, 0.5, 10, llx, llz);
var terrainPainter = new LayeredPainter([tCliffs, tGrass], [2] );
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 5,0);
createArea(placer, [terrainPainter, paintClass(clCorsica), elevationPainter], null);
var nbSubIsland = 5;	// actually 5+1
for (var i = 0; i <= nbSubIsland; i++)
{
	// radius is "sqrt(this.size / PI)"... so in my case it's "sqrt(fractionofSize(0.33)*2.0/PI), about 0.64, sqrt-ed
	//Let's round down.
	// only from π to 3π/2
	var angle = (i * (-PI/(nbSubIsland*2)) + PI);
	if (!swap)
		angle *= -1;
	var llx = round (CorsicaX + sqrt(fractionToSize(0.3)*0.55)*sin(angle));
	var llz = round (CorsicaZ + sqrt(fractionToSize(0.3)*0.55)*cos(angle));
	var placer = new ClumpPlacer(fractionToSize(0.05)/2, 0.6, 0.03, 10, llx, llz);
	var terrainPainter = new LayeredPainter([tCliffs, tGrass], [2] );
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 5,1);
	createArea(placer, [terrainPainter, paintClass(clCorsica), elevationPainter], null);
}
RMS.SetProgress(10);

log("Creating Sardinia");
var SardiniaX = fractionToTiles(0.01);
var SardiniaZ = fractionToTiles(0.1);
if (swap)
	SardiniaX = fractionToTiles(0.99);

var llx = round(SardiniaX);
var llz = round(SardiniaZ);
// okay so the circle reaches close to a third of the map
var placer = new ClumpPlacer(fractionToSize(0.3)*1.8, 1.0, 0.5, 10, llx, llz);
var terrainPainter = new LayeredPainter([tCliffs, tGrass], [2] );
var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 5,0);
createArea(placer, [terrainPainter, paintClass(clSardinia), elevationPainter], null);
// same as Corsica on the other side
for (var i = 0; i <= nbSubIsland; i++)
{
	var angle = (i * (-PI/(nbSubIsland*2)));
	if (!swap)
		angle *= -1;
	var llx = round (SardiniaX + sqrt(fractionToSize(0.3)*0.55)*sin(angle));
	var llz = round (SardiniaZ + sqrt(fractionToSize(0.3)*0.55)*cos(angle));
	var placer = new ClumpPlacer(fractionToSize(0.05)/2, 0.6, 0.03, 10, llx, llz);
	var terrainPainter = new LayeredPainter([tCliffs, tGrass], [2] );
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, 5,1);
	createArea(placer, [terrainPainter, paintClass(clSardinia), elevationPainter], null);
}

log("Creating Creeks");

// okay so now let's make some cleverly designed creeks: this creates a very jagged relief, looks good
var nbCreeks = scaleByMapSize(6,15);
// inCorsica first
var islandX = [SardiniaX,CorsicaX];
var islandZ = [SardiniaZ,CorsicaZ];
// first: the creeks
for (var island = 0; island <= 1; island++)
	for (var i = 0; i <= nbCreeks; i++)
	{
		var radius = fractionToTiles( (Math.random()/17) + 0.49);
		var angle = PI*island + i*(PI/(nbCreeks*2));
		if (swap)
			angle += PI/2;
		fx = radius * cos(angle);
		fz = radius * sin(angle);	
		fx = round(islandX[island] + fx);
		fz = round(islandZ[island] + fz);
		var size = scaleByMapSize(75,100);
		if (Math.random() > 0.5)
			size = Math.random() * 40 + 10;
		else
			size += Math.random() * 20;
		var placer = new ClumpPlacer(size, 0.4, 0.01, 10, fx,fz);
		var terrainPainter = new TerrainPainter(tSteepCliffs);
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -5,0);	// base height is -10
		createArea(placer, [terrainPainter, paintClass(clCreek), elevationPainter], null);
	}

var nbBeaches = scaleByMapSize(2,5);
for (var island = 0; island <= 1; island++)
{
	for (var i = 0; i <= nbBeaches; i++)
	{
		var smallRadius = fractionToTiles( 0.45);
		var bigRadius = fractionToTiles( 0.57);
		var angle = PI*island + i*(PI/(nbBeaches*2.5)) + PI/(nbBeaches*6) + randFloat(-PI/(nbBeaches*7),PI/(nbBeaches*7));
		if (swap)
			angle += PI/2;
		var startX = smallRadius * cos(angle);
		var startZ = smallRadius * sin(angle);	
		startX = round(islandX[island] + startX);
		startZ = round(islandZ[island] + startZ);
		
		var endX = bigRadius * cos(angle);
		var endZ = bigRadius * sin(angle);	
		endX = round(islandX[island] + endX);
		endZ = round(islandZ[island] + endZ);
		
		var placer = new ClumpPlacer(130, 0.7, 0.8, 10, round((startX+endX*3)/4),round((startZ+endZ*3)/4));
		var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, -1,5);	// base height is -10
		createArea(placer, [elevationPainter], null);
		
		straightPassageMaker(startX, startZ,endX,endZ, 25, 18, 4,clShore,null);
	}
}
RMS.SetProgress(20);
log("Creating Main Relief");
// Let's make it generally cliffy
placer = new ClumpPlacer(fractionToSize(0.3)*1.8, 1.0, 0.2, 4,round((CorsicaX * 5 + fractionToTiles(0.5)) / 6.0),round(fractionToTiles(0.8)));
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 30,fractionToTiles(0.45));
createArea( placer, [elevationPainter],  null);
placer = new ClumpPlacer(fractionToSize(0.3)*1.8, 1.0, 0.2, 4,round((SardiniaX * 5 + fractionToTiles(0.5)) / 6.0),round(fractionToTiles(0.2)));
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 30,fractionToTiles(0.45));
createArea( placer, [elevationPainter],  null);

log("Creating players");

// randomize player order
var playerIDs = [];
for (var i = 0; i < numPlayers; i++)
{
	playerIDs.push(i+1);
}
playerIDs = sortPlayers(playerIDs);

// place players
var playerX = new Array(numPlayers);
var playerZ = new Array(numPlayers);
var playerAngle = new Array(numPlayers);

var island = 0;
var formerTeam = getPlayerTeam(0);
var onCorsica = [];
var onSardinia = [];
for (var o = 0; o < numPlayers; o++)
{
	if (getPlayerTeam(o) === formerTeam && formerTeam !== -1)
	{
		// same island
		if (island === 0)
			onCorsica.push(o);
		else
			onSardinia.push(o);
	} else if (getPlayerTeam(o) !== -1){
		if (island === 0)
		{
			island = 1;
			onSardinia.push(o);
		} else {
			island = 0;
			onCorsica.push(o);
		}
	} else {
		// okay now the less crowded:
		if (onCorsica.length > onSardinia.length)
			onSardinia.push(o);
		else
			onCorsica.push(o);
	}
	formerTeam = getPlayerTeam(o);
}
// le'ts place the players in a circle around the island.
for (var i = 0;i < onCorsica.length; i++)
{
	var angle = (i * (PI/(onCorsica.length*2)) + PI + PI/(4*onCorsica.length));
	if (swap)
		angle += PI/2;
	playerAngle[onCorsica[i]] = angle;
	playerX[onCorsica[i]] = round( CorsicaX + fractionToTiles(0.36*cos(angle)));
	playerZ[onCorsica[i]] = round( fractionToTiles(1 + 0.36*sin(angle)));
}
for (var i = 0;i < onSardinia.length; i++)
{
	var angle = (i * (PI/(onSardinia.length*2)) + PI/(4*onSardinia.length));
	if (swap)
		angle += PI/2;
	playerAngle[onSardinia[i]] = angle;
	playerX[onSardinia[i]] = round( SardiniaX + fractionToTiles(0.36*cos(angle)));
	playerZ[onSardinia[i]] = round( fractionToTiles(0 + 0.36*sin(angle)));
}

var placer = undefined;
var fx = 0; var fz = 0;
var ix =0; var iz = 0;
for (var i = 0; i < numPlayers; i++)
{
	var id = playerIDs[i];
	log("Creating base for player " + id + "...");
	
	// some constants
	var radius = 23;
	
	// get the x and z in tiles
	fx = playerX[i];
	fz = playerZ[i];
	
	
	// let's create a nice platform
	var placer = new ClumpPlacer(PI*radius*radius, 0.95, 0.3, 10, fx,fz);
	var PlayerArea = createArea(placer, [paintClass(clPlayer)], null);

	// create the city patch
	var cityRadius = radius/4;
	placer = new ClumpPlacer(PI*cityRadius*cityRadius, 0.8, 0.3, 10, fx, fz);
	var painter = new LayeredPainter([tRoadWild,tRoad],[1]);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, getHeight(fx,fz),10);
	createArea(placer, [painter,paintClass(clSettlement),elevationPainter], null);
	
	// create starting units
	placeCivDefaultEntities(fx, fz, id, BUILDING_ANGlE, {'iberWall' : false});
	
	// create animals
	for (var j = 0; j < 2; ++j)
	{
		var aAngle = randFloat(0, TWO_PI);
		var aDist = 7;
		var aX = round(fx + aDist * cos(aAngle));
		var aZ = round(fz + aDist * sin(aAngle));
		var group = new SimpleGroup(
			[new SimpleObject(eChicken, 5,5, 0,2)],
			true, clBaseResource, aX, aZ
		);
		createObjectGroup(group, 0);
	}
	
	// create berry bushes
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 11;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
		[new SimpleObject(eBush, 5,5, 1,2)],
		true, clBaseResource, bbX, bbZ
	);
	createObjectGroup(group, 0);
	
	// create metal mine
	// this makes sure it's created on the same level as the player.
	var mAngle = randFloat(playerAngle[i] + PI/2,playerAngle[i] + PI/3);
	var mDist = 18;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(eMetalMine, 1,1, 0,0),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3), new SimpleObject(ePine, 0,1, 3,3)], true, clBaseResource, mX, mZ );
	createObjectGroup(group, 0);
	// create stone mines
	mAngle += randFloat(PI/8, PI/5);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(eStoneMine, 1,1, 0,2),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3), new SimpleObject(ePine, 0,1, 3,3)], true, clBaseResource, mX, mZ );
	createObjectGroup(group, 0);
	
	group = new SimpleGroup([new SimpleObject(ePine, 1,3, 1,4),new SimpleObject(ePalmTall, 0,1, 1,4),new SimpleObject(eFanPalm, 0,1, 0,2)], true, clForest);
	createObjectGroups(group, 0, [avoidClasses(clBaseResource,3, clSettlement,0), stayClasses(clPlayer,1)], 150, 1000);
}

RMS.SetProgress(40);
log ("making plateaux");
// Corsica and Sardinia

var SardX = round((SardiniaX*5 + fractionToTiles(0.5))/6.0);
var SardZ = round(fractionToTiles(0.1));
var CorsX = round((CorsicaX*5 + fractionToTiles(0.5))/6.0);
var CorsZ = round(fractionToTiles(0.9));
// first level plateaux, puts the player higher
placer = new ClumpPlacer(fractionToSize(0.18)*1.8, 0.95, 0.02, 4,CorsX,CorsZ);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 9,1);
createArea( placer, [elevationPainter],  null);
placer = new ClumpPlacer(fractionToSize(0.18)*1.8, 0.95, 0.02, 4,SardX,SardZ);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 9,1);
createArea( placer, [elevationPainter],  null);
// second level plateaux, top of the hill
if(mapSize > 150)
{
	placer = new ClumpPlacer(fractionToSize(0.1), 0.98, 0.04, 4,CorsX,CorsZ);
	terrainPainter = new LayeredPainter([tCliffs, tGrass], [2] );
	elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 8,1);
	createArea( placer, [terrainPainter,elevationPainter],  null);
	placer = new ClumpPlacer(fractionToSize(0.1), 0.98, 0.04, 4,SardX,SardZ);
	terrainPainter = new LayeredPainter([tCliffs, tGrass], [2] );
	elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 8,1);
	createArea( placer, [terrainPainter,elevationPainter],  null);
}
log ("creating passages towards the island");
if(mapSize > 150)
{
	var nb = scaleByMapSize(1,4);
	for (var i = 0; i < nb; i++) {
		var radius = sqrt(fractionToSize(0.1)/PI);
		var angle = PI + i*(PI/(2*nb)) + PI/(4*nb);
		if (swap)
			angle += PI/2;
		var x1 = round(CorsX + (radius+5)*cos(angle));
		var y1 = round(CorsZ + (radius+5)*sin(angle));	
		var x2 = round(CorsX + (radius-4)*cos(angle));
		var y2 = round(CorsZ + (radius-4)*sin(angle));	
		straightPassageMaker(x1, y1, x2, y2, 1, 6, 2,clPassage,tGrass)
	}
	for (var i = 0; i < nb; i++) {
		var radius = sqrt(fractionToSize(0.1)/PI)+ 2;
		var angle = i*(PI/(2*nb)) + PI/(4*nb);
		if (swap)
			angle += PI/2;
		var x1 = round(SardX + (radius+5)*cos(angle));
		var y1 = round(SardZ + (radius+5)*sin(angle));	
		var x2 = round(SardX + (radius-4)*cos(angle));
		var y2 = round(SardZ + (radius-4)*sin(angle));	
		straightPassageMaker(x1, y1, x2, y2, 1, 6, 2,clPassage,tGrass)
	}
}
for (var i = 0; i <= 3; i++) {
	var radius = sqrt(fractionToSize(0.18)*1.8/PI) + 2;
	var angle = PI + i*(PI/7) + PI/9;
	if (swap)
		angle += PI/2;
	var x1 = round(CorsX + (radius+7)*cos(angle));
	var y1 = round(CorsZ + (radius+7)*sin(angle));	
	var x2 = round(CorsX + (radius-5)*cos(angle));
	var y2 = round(CorsZ + (radius-5)*sin(angle));	
	straightPassageMaker(x1, y1, x2, y2, 4, 10, 3,clPassage,tGrass)
}
for (var i = 0; i <= 3; i++) {
	var radius = sqrt(fractionToSize(0.18)*1.8/PI)+ 2;
	var angle = i*(PI/7) + PI/9;
	if (swap)
		angle += PI/2;
	var x1 = round(SardX + (radius+7)*cos(angle));
	var y1 = round(SardZ + (radius+7)*sin(angle));	
	var x2 = round(SardX + (radius-5)*cos(angle));
	var y2 = round(SardZ + (radius-5)*sin(angle));	
	straightPassageMaker(x1, y1, x2, y2, 4, 10, 3,clPassage,tGrass)
}
RMS.SetProgress(50);

log ("creating bumps");
placer = new ClumpPlacer(70, 0.6, 0.1, 4);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, 2,3);
createAreas( placer, [elevationPainter],  [avoidClasses(clPlayer,2,clPassage, 2), stayClasses(clCorsica,2)],scaleByMapSize(20,100), 5 );
createAreas( placer, [elevationPainter],  [avoidClasses(clPlayer,2,clPassage, 2), stayClasses(clSardinia,2)],scaleByMapSize(20,100), 5 );

log ("creating anti bumps");
placer = new ClumpPlacer(120, 0.3, 0.1, 4);
elevationPainter = new SmoothElevationPainter(ELEVATION_MODIFY, -5,6);
createAreas( placer, [elevationPainter],  [avoidClasses(clPlayer,2,clPassage, 2,clCorsica,2,clSardinia,2)],scaleByMapSize(20,100), 5 );


log("Repainting");
var terrTop = createTerrain(tMountainTop);
var terrMount = createTerrain(tMountain);
var terrHill = createTerrain(tHill);
var terrCliff = createTerrain(tCliffs);
var terrSteepCliff = createTerrain(tSteepCliffs);
var terrGrass = createTerrain(tGrass);

var terrShallow = createTerrain(tCreekWater);
var terrDeep = createTerrain(tDeepWater);
var terrDark = createTerrain(tVeryDeepWater);
var terrSand = createTerrain(tShore);
var terrWetSand = createTerrain(tShoreBlend);
var terrSandTransition = createTerrain(tSandTransition);
// first pass: who's water?
for (var sandx = 0; sandx < mapSize; sandx++)
	for (var sandz = 0; sandz < mapSize; sandz++)
		if (getHeight(sandx,sandz) < 0)
			addToClass(sandx,sandz,clWater);

// second pass: who's not water
for (var sandx = 0; sandx < mapSize; sandx++) {
	for (var sandz = 0; sandz < mapSize; sandz++) {
		if (getTileClass(clSettlement).countMembersInRadius(sandx,sandz,2) === 0)
		{
			var height = getHeight(sandx,sandz);
			var heightDiff = getHeightDiff(sandx,sandz);
			if (height >= 0.5 && height < 1.5 /*&& getTileClass(clWater).countMembersInRadius(sandx,sandz,3) > 0 */ && getTileClass(clShore).countMembersInRadius(sandx,sandz,2) > 0)
			{
				terrSandTransition.place(sandx,sandz);
			} else if (height >= 1 && getTileClass(clWater).countMembersInRadius(sandx,sandz,3) == 0)
			{
				// paint hills or cliffs depending on terrain elevation difference
				if (height > 17 && getTileClass(clPassage).countMembersInRadius(sandx,sandz,2) == 0)
				{
					if (heightDiff < 5)
						terrHill.place(sandx,sandz);
					else if(heightDiff < 10)
						terrMount.place(sandx,sandz);
				} else {
					terrGrass.place(sandx,sandz);
				}
				if (height > 25 && heightDiff >= 10 && getTileClass(clPassage).countMembersInRadius(sandx,sandz,2) == 0) {
					terrSteepCliff.place(sandx,sandz);
					addToClass(sandx,sandz,clCliffs);
				} else if(heightDiff >= 10 && getTileClass(clPassage).countMembersInRadius(sandx,sandz,2) == 0) {
					terrCliff.place(sandx,sandz);
					addToClass(sandx,sandz,clCliffs);
				}
			} else {
				if (height >= 0 && heightDiff >= 9) {
					terrCliff.place(sandx,sandz);
					addToClass(sandx,sandz,clCliffs);
				} else if (height >= -0.75 && height < 1.5 && heightDiff < 9) {
						terrSand.place(sandx,sandz);
				} else if (height >= -3  && height < -0.75 && heightDiff < 9) {
					terrWetSand.place(sandx,sandz);
				}  else if (height >= -6  && height < -3 && heightDiff < 9) {
					terrShallow.place(sandx,sandz);
				} else if (height > -10  && height < -6 && heightDiff < 6) {
					terrDeep.place(sandx,sandz);
				}
				if (heightDiff >= 9) {
					terrCliff.place(sandx,sandz);
					addToClass(sandx,sandz,clCliffs);
				}
			}
		}
	}
}

RMS.SetProgress(65);

log("Creating stone mines...");
// create large stone quarries
group = new SimpleGroup([new SimpleObject(eStoneMine, 1,1, 0,0),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3)], true, clBaseResource);
createObjectGroups(group, 0,[stayClasses(clCorsica, 1),avoidClasses(clWater, 3, clPlayer,2 , clBaseResource, 2,clCliffs,1)],  scaleByMapSize(6,25), 1000  );
createObjectGroups(group, 0,[stayClasses(clSardinia, 1),avoidClasses(clWater, 3, clPlayer,2 , clBaseResource, 2,clCliffs,1)],  scaleByMapSize(6,25), 1000  );

log("Creating metal mines...");
// create large metal quarries
group = new SimpleGroup([new SimpleObject(eMetalMine, 1,1, 0,0),new SimpleObject(aBushB, 1,1, 2,2), new SimpleObject(aBushA, 0,2, 1,3)], true, clBaseResource);
createObjectGroups(group, 0,[avoidClasses(clWater, 3, clPlayer,2 , clBaseResource, 2,clCliffs,1),stayClasses(clCorsica, 1)],  scaleByMapSize(6,25), 1000  );
createObjectGroups(group, 0,[avoidClasses(clWater, 3, clPlayer,2 , clBaseResource, 2,clCliffs,1),stayClasses(clSardinia, 1)],  scaleByMapSize(6,25), 1000  );

log("Creating grass patches...");
placer = new ClumpPlacer(20, 0.3, 0.06, 0.5);
painter = new TerrainPainter(tLushGrass);
createAreas( placer, [painter,paintClass(clForest)], avoidClasses(clWater, 1,clPlayer, 0,clBaseResource, 3,clCliffs,1), scaleByMapSize(10, 40) );

log ("creating forests");
var TreeGroup = new SimpleGroup([new SimpleObject(ePine, 3,6, 1,3),new SimpleObject(ePalmTall, 1,3, 1,3),new SimpleObject(eFanPalm, 0,2, 0,2),new SimpleObject(eApple, 0,1, 1,2)], true, clForest);
createObjectGroups(TreeGroup, 0, [avoidClasses(clWater, 1, clForest, 0,clPlayer, 0,clBaseResource, 2,clCliffs,2), stayClasses(clCorsica, 3)],  scaleByMapSize(350,2500), 100 );
createObjectGroups(TreeGroup, 0, [avoidClasses(clWater, 1, clForest, 0,clPlayer, 0,clBaseResource, 2,clCliffs,2), stayClasses(clSardinia, 3)],  scaleByMapSize(350,2500), 100 );

RMS.SetProgress(75);


// create small decorative rocks
log("Creating small decorative rocks...");
group = new SimpleGroup( [new SimpleObject(aRock, 1,3, 0,1),new SimpleObject(aStandingStone, 0,2, 0,3)], true );
createObjectGroups( group, 0, avoidClasses(clWater, 0, clForest, 0, clPlayer, 0,clBaseResource, 0, clPassage, 2),
	scaleByMapSize(16, 262), 50
);


// create large decorative rocks
log("Creating large decorative rocks...");
group = new SimpleGroup( [new SimpleObject(aLargeRock, 1,2, 0,1), new SimpleObject(aRock, 1,3, 0,2)], true
);
createObjectGroups( group, 0, avoidClasses(clWater, 0, clForest, 0, clPlayer, 0,clBaseResource, 0, clPassage, 2),
	scaleByMapSize(8, 131), 50
);
createObjectGroups( group, 0, borderClasses(clWater, 5,10), scaleByMapSize(100,800), 500);

// create decorative grass
log("Creating beautification...");
group = new SimpleGroup( [new SimpleObject(aPlantA, 3,7, 0,3),new SimpleObject(aPlantB, 3,6, 0,3),new SimpleObject(aPlantC, 1,4, 0,4)], true );
createObjectGroups( group, 0, avoidClasses(clWater, 0,clBaseResource, 0, clShore,3), scaleByMapSize(100, 600), 50  );
group = new SimpleGroup( [new SimpleObject(aPlantB, 5,20, 0,5),new SimpleObject(aPlantC, 4,10, 0,4)], true );
createObjectGroups( group, 0, avoidClasses(clWater, 0,clBaseResource, 0, clShore,3), scaleByMapSize(100, 600), 50  );

RMS.SetProgress(80);

log("Creating animals...");
group = new SimpleGroup( [new SimpleObject(ePig, 2,4, 0,3)] );
createObjectGroups( group, 0, avoidClasses(clWater, 3,clBaseResource, 0), scaleByMapSize(20, 100), 50  );

group = new SimpleGroup( [new SimpleObject(eFish, 1,2, 0,3)] );
createObjectGroups( group, 0, [avoidClasses(clCreek,3,clShore,3),stayClasses(clWater, 3)], scaleByMapSize(50, 150), 100  );

RMS.SetProgress(90);

RMS.SetProgress(95);
if (randFloat(0,1) > 0.5)
	setSkySet("cumulus");
else
	setSkySet("sunny");
setSunColour(0.8,0.66,0.48);
setSunElevation(0.828932);
if (!swap)
	setSunRotation(6.3*PI/8);
else
	setSunRotation(2.3*PI/8);
setTerrainAmbientColour(0.564706,0.543726,0.419608);
setUnitsAmbientColour(0.53,0.55,0.45);
setWaterColour(0.2,0.294,0.49);
setWaterTint(0.208, 0.659, 0.925);
setWaterMurkiness(0.72);
setWaterWaviness(4.5);
// Export map data
ExportMap();




// this function will go from point [x1,z1] to point [x2,z2], while following a curve of width (starting-center-starting)
// it can smooth on the side depending on "smooth", which is the distance of the smooth. Tileclass and Terrain set a tileclass/terrain
// it effectively can create a smooth path from point [x1,z1] to point [x2,z2], ie Canyon, whatever.
// note: NOT efficient for large distances: I'm widely oversampling
function straightPassageMaker(x1, z1, x2, z2, startWidth, centerWidth, smooth, tileclass, terrain)
{
	var mapSize = g_Map.size;
	var stepNB = sqrt((x2-x1)*(x2-x1) + (z2-z1)*(z2-z1)) + 2;
	
	var startHeight = getHeight(x1,z1);
	var finishHeight = getHeight(x2,z2);
	for (var step = 0; step <= stepNB; step+=0.5)
	{
		var ix = ((stepNB-step)*x1 + x2*step) / stepNB;
		var iz = ((stepNB-step)*z1 + z2*step) / stepNB;
		// 5 at star/end, and 0 at the center
		var width = (abs(step - stepNB/2.0) *startWidth + (stepNB/2 - abs(step - stepNB/2.0)) * centerWidth ) / (stepNB/2);
		var oldDirection = [x2-x1, z2-z1];
		// let's get the perpendicular direction
		var direction = [ -oldDirection[1],oldDirection[0] ];
		if (abs(direction[0]) > abs(direction[1]))
		{
			direction[1] = direction[1] / abs(direction[0]);
			if (direction[0] > 0)
				direction[0] = 1;
			else
				direction[0] = -1;
		} else {
			direction[0] = direction[0] / abs(direction[1]);
			if (direction[1] > 0)
				direction[1] = 1;
			else
				direction[1] = -1;
		}
		for (var po = -Math.floor(width/2.0); po <= Math.floor(width/2.0); po+=0.5)
		{
			var rx = po*direction[0];
			var rz = po*direction[1];
			
			var relativeWidth = abs(po / Math.floor(width/2));
			var targetHeight = ((stepNB-step)*startHeight + finishHeight*step) / stepNB;
			if (round(ix + rx) < mapSize && round(iz + rz) < mapSize && round(ix + rx) >= 0 && round(iz + rz) >= 0)
			{
				// smoothing the sides
				if ( abs(abs(po) - abs(Math.floor(width/2.0))) < smooth)
				{
					var localHeight = getHeight(round(ix + rx), round(iz + rz));
					var localPart = smooth - abs(abs(po) - abs(Math.floor(width/2.0)));
					var targetHeight = (localHeight * localPart + targetHeight * (1/localPart) )/ (localPart + 1/localPart);
				}

				g_Map.setHeight(round(ix + rx), round(iz + rz), targetHeight);
				if (tileclass !== null)
					addToClass(round(ix + rx), round(iz + rz), tileclass);
				if (terrain !== null)
					placeTerrain(round(ix + rx), round(iz + rz), terrain);
			}
		}
	}
}
// no need for preliminary rounding
function getHeightDiff(x1, z1)
{
	var height = getHeight(round(x1),round(z1));
	var diff = 0;
	if (z1 + 1 < mapSize)
		diff += abs(getHeight(round(x1),round(z1+1)) - height);
	if (x1 + 1 < mapSize && z1 + 1 < mapSize)
		diff += abs(getHeight(round(x1+1),round(z1+1)) - height);
	if (x1 + 1 < mapSize)
		diff += abs(getHeight(round(x1+1),round(z1)) - height);
	if (x1 + 1 < mapSize && z1 - 1 >= 0)
		diff += abs(getHeight(round(x1+1),round(z1-1)) - height);
	if (z1 - 1 >= 0)
		diff += abs(getHeight(round(x1),round(z1-1)) - height);
	if (x1 - 1 >= 0 && z1 - 1 >= 0)
		diff += abs(getHeight(round(x1-1),round(z1-1)) - height);
	if (x1 - 1 >= 0)
		diff += abs(getHeight(round(x1-1),round(z1)) - height);
	if (x1 - 1 >= 0 && z1 + 1 < mapSize)
		diff += abs(getHeight(round(x1-1),round(z1+1)) - height);
	return diff;
}
function hasTextureInRadius(x1, z1,radius, textureName)
{
	for (var xx = x1-radius;xx <= x1 + radius; xx++)
		for (var zz = z1-radius;zz <= z1 + radius; zz++)
			if (xx !== x1 || zz !== z1)
				if (xx >= 0 && xx < mapSize)
					if (zz >= 0 && zz < mapSize)
					{
						if (typeof(textureName) != "number")
						{
							for (var i in textureName)
								if ( g_Map.getTexture(xx,zz) == textureName[i])
									return true;
						} else {
							if ( g_Map.getTexture(xx,zz) == textureName)
								return true;
						}
					}
	return false;
}
