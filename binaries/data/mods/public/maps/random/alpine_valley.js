Engine.LoadLibrary("rmgen");
Engine.LoadLibrary("rmgen-common");

TILE_CENTERED_HEIGHT_MAP = true;

/**
 * This class creates random mountainranges without enclosing any area completely.
 *
 * To determine their location, a graph is created where each vertex is a possible starting or
 * ending location of a mountainrange and each edge a possible mountainrange.
 *
 * That graph starts nearly complete (i.e almost every vertex is connected to most other vertices).
 * After a random edge was chosen and placed as a mountainrange,
 * all edges that intersect, that leave a too small gap to another mountainrange or that are connected to
 * too many other mountainranges are removed from the graph.
 * This is repeated until all edges were removed.
 */
function MountainRangeBuilder(args)
{
	/**
	 * These parameters paint the mountainranges after their location was determined.
	 */
	this.pathplacer = args.pathplacer;
	this.painters = args.painters;
	this.constraint = args.constraint;
	this.mountainWidth = args.mountainWidth;

	/**
	 * Minimum geometric distance between two mountains that don't end in one place (disjoint edges).
	 */
	this.minDistance = args.mountainWidth + args.passageWidth;

	/**
	 * Array of Vector2D locations where a mountainrange can start or end.
	 */
	this.vertices = args.points;

	/**
	 * Number of mountainranges starting or ending at the given point.
	 */
	this.vertexDegree = this.vertices.map(p => 0);

	/**
	 * Highest number of mountainranges that can meet in one point (maximum degree of each vertex).
	 */
	this.maxDegree = args.maxDegree;

	/**
	 * Each possible edge is an array containing two vertex indices.
	 * The algorithm adds possible edges consecutively and removes subsequently invalid edges.
	 */
	this.possibleEdges = [];
	this.InitPossibleEdges();

	/**
	 * A two-dimensional array of booleans that are true if the two corresponding vertices may be connected by a new edge (mountainrange).
	 * It is initialized with some points that should never be connected and updated with every placed edge.
	 * The purpose is to rule out any cycles in the graph, i.e. prevent any territory enclosed by mountainranges.
	 */
	this.verticesConnectable = [];
	this.InitConnectable();

	/**
	 * Currently iterated item of possibleEdges that is either used as a mountainrange or removed from the possibleEdges.
	 */
	this.index = undefined;

	/**
	 * These variables hold the indices of the two points of that edge and the location of them as a Vector2D.
	 */
	this.currentEdge = undefined;
	this.currentEdgeStart = undefined;
	this.currentEdgeEnd = undefined;
}

MountainRangeBuilder.prototype.InitPossibleEdges = function()
{
	for (let i = 0; i < this.vertices.length; ++i)
		for (let j = numPlayers; j < this.vertices.length; ++j)
			if (j > i)
				this.possibleEdges.push([i, j]);
};

MountainRangeBuilder.prototype.InitConnectable = function()
{
	for (let i = 0; i < this.vertices.length; ++i)
	{
		this.verticesConnectable[i] = [];
		for (let j = 0; j < this.vertices.length; ++j)
			this.verticesConnectable[i][j] = i >= numPlayers || j >= numPlayers || i == j || i != j - 1 && i != j + 1;
	}
};

MountainRangeBuilder.prototype.SetConnectable = function(isConnectable)
{
	this.verticesConnectable[this.currentEdge[0]][this.currentEdge[1]] = isConnectable;
	this.verticesConnectable[this.currentEdge[1]][this.currentEdge[0]] = isConnectable;
};

MountainRangeBuilder.prototype.UpdateCurrentEdge = function()
{
	this.currentEdge = this.possibleEdges[this.index];
	this.currentEdgeStart = this.vertices[this.currentEdge[0]];
	this.currentEdgeEnd = this.vertices[this.currentEdge[1]];
};

/**
 * Remove all edges that are too close to the current mountainrange or intersect.
 */
