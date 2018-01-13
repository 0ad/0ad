/**
 * @file These functions locate and place the starting entities of players.
 */

/**
 * These are identifiers of functions that can generate parts of a player base.
 * There must be a function starting with placePlayerBase and ending with this name.
 * This is a global so mods can extend this from external files.
 */
var g_PlayerBaseFunctions = [
	// Possibly mark player class first here and use it afterwards
	"CityPatch",
	// Create the largest and most important entities first
	"Trees",
	"Mines",
	"Treasures",
	"Berries",
	"Chicken",
	"Decoratives"
];

/**
 * Gets the default starting entities for the civ of the given player, as defined by the civ file.
 */
function getStartingEntities(playerID)
{
	return g_CivData[getCivCode(playerID)].StartEntities;
}

/**
 * Places the given entities at the given location (typically a civic center and starting units).
 * @param civEntities - An array of objects with the Template property and optionally a Count property.
 * The first entity is placed in the center, the other ones surround it.
 */
function placeStartingEntities(fx, fz, playerID, civEntities, dist = 6, orientation = BUILDING_ORIENTATION)
{
	// Place the central structure
	let i = 0;
	let firstTemplate = civEntities[i].Template;
	if (firstTemplate.startsWith("structures/"))
	{
		placeObject(fx, fz, firstTemplate, playerID, orientation);
		++i;
	}

	// Place entities surrounding it
	let space = 2;
	for (let j = i; j < civEntities.length; ++j)
	{
		let angle = orientation - Math.PI * (1 - j / 2);
		let count = civEntities[j].Count || 1;

		for (let num = 0; num < count; ++num)
			placeObject(
				fx + dist * Math.cos(angle) + space * (-num + 0.75 * Math.floor(count / 2)) * Math.sin(angle),
				fz + dist * Math.sin(angle) + space * (num - 0.75 * Math.floor(count / 2)) * Math.cos(angle),
				civEntities[j].Template,
				playerID,
				angle);
	}
}

/**
 * Places the default starting entities as defined by the civilization definition, optionally including city walls.
 */
function placeCivDefaultStartingEntities(x, z, playerID, wallType, dist = 6, orientation = BUILDING_ORIENTATION)
{
	placeStartingEntities(x, z, playerID, getStartingEntities(playerID), dist, orientation);
	placeStartingWalls(x, z, playerID, wallType, orientation);
}

/**
 * If the map is large enough and the civilization defines them, places the initial city walls or towers.
 * @param {string|boolean} wallType - Either "towers" to only place the wall turrets or a boolean indicating enclosing city walls.
 */
function placeStartingWalls(x, z, playerID, wallType, orientation = BUILDING_ORIENTATION)
{
	let civ = getCivCode(playerID);
	if (civ != "iber" || getMapSize() <= 128)
		return;

	if (wallType == "towers")
		placePolygonalWall(x, z, 15, ["entry"], "tower", civ, playerID, orientation, 7);
	else if (wallType)
		placeGenericFortress(x, z, 20, playerID);
}

/**
 * Places the civic center and starting resources for all given players.
 */
function placePlayerBases(playerBaseArgs)
{
	let [playerIDs, playerX, playerZ] = playerBaseArgs.PlayerPlacement;

	for (let i = 0; i < getNumPlayers(); ++i)
	{
		playerBaseArgs.playerID = playerIDs[i];
		playerBaseArgs.playerX = playerX[i];
		playerBaseArgs.playerZ = playerZ[i];
		placePlayerBase(playerBaseArgs);
	}
}

/**
 * Places the civic center and starting resources.
 */
