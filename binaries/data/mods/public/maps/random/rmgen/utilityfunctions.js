var g_numStragglerTrees = 0;

function createForests(terrainset, constraint, tileclass, numMultiplier = 1, minTrees = 500, maxTrees = 3000, forestProbability = 0.7)
{
	log("Creating forests...");

	tileclass = tileclass || clForest;
	constraint = constraint || avoidClasses(clPlayer, 20, clForest, 17, clHill, 0);

	var [tM, tFF1, tFF2, tF1, tF2] = terrainset;
	var totalTrees = scaleByMapSize(minTrees, maxTrees);
	var numForest = totalTrees * forestProbability;
	g_numStragglerTrees = totalTrees * (1.0 - forestProbability);

	if (!forestProbability)
		return;

	log("Creating forests...");

	let types = [
		[[tFF2, tM, tF1], [tFF2, tF1]],
		[[tFF1, tM, tF2], [tFF1, tF2]]
	];

	let num = Math.floor(numForest / (scaleByMapSize(3,6) * numPlayers) / types.length);
	for (let type of types)
		createAreas(
			new ChainPlacer(1, Math.floor(scaleByMapSize(3, 5)), numForest / num, 0.5),
			[
				new LayeredPainter(type, [2]),
				paintClass(tileclass)
			],
			constraint,
			num
		);
}

function createStragglerTrees(types, constraint, tileclass)
{
	log("Creating straggler trees...");

	constraint = constraint !== undefined ?
		constraint :
		avoidClasses(clForest, 8, clHill, 1, clPlayer, 12, clMetal, 6, clRock, 6);

	tileclass = tileclass !== undefined ? tileclass : clForest;

	var num = floor(g_numStragglerTrees / types.length);
	for (var i = 0; i < types.length; ++i)
	{
		let group = new SimpleGroup(
			[new SimpleObject(types[i], 1,1, 0,3)],
			true, tileclass
		);
		createObjectGroupsDeprecated(group, 0,
			constraint,
			num
		);
	}
}