MountainRangeBuilder.prototype.RemoveInvalidEdges = function()
{
	for (let i = 0; i < this.possibleEdges.length; ++i)
	{
		this.UpdateCurrentEdge();

		let comparedEdge = this.possibleEdges[i];
		let comparedEdgeStart = this.vertices[comparedEdge[0]];
		let comparedEdgeEnd = this.vertices[comparedEdge[1]];

		let edge0Equal = this.currentEdgeStart == comparedEdgeStart;
		let edge1Equal = this.currentEdgeStart == comparedEdgeEnd;
		let edge2Equal = this.currentEdgeEnd == comparedEdgeEnd;
		let edge3Equal = this.currentEdgeEnd == comparedEdgeStart;

		if (!edge0Equal && !edge2Equal && !edge1Equal && !edge3Equal  && testLineIntersection(this.currentEdgeStart, this.currentEdgeEnd, comparedEdgeStart, comparedEdgeEnd, this.minDistance) ||
		   ( edge0Equal && !edge2Equal || !edge1Equal &&  edge3Equal) && distanceOfPointFromLine(this.currentEdgeStart, this.currentEdgeEnd, comparedEdgeEnd) < this.minDistance ||
		   (!edge0Equal &&  edge2Equal ||  edge1Equal && !edge3Equal) && distanceOfPointFromLine(this.currentEdgeStart, this.currentEdgeEnd, comparedEdgeStart) < this.minDistance)
		{
			this.possibleEdges.splice(i, 1);
			--i;
			if (this.index > i)
				--this.index;
		}
	}
};

/**
 * Tests using depth-first-search if the graph according to pointsConnectable contains a cycle,
 * i.e. if adding the currentEdge would result in an area enclosed by mountainranges.
 */
MountainRangeBuilder.prototype.HasCycles = function()
{
	let tree = [];
	let backtree = [];
	let pointQueue = [this.currentEdge[0]];

	while (pointQueue.length)
	{
		let selectedPoint = pointQueue.shift();

		if (tree.indexOf(selectedPoint) == -1)
		{
			tree.push(selectedPoint);
			backtree.push(-1);
		}

		for (let i = 0; i < this.vertices.length; ++i)
		{
			if (this.verticesConnectable[selectedPoint][i] || i == backtree[tree.lastIndexOf(selectedPoint)])
				continue;

			// If the current point was encountered already, then a cycle was identified.
			if (tree.indexOf(i) != -1)
				return true;

			// Otherwise visit this point next
			pointQueue.unshift(i);
			tree.push(i);
			backtree.push(selectedPoint);
		}
	}

	return false;
};

MountainRangeBuilder.prototype.PaintCurrentEdge = function()
{
	this.pathplacer.start = this.currentEdgeStart;
	this.pathplacer.end = this.currentEdgeEnd;
	this.pathplacer.width = this.mountainWidth;

	// Creating mountainrange
	if (!createArea(this.pathplacer, this.painters, this.constraint))
		return false;

	// Creating circular mountains at both ends of that mountainrange
	for (let point of [this.currentEdgeStart, this.currentEdgeEnd])
		createArea(
			new ClumpPlacer(diskArea(this.mountainWidth / 2), 0.95, 0.6, Infinity, point),
			this.painters,
			this.constraint);

	return true;
};

/**
 * This is the only function meant to be publicly accessible.
 */
MountainRangeBuilder.prototype.CreateMountainRanges = function()
{
	g_Map.log("Creating mountainrange with " + this.possibleEdges.length + " possible edges");

	let max = this.possibleEdges.length

	while (this.possibleEdges.length)
	{
		Engine.SetProgress(35 - 15 * this.possibleEdges.length / max);

		this.index = randIntExclusive(0, this.possibleEdges.length);
		this.UpdateCurrentEdge();
		this.SetConnectable(false);

		if (this.vertexDegree[this.currentEdge[0]] < this.maxDegree &&
		    this.vertexDegree[this.currentEdge[1]] < this.maxDegree &&
		    !this.HasCycles() &&
		    this.PaintCurrentEdge())
		{
			++this.vertexDegree[this.currentEdge[0]];
			++this.vertexDegree[this.currentEdge[1]];
			this.RemoveInvalidEdges();
		}
		else
			this.SetConnectable(true);

		this.possibleEdges.splice(this.index, 1);
	}
};