function placePlayerBase(playerBaseArgs)
{
	log("Creating base for player " + playerBaseArgs.playerID + "...");

	let fx = fractionToTiles(playerBaseArgs.playerX);
	let fz = fractionToTiles(playerBaseArgs.playerZ);

	placeCivDefaultStartingEntities(fx, fz, playerBaseArgs.playerID, playerBaseArgs.Walls !== undefined ? playerBaseArgs.Walls : true);

	if (playerBaseArgs.PlayerTileClass !== undefined)
		addCivicCenterAreaToClass(Math.round(fx), Math.round(fz), playerBaseArgs.PlayerTileClass);

	for (let functionID of g_PlayerBaseFunctions)
	{
		let funcName = "placePlayerBase" + functionID;
		let func = global[funcName];
		if (!func)
			throw new Error("Could not find " + funcName);

		if (!playerBaseArgs[functionID])
			continue;

		let args = playerBaseArgs[functionID];

		// Copy some global arguments to the arguments for each function
		for (let prop of ["playerID", "playerX", "playerZ", "BaseResourceClass", "baseResourceConstraint"])
			args[prop] = playerBaseArgs[prop];

		func(args);
	}
}

function defaultPlayerBaseRadius()
{
	return scaleByMapSize(15, 25);
}

/**
 * Marks the corner and center tiles of an area that is about the size of a Civic Center with the given TileClass.
 * Used to prevent resource collisions with the Civic Center.
 */
function addCivicCenterAreaToClass(ix, iz, tileClass)
{
	addToClass(ix, iz, tileClass);

	addToClass(ix, iz + 5, tileClass);
	addToClass(ix, iz - 5, tileClass);

	addToClass(ix + 5, iz, tileClass);
	addToClass(ix - 5, iz, tileClass);
}

/**
 * Helper function.
 */
function getPlayerBaseArgs(playerBaseArgs)
{
	let baseResourceConstraint = playerBaseArgs.BaseResourceClass && avoidClasses(playerBaseArgs.BaseResourceClass, 4);

	if (playerBaseArgs.baseResourceConstraint)
		baseResourceConstraint = new AndConstraint([baseResourceConstraint, playerBaseArgs.baseResourceConstraint]);

	return [
		(property, defaultVal) => playerBaseArgs[property] === undefined ? defaultVal : playerBaseArgs[property],
		new Vector2D(playerBaseArgs.playerX, playerBaseArgs.playerZ).mult(getMapSize()),
		baseResourceConstraint
	];
}

function placePlayerBaseCityPatch(args)
{
	let [get, basePosition, baseResourceConstraint] = getPlayerBaseArgs(args);

	let painters = [];

	if (args.outerTerrain && args.innerTerrain)
		painters.push(new LayeredPainter([args.outerTerrain, args.innerTerrain], [get("width", 1)]));

	if (args.painters)
		painters = painters.concat(args.painters);

	createArea(
		new ClumpPlacer(
			Math.floor(diskArea(get("radius", defaultPlayerBaseRadius() / 3))),
			get("coherence", 0.6),
			get("smoothness", 0.3),
			get("failFraction", 10),
			Math.round(basePosition.x),
			Math.round(basePosition.y)),
		painters);
}

function placePlayerBaseChicken(args)
{
	let [get, basePosition, baseResourceConstraint] = getPlayerBaseArgs(args);

	for (let i = 0; i < get("groupCount", 2); ++i)
	{
		let success = false;
		for (let tries = 0; tries < get("maxTries", 30); ++tries)
		{
			let loc = new Vector2D(0, get("distance", 9)).rotate(randFloat(0, 2 * Math.PI)).add(basePosition);
			if (createObjectGroup(
				new SimpleGroup(
					[new SimpleObject(get("template", "gaia/fauna_chicken"), 5, 5, 0, get("count", 2))],
					true,
					args.BaseResourceClass,
					loc.x,
					loc.y),
				0,
				baseResourceConstraint))
			{
				success = true;
				break;
			}
		}

		if (!success)
		{
			error("Could not place chicken for player " + args.playerID);
			return;
		}
	}
}