if (randBool())
{
	g_Map.log("Late spring biome");
	var tPrimary = ["alpine_dirt_grass_50"];
	var tForestFloor = "alpine_forrestfloor";
	var tCliff = ["alpine_cliff_a", "alpine_cliff_b", "alpine_cliff_c"];
	var tSecondary = "alpine_grass_rocky";
	var tHalfSnow = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tSnowLimited = ["alpine_snow_rocky"];
	var tDirt = "alpine_dirt";
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";

	var oPine = "gaia/flora_tree_pine";
	var oBerryBush = "gaia/flora_bush_berry";
	var oDeer = "gaia/fauna_deer";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";

	var aGrass = "actor|props/flora/grass_soft_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm.xml";
}
else
{
	g_Map.log("Winter biome");
	var tPrimary = ["alpine_snow_a", "alpine_snow_b"];
	var tForestFloor = "alpine_forrestfloor_snow";
	var tCliff = ["alpine_cliff_snow"];
	var tSecondary = "alpine_grass_snow_50";
	var tHalfSnow = ["alpine_grass_snow_50", "alpine_dirt_snow"];
	var tSnowLimited = ["alpine_snow_a", "alpine_snow_b"];
	var tDirt = "alpine_dirt";
	var tRoad = "new_alpine_citytile";
	var tRoadWild = "new_alpine_citytile";

	var oPine = "gaia/flora_tree_pine_w";
	var oBerryBush = "gaia/flora_bush_berry";
	var oDeer = "gaia/fauna_deer";
	var oRabbit = "gaia/fauna_rabbit";
	var oStoneLarge = "gaia/geology_stonemine_alpine_quarry";
	var oStoneSmall = "gaia/geology_stone_alpine_a";
	var oMetalLarge = "gaia/geology_metal_alpine_slabs";

	var aGrass = "actor|props/flora/grass_soft_dry_small_tall.xml";
	var aGrassShort = "actor|props/flora/grass_soft_dry_large.xml";
	var aRockLarge = "actor|geology/stone_granite_med.xml";
	var aRockMedium = "actor|geology/stone_granite_med.xml";
	var aBushMedium = "actor|props/flora/bush_medit_me_dry.xml";
	var aBushSmall = "actor|props/flora/bush_medit_sm_dry.xml";
}

var heightLand = 3;
var heightOffsetBump = 2;
var snowlineHeight = 29;
var heightMountain = 30;

const pForest = [tForestFloor + TERRAIN_SEPARATOR + oPine, tForestFloor];

var g_Map = new RandomMap(heightLand, tPrimary);

const numPlayers = getNumPlayers();
const mapCenter = g_Map.getCenter();

var clPlayer = g_Map.createTileClass();
var clHill = g_Map.createTileClass();
var clForest = g_Map.createTileClass();
var clDirt = g_Map.createTileClass();
var clRock = g_Map.createTileClass();
var clMetal = g_Map.createTileClass();
var clFood = g_Map.createTileClass();
var clBaseResource = g_Map.createTileClass();

var [playerIDs, playerPosition, playerAngle, startAngle] = playerPlacementCircle(fractionToTiles(0.35));

placePlayerBases({
	"PlayerPlacement": [playerIDs, playerPosition],
	"PlayerTileClass": clPlayer,
	"BaseResourceClass": clBaseResource,
	"CityPatch": {
		"outerTerrain": tRoadWild,
		"innerTerrain": tRoad
	},
	"Chicken": {
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
		"template": oPine
	},
	"Decoratives": {
		"template": aGrassShort
	}
});
Engine.SetProgress(20);