function placePlayerBaseBerries(args)
{
	let [get, basePosition, baseResourceConstraint] = getPlayerBaseArgs(args);
	for (let tries = 0; tries < get("maxTries", 30); ++tries)
	{
		let loc = new Vector2D(0, get("distance", 12)).rotate(randFloat(0, 2 * Math.PI)).add(basePosition);
		if (createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(args.template, get("minCount", 5), get("maxCount", 5), get("maxDist", 1), get("maxDist", 3))],
				true,
				args.BaseResourceClass,
				loc.x,
				loc.y),
			0,
			baseResourceConstraint))
			return;
	}

	error("Could not place berries for player " + args.playerID);
}

function placePlayerBaseMines(args)
{
	let [get, basePosition, baseResourceConstraint] = getPlayerBaseArgs(args);

	let angleBetweenMines = randFloat(get("minAngle", Math.PI / 6), get("maxAngle", Math.PI / 3));
	let mineCount = args.types.length;

	let groupElements = [];
	if (args.groupElements)
		groupElements = groupElements.concat(args.groupElements);

	for (let tries = 0; tries < get("maxTries", 75); ++tries)
	{
		// First find a place where all mines can be placed
		let pos = [];
		let startAngle = randFloat(0, 2 * Math.PI);
		for (let i = 0; i < mineCount; ++i)
		{
			let angle = startAngle + angleBetweenMines * (i + (mineCount - 1) / 2);
			pos[i] = new Vector2D(0, get("distance", 12)).rotate(angle).add(basePosition).round();
			if (!g_Map.validT(pos[i].x, pos[i].y) || !baseResourceConstraint.allows(pos[i].x, pos[i].y))
			{
				pos = undefined;
				break;
			}
		}

		if (!pos)
			continue;

		// Place the mines
		for (let i = 0; i < mineCount; ++i)
		{
			if (args.types[i].type && args.types[i].type == "stone_formation")
			{
				createStoneMineFormation(pos[i].x, pos[i].y, args.types[i].template, args.types[i].terrain);
				addToClass(pos[i].x, pos[i].y, args.BaseResourceClass);
				continue;
			}

			createObjectGroup(
				new SimpleGroup(
					[new SimpleObject(args.types[i].template, 1, 1, 0, 0)].concat(groupElements),
					true,
					args.BaseResourceClass,
					pos[i].x,
					pos[i].y),
				0);
		}
		return;
	}

	error("Could not place mines for player " + args.playerID);
}

function placePlayerBaseTrees(args)
{
	let [get, basePosition, baseResourceConstraint] = getPlayerBaseArgs(args);

	let num = Math.floor(get("count", scaleByMapSize(7, 20)));

	for (let x = 0; x < get("maxTries", 30); ++x)
	{
		let loc = new Vector2D(0, randFloat(get("minDist", 11), get("maxDist", 13))).rotate(randFloat(0, 2 * Math.PI)).add(basePosition).round();

		if (createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(args.template, num, num, get("minDistGroup", 0), get("maxDistGroup", 5))],
				false,
				args.BaseResourceClass,
				loc.x,
				loc.y),
			0,
			baseResourceConstraint))
			return;
	}

	error("Could not place starting trees for player " + args.playerID);
}

function placePlayerBaseTreasures(args)
{
	let [get, basePosition, baseResourceConstraint] = getPlayerBaseArgs(args);

	for (let resourceTypeArgs of args.types)
	{
		get = (property, defaultVal) => resourceTypeArgs[property] === undefined ? defaultVal : resourceTypeArgs[property];

		let success = false;

		for (let tries = 0; tries < get("maxTries", 30); ++tries)
		{
			let loc = new Vector2D(0, randFloat(get("minDist", 11), get("maxDist", 13))).rotate(randFloat(0, 2 * Math.PI)).add(basePosition).round();

			if (createObjectGroup(
				new SimpleGroup(
					[new SimpleObject(resourceTypeArgs.template, get("count", 14), get("count", 14), get("minDistGroup", 1), get("maxDistGroup", 3))],
					false,
					args.BaseResourceClass,
					loc.x,
					loc.y),
				0,
				baseResourceConstraint))
			{
				success = true;
				break;
			}
		}
		if (!success)
		{
			error("Could not place treasure " + resourceTypeArgs.template + " for player " + args.playerID);
			return;
		}
	}
}