new MountainRangeBuilder({
	"pathplacer": new PathPlacer(undefined, undefined, undefined, 0.4, scaleByMapSize(3, 12), 0.1, 0.1, 0.1),
	"painters":[
		new LayeredPainter([tCliff, tPrimary], [3]),
		new SmoothElevationPainter(ELEVATION_SET, heightMountain, 2),
		new TileClassPainter(clHill)
	],
	"constraint": avoidClasses(clPlayer, 20),
	"passageWidth": scaleByMapSize(10, 15),
	"mountainWidth": scaleByMapSize(9, 15),
	"maxDegree": 3,
	"points": [
		// Four points near each player
		...distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.49), mapCenter)[0],
		...distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers * 1.4, fractionToTiles(0.34), mapCenter)[0],
		...distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers * 0.6, fractionToTiles(0.34), mapCenter)[0],
		...distributePointsOnCircle(numPlayers, startAngle + Math.PI / numPlayers, fractionToTiles(0.18), mapCenter)[0],
		mapCenter
	]
}).CreateMountainRanges();

Engine.SetProgress(35);

paintTerrainBasedOnHeight(heightLand + 0.1, snowlineHeight, 0, tCliff);
paintTerrainBasedOnHeight(snowlineHeight, heightMountain, 3, tSnowLimited);

g_Map.log("Creating bumps");
createAreas(
	new ClumpPlacer(scaleByMapSize(20, 50), 0.3, 0.06, Infinity),
	new SmoothElevationPainter(ELEVATION_MODIFY, heightOffsetBump, 2),
	avoidClasses(clPlayer, 10),
	scaleByMapSize(100, 200));
Engine.SetProgress(40);

g_Map.log("Creating hills");
createAreas(
	new ClumpPlacer(scaleByMapSize(40, 150), 0.2, 0.1, Infinity),
	[
		new LayeredPainter([tCliff, tSnowLimited], [2]),
		new SmoothElevationPainter(ELEVATION_SET, heightMountain, 2),
		new TileClassPainter(clHill)
	],
	avoidClasses(clPlayer, 20, clHill, 14),
	scaleByMapSize(10, 80) * numPlayers
);
Engine.SetProgress(50);

g_Map.log("Creating forests");
var [forestTrees, stragglerTrees] = getTreeCounts(500, 3000, 0.7);
var types = [
	[[tForestFloor, tPrimary, pForest], [tForestFloor, pForest]]
];

var size = forestTrees / (scaleByMapSize(2,8) * numPlayers);

var num = Math.floor(size / types.length);
for (let type of types)
	createAreas(
		new ClumpPlacer(forestTrees / num, 0.1, 0.1, Infinity),
		[
			new LayeredPainter(type, [2]),
			new TileClassPainter(clForest)
		],
		avoidClasses(clPlayer, 12, clForest, 10, clHill, 0),
		num);
Engine.SetProgress(60);

g_Map.log("Creating dirt patches");
for (let size of [scaleByMapSize(3, 48), scaleByMapSize(5, 84), scaleByMapSize(8, 128)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		[
			new LayeredPainter([[tDirt, tHalfSnow], [tHalfSnow, tSnowLimited]], [2]),
			new TileClassPainter(clDirt)
		],
		avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45));

g_Map.log("Creating grass patches");
for (let size of [scaleByMapSize(2, 32), scaleByMapSize(3, 48), scaleByMapSize(5, 80)])
	createAreas(
		new ClumpPlacer(size, 0.3, 0.06, 0.5),
		new TerrainPainter(tSecondary),
		avoidClasses(clForest, 0, clHill, 0, clDirt, 5, clPlayer, 12),
		scaleByMapSize(15, 45));

Engine.SetProgress(65);

g_Map.log("Creating stone mines");
var group = new SimpleGroup([new SimpleObject(oStoneSmall, 0, 2, 0, 4, 0, 2 * Math.PI, 1), new SimpleObject(oStoneLarge, 1, 1, 0, 4, 0, 2 * Math.PI, 4)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating small stone mines");
group = new SimpleGroup([new SimpleObject(oStoneSmall, 2,5, 1,3)], true, clRock);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clRock, 10, clHill, 1),
	scaleByMapSize(4,16), 100
);

g_Map.log("Creating metal mines");
group = new SimpleGroup([new SimpleObject(oMetalLarge, 1,1, 0,4)], true, clMetal);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 1, clPlayer, 20, clMetal, 10, clRock, 5, clHill, 1),
	scaleByMapSize(4,16), 100
);
Engine.SetProgress(70);

g_Map.log("Creating small decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockMedium, 1,3, 0,1)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(16, 262), 50
);

g_Map.log("Creating large decorative rocks");
group = new SimpleGroup(
	[new SimpleObject(aRockLarge, 1,2, 0,1), new SimpleObject(aRockMedium, 1,3, 0,2)],
	true
);
createObjectGroupsDeprecated(
	group, 0,
	avoidClasses(clForest, 0, clPlayer, 0, clHill, 0),
	scaleByMapSize(8, 131), 50
);
Engine.SetProgress(75);

g_Map.log("Creating deer");
group = new SimpleGroup(
	[new SimpleObject(oDeer, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);

g_Map.log("Creating berry bush");
group = new SimpleGroup(
	[new SimpleObject(oBerryBush, 5,7, 0,4)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 20, clHill, 1, clFood, 10),
	randIntInclusive(1, 4) * numPlayers + 2, 50
);

g_Map.log("Creating rabbit");
group = new SimpleGroup(
	[new SimpleObject(oRabbit, 2,3, 0,2)],
	true, clFood
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clForest, 0, clPlayer, 10, clHill, 1, clFood, 20),
	3 * numPlayers, 50
);
Engine.SetProgress(85);

createStragglerTrees(
	[oPine],
	avoidClasses(clForest, 1, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6),
	clForest,
	stragglerTrees);

g_Map.log("Creating small grass tufts");
var planetm = 1;

group = new SimpleGroup(
	[new SimpleObject(aGrassShort, 1,2, 0,1, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill, 2, clPlayer, 2, clDirt, 0),
	planetm * scaleByMapSize(13, 200)
);
Engine.SetProgress(90);

g_Map.log("Creating large grass tufts");
group = new SimpleGroup(
	[new SimpleObject(aGrass, 2,4, 0,1.8, -Math.PI / 8, Math.PI / 8), new SimpleObject(aGrassShort, 3,6, 1.2,2.5, -Math.PI / 8, Math.PI / 8)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill, 2, clPlayer, 2, clDirt, 1, clForest, 0),
	planetm * scaleByMapSize(13, 200)
);
Engine.SetProgress(95);

g_Map.log("Creating bushes");
group = new SimpleGroup(
	[new SimpleObject(aBushMedium, 1,2, 0,2), new SimpleObject(aBushSmall, 2,4, 0,2)]
);
createObjectGroupsDeprecated(group, 0,
	avoidClasses(clHill, 1, clPlayer, 1, clDirt, 1),
	planetm * scaleByMapSize(13, 200), 50
);

placePlayersNomad(clPlayer, avoidClasses(clForest, 1, clMetal, 4, clRock, 4, clHill, 4, clFood, 2));

setSkySet(pickRandom(["cirrus", "cumulus", "sunny"]));
setSunRotation(randomAngle());
setSunElevation(Math.PI * randFloat(1/5, 1/3));
setWaterColor(0.0, 0.047, 0.286);				// dark majestic blue
setWaterTint(0.471, 0.776, 0.863);				// light blue
setWaterMurkiness(0.72);
setWaterWaviness(2.0);
setWaterType("lake");

g_Map.ExportMap();