/**
 * Typically used for placing grass tufts around the civic centers.
 */
function placePlayerBaseDecoratives(args)
{
	let [get, basePosition, baseResourceConstraint] = getPlayerBaseArgs(args);

	for (let i = 0; i < get("count", scaleByMapSize(2, 5)); ++i)
	{
		let success = false;
		for (let x = 0; x < get("maxTries", 30); ++x)
		{
			let loc = new Vector2D(0, randIntInclusive(get("minDist", 8), get("maxDist", 11))).rotate(randFloat(0, 2 * Math.PI)).add(basePosition).round();

			if (createObjectGroup(
				new SimpleGroup(
					[new SimpleObject(args.template, get("minCount", 2), get("maxCount", 5), 0, 1)],
					false,
					args.BaseResourceClass,
					loc.x,
					loc.y),
				0,
				baseResourceConstraint))
			{
				success = true;
				break;
			}
		}
		if (!success)
			// Don't warn since the decoratives are not important
			return;
	}
}

/**
 * Sorts an array of player IDs by team index. Players without teams come first.
 * Randomize order for players of the same team.
 */
function sortPlayers(playerIDs)
{
	return shuffleArray(playerIDs).sort((playerID1, playerID2) => getPlayerTeam(playerID1) - getPlayerTeam(playerID2));
}

/**
 * Randomize playerIDs but sort by team.
 *
 * @returns {Array} - every item is an array of player indices
 */
function sortAllPlayers()
{
	let playerIDs = [];
	for (let i = 0; i < getNumPlayers(); ++i)
		playerIDs.push(i+1);

	return sortPlayers(playerIDs);
}

/**
 * Rearrange order so that teams of neighboring players alternate (if the given IDs are sorted by team).
 */
function primeSortPlayers(playerIDs)
{
	if (!playerIDs.length)
		return [];

	let prime = [];
	for (let i = 0; i < Math.ceil(playerIDs.length / 2); ++i)
	{
		prime.push(playerIDs[i]);
		prime.push(playerIDs[playerIDs.length - 1 - i]);
	}

	return prime;
}

function primeSortAllPlayers()
{
	return primeSortPlayers(sortAllPlayers());
}

/**
 * Determine player starting positions on a circular pattern.
 */
function playerPlacementCircle(radius, startingAngle = undefined, centerX = 0.5, centerZ = 0.5)
{
	let startAngle = startingAngle !== undefined ? startingAngle : randFloat(0, 2 * Math.PI);
	let [locations, playerAngle] = distributePointsOnCircle(getNumPlayers(), startAngle, radius, new Vector2D(centerX, centerZ));
	return [sortAllPlayers(), locations.map(l => l.x), locations.map(l => l.y), playerAngle, startAngle];
}

/**
 * Determine player starting positions on a circular pattern, with a custom angle for each player.
 * Commonly used for gulf terrains.
 */
function playerPlacementCustomAngle(radius, centerX, centerZ, playerAngleFunc)
{
	let playerX = [];
	let playerZ = [];
	let playerAngle = [];

	let numPlayers = getNumPlayers();

	for (let i = 0; i < numPlayers; ++i)
	{
		playerAngle[i] = playerAngleFunc(i);
		playerX[i] = centerX + radius * Math.cos(playerAngle[i]);
		playerZ[i] = centerZ + radius * Math.sin(playerAngle[i]);
	}

	return [playerX, playerZ, playerAngle];
}

/**
 * Returns player starting positions located on two parallel lines, typically used by central river maps.
 * If there are two teams with an equal number of players, each team will occupy exactly one line.
 * Angle 0 means the players are placed in north to south direction, i.e. along the Z axis.
 */
function playerPlacementRiver(angle, width)
{
	let positions = [];

	let numPlayers = getNumPlayers();
	let numPlayersEven = numPlayers % 2 == 0;

	let mapCenter = new Vector2D(0.5, 0.5);

	for (let i = 0; i < numPlayers; ++i)
	{
		let currentPlayerEven = i % 2 == 0;

		let offsetDivident = numPlayersEven || currentPlayerEven ? (i + 1) % 2 : 0;
		let offsetDivisor = numPlayersEven ? 0 : currentPlayerEven ? +1 : -1;

		positions[i] = new Vector2D(
			width * (i % 2) + (1 - width) / 2,
			((i - 1 + offsetDivident) / 2 + 1) / ((numPlayers + offsetDivisor) / 2 + 1)).rotateAround(angle, mapCenter);
	}

	return [primeSortAllPlayers(), positions.map(p => p.x), positions.map(p => p.y)];
}

/***
 * Returns starting positions located on two parallel lines.
 * The locations on the first line are shifted in comparison to the other line.
 * The players are grouped per team and hence they can be found on both lines.
 */
function playerPlacementLine(horizontal, center, width)
{
	let playerX = [];
	let playerZ = [];
	let numPlayers = getNumPlayers();

	for (let i = 0; i < numPlayers; ++i)
	{
		playerX[i] = (i + 1) / (numPlayers + 1);
		playerZ[i] = center + width * (i % 2 - 1/2);

		if (!horizontal)
			[playerX[i], playerZ[i]] = [playerZ[i], playerX[i]];
	}

	return [sortAllPlayers(), playerX, playerZ];
}

/**
 * Sorts the playerIDs so that team members are as close as possible.
 */
function sortPlayersByLocation(startLocations)
{
	// Sort start locations to form a "ring"
	let startLocationOrder = sortPointsShortestCycle(startLocations);

	let newStartLocations = [];
	for (let i = 0; i < startLocations.length; ++i)
		newStartLocations.push(startLocations[startLocationOrder[i]]);

	startLocations = newStartLocations;

	// Sort players by team
	let playerIDs = [];
	let teams = [];
	for (let i = 0; i < g_MapSettings.PlayerData.length - 1; ++i)
	{
		playerIDs.push(i+1);
		let t = g_MapSettings.PlayerData[i + 1].Team;
		if (teams.indexOf(t) == -1 && t !== undefined)
			teams.push(t);
	}

	playerIDs = sortPlayers(playerIDs);

	if (!teams.length)
		return [playerIDs, startLocations];

	// Minimize maximum distance between players within a team
	let minDistance = Infinity;
	let bestShift;
	for (let s = 0; s < playerIDs.length; ++s)
	{
		let maxTeamDist = 0;
		for (let pi = 0; pi < playerIDs.length - 1; ++pi)
		{
			let t1 = getPlayerTeam(playerIDs[(pi + s) % playerIDs.length]);

			if (teams.indexOf(t1) === -1)
				continue;

			for (let pj = pi + 1; pj < playerIDs.length; ++pj)
			{
				if (t1 != getPlayerTeam(playerIDs[(pj + s) % playerIDs.length]))
					continue;

				maxTeamDist = Math.max(
					maxTeamDist,
					Math.euclidDistance2D(
						startLocations[pi].x,
						startLocations[pi].y,
						startLocations[pj].x,
						startLocations[pj].y));
			}
		}

		if (maxTeamDist < minDistance)
		{
			minDistance = maxTeamDist;
			bestShift = s;
		}
	}

	if (bestShift)
	{
		let newPlayerIDs = [];
		for (let i = 0; i < playerIDs.length; ++i)
			newPlayerIDs.push(playerIDs[(i + bestShift) % playerIDs.length]);
		playerIDs = newPlayerIDs;
	}

	return [playerIDs, startLocations];
}
