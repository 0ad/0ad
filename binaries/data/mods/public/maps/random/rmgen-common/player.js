/**
 * @file These functions locate and place the starting entities of players.
 */

var g_NomadTreasureTemplates = {
	"food": "gaia/treasure/food_jars",
	"wood": "gaia/treasure/wood",
	"stone": "gaia/treasure/stone",
	"metal": "gaia/treasure/metal"
};

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

function isNomad()
{
	return !!g_MapSettings.Nomad;
}

function getNumPlayers()
{
	return g_MapSettings.PlayerData.length - 1;
}

function getCivCode(playerID)
{
	return g_MapSettings.PlayerData[playerID].Civ;
}

function areAllies(playerID1, playerID2)
{
	return g_MapSettings.PlayerData[playerID1].Team !== undefined &&
	       g_MapSettings.PlayerData[playerID2].Team !== undefined &&
	       g_MapSettings.PlayerData[playerID1].Team != -1 &&
	       g_MapSettings.PlayerData[playerID2].Team != -1 &&
	       g_MapSettings.PlayerData[playerID1].Team === g_MapSettings.PlayerData[playerID2].Team;
}

function getPlayerTeam(playerID)
{
	if (g_MapSettings.PlayerData[playerID].Team === undefined)
		return -1;

	return g_MapSettings.PlayerData[playerID].Team;
}

/**
 * Gets the default starting entities for the civ of the given player, as defined by the civ file.
 */
function getStartingEntities(playerID)
{
	return g_CivData[getCivCode(playerID)].StartEntities;
}

/**
 * Places the given entities at the given location (typically a civic center and starting units).
 * @param location - A Vector2D specifying tile coordinates.
 * @param civEntities - An array of objects with the Template property and optionally a Count property.
 * The first entity is placed in the center, the other ones surround it.
 */
function placeStartingEntities(location, playerID, civEntities, dist = 6, orientation = BUILDING_ORIENTATION)
{
	// Place the central structure
	let i = 0;
	let firstTemplate = civEntities[i].Template;
	if (firstTemplate.startsWith("structures/"))
	{
		g_Map.placeEntityPassable(firstTemplate, playerID, location, orientation);
		++i;
	}

	// Place entities surrounding it
	let space = 2;
	for (let j = i; j < civEntities.length; ++j)
	{
		let angle = orientation - Math.PI * (1 - j / 2);
		let count = civEntities[j].Count || 1;

		for (let num = 0; num < count; ++num)
		{
			let position = Vector2D.sum([
				location,
				new Vector2D(dist, 0).rotate(-angle),
				new Vector2D(space * (-num + (count - 1) / 2), 0).rotate(angle)
			]);

			g_Map.placeEntityPassable(civEntities[j].Template, playerID, position, angle);
		}
	}
}

/**
 * Places the default starting entities as defined by the civilization definition, optionally including city walls.
 */
function placeCivDefaultStartingEntities(position, playerID, wallType, dist = 6, orientation = BUILDING_ORIENTATION)
{
	placeStartingEntities(position, playerID, getStartingEntities(playerID), dist, orientation);
	placeStartingWalls(position, playerID, wallType, orientation);
}

/**
 * If the map is large enough and the civilization defines them, places the initial city walls or towers.
 * @param {string|boolean} wallType - Either "towers" to only place the wall turrets or a boolean indicating enclosing city walls.
 */
function placeStartingWalls(position, playerID, wallType, orientation = BUILDING_ORIENTATION)
{
	let civ = getCivCode(playerID);
	if (civ != "iber" || g_Map.getSize() <= 128)
		return;

	// TODO: should prevent trees inside walls
	// When fixing, remove the DeleteUponConstruction flag from template_gaia_flora.xml

	if (wallType == "towers")
		placePolygonalWall(position, 15, ["entry"], "tower", civ, playerID, orientation, 7);
	else if (wallType)
		placeGenericFortress(position, 20, playerID);
}

/**
 * Places the civic center and starting resources for all given players.
 */
function placePlayerBases(playerBaseArgs)
{
	g_Map.log("Creating playerbases");

	let [playerIDs, playerPosition] = playerBaseArgs.PlayerPlacement;

	for (let i = 0; i < getNumPlayers(); ++i)
	{
		playerBaseArgs.playerID = playerIDs[i];
		playerBaseArgs.playerPosition = playerPosition[i];
		placePlayerBase(playerBaseArgs);
	}
}

/**
 * Places the civic center and starting resources.
 */
function placePlayerBase(playerBaseArgs)
{
	if (isNomad())
		return;

	placeCivDefaultStartingEntities(playerBaseArgs.playerPosition, playerBaseArgs.playerID, playerBaseArgs.Walls !== undefined ? playerBaseArgs.Walls : true);

	if (playerBaseArgs.PlayerTileClass)
		addCivicCenterAreaToClass(playerBaseArgs.playerPosition, playerBaseArgs.PlayerTileClass);

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
		for (let prop of ["playerID", "playerPosition", "BaseResourceClass", "baseResourceConstraint"])
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
function addCivicCenterAreaToClass(position, tileClass)
{
	createArea(
		new DiskPlacer(5, position),
		new TileClassPainter(tileClass));
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
		playerBaseArgs.playerPosition,
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
			get("failFraction", Infinity),
			basePosition),
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
			let position = new Vector2D(0, get("distance", 9)).rotate(randomAngle()).add(basePosition);
			if (createObjectGroup(
				new SimpleGroup(
					[
						new SimpleObject(
							get("template", "gaia/fauna_chicken"),
							get("minGroupCount", 5),
							get("maxGroupCount", 5),
							get("minGroupDistance", 0),
							get("maxGroupDistance", 2))
					],
					true,
					args.BaseResourceClass,
					position),
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
		let position = new Vector2D(0, get("distance", 12)).rotate(randomAngle()).add(basePosition);
		if (createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(args.template, get("minCount", 5), get("maxCount", 5), get("maxDist", 1), get("maxDist", 3))],
				true,
				args.BaseResourceClass,
				position),
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
		let startAngle = randomAngle();
		for (let i = 0; i < mineCount; ++i)
		{
			let angle = startAngle + angleBetweenMines * (i + (mineCount - 1) / 2);
			pos[i] = new Vector2D(0, get("distance", 12)).rotate(angle).add(basePosition).round();
			if (!g_Map.validTilePassable(pos[i]) || !baseResourceConstraint.allows(pos[i]))
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
				createStoneMineFormation(pos[i], args.types[i].template, args.types[i].terrain);
				args.BaseResourceClass.add(pos[i]);
				continue;
			}

			createObjectGroup(
				new SimpleGroup(
					[new SimpleObject(args.types[i].template, 1, 1, 0, 0)].concat(groupElements),
					true,
					args.BaseResourceClass,
					pos[i]),
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
		let position = new Vector2D(0, randFloat(get("minDist", 11), get("maxDist", 13))).rotate(randomAngle()).add(basePosition).round();

		if (createObjectGroup(
			new SimpleGroup(
				[new SimpleObject(args.template, num, num, get("minDistGroup", 0), get("maxDistGroup", 5))],
				false,
				args.BaseResourceClass,
				position),
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
			let position = new Vector2D(0, randFloat(get("minDist", 11), get("maxDist", 13))).rotate(randomAngle()).add(basePosition).round();

			if (createObjectGroup(
				new SimpleGroup(
					[new SimpleObject(resourceTypeArgs.template, get("count", 14), get("count", 14), get("minDistGroup", 1), get("maxDistGroup", 3))],
					false,
					args.BaseResourceClass,
					position),
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
			let position = new Vector2D(0, randIntInclusive(get("minDist", 8), get("maxDist", 11))).rotate(randomAngle()).add(basePosition).round();

			if (createObjectGroup(
				new SimpleGroup(
					[new SimpleObject(args.template, get("minCount", 2), get("maxCount", 5), 0, 1)],
					false,
					args.BaseResourceClass,
					position),
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

function placePlayersNomad(playerClass, constraints)
{
	if (!isNomad())
		return undefined;

	g_Map.log("Placing nomad starting units");

	let distance = scaleByMapSize(60, 240);
	let constraint = new StaticConstraint(constraints);

	let numPlayers = getNumPlayers();
	let playerIDs = shuffleArray(sortAllPlayers());
	let playerPosition = [];

	for (let i = 0; i < numPlayers; ++i)
	{
		let objects = getStartingEntities(playerIDs[i]).filter(ents => ents.Template.startsWith("units/")).map(
			ents => new SimpleObject(ents.Template, ents.Count || 1, ents.Count || 1, 1, 3));

		// Add treasure if too few resources for a civic center
		let ccCost = Engine.GetTemplate("structures/" + getCivCode(playerIDs[i]) + "_civil_centre").Cost.Resources;
		for (let resourceType in ccCost)
		{
			let treasureTemplate = g_NomadTreasureTemplates[resourceType];

			let count = Math.max(0, Math.ceil(
				(ccCost[resourceType] - (g_MapSettings.StartingResources || 0)) /
				Engine.GetTemplate(treasureTemplate).ResourceSupply.Amount));

			objects.push(new SimpleObject(treasureTemplate, count, count, 3, 5));
		}

		// Try place these entities at a random location
		let group = new SimpleGroup(objects, true, playerClass);
		let success = false;
		for (let distanceFactor of [1, 1/2, 1/4, 0])
			if (createObjectGroups(group, playerIDs[i], new AndConstraint([constraint, avoidClasses(playerClass, distance * distanceFactor)]), 1, 200, false).length)
			{
				success = true;
				playerPosition[i] = group.centerPosition;
				break;
			}

		if (!success)
			throw new Error("Could not place starting units for player " + playerIDs[i] + "!");
	}

	return [playerIDs, playerPosition];
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
	let prime = [];
	for (let i = 0; i < Math.floor(playerIDs.length / 2); ++i)
	{
		prime.push(playerIDs[i]);
		prime.push(playerIDs[playerIDs.length - 1 - i]);
	}

	if (playerIDs.length % 2)
		prime.push(playerIDs[Math.floor(playerIDs.length / 2)]);

	return prime;
}

function primeSortAllPlayers()
{
	return primeSortPlayers(sortAllPlayers());
}

/*
 * Separates playerIDs into two arrays such that teammates are in the same array,
 * unless everyone's on the same team in which case they'll be split in half.
 */
function partitionPlayers(playerIDs)
{
	let teamIDs = Array.from(new Set(playerIDs.map(getPlayerTeam)));
	let teams = teamIDs.map(teamID => playerIDs.filter(playerID => getPlayerTeam(playerID) == teamID));
	if (teamIDs.indexOf(-1) != -1)
		teams = teams.concat(teams.splice(teamIDs.indexOf(-1), 1)[0].map(playerID => [playerID]));

	if (teams.length == 1)
	{
		let idx = Math.floor(teams[0].length / 2);
		teams = [teams[0].slice(idx), teams[0].slice(0, idx)];
	}

	teams.sort((a, b) => b.length - a.length);

	// Use the greedy algorithm: add the next team to the side with fewer players
	return teams.reduce(([east, west], team) =>
		east.length > west.length ?
			[east, west.concat(team)] :
			[east.concat(team), west],
		[[], []]);
}

/**
 * Determine player starting positions on a circular pattern.
 */
function playerPlacementCircle(radius, startingAngle = undefined, center = undefined)
{
	let startAngle = startingAngle !== undefined ? startingAngle : randomAngle();
	let [playerPosition, playerAngle] = distributePointsOnCircle(getNumPlayers(), startAngle, radius, center || g_Map.getCenter());
	return [sortAllPlayers(), playerPosition.map(p => p.round()), playerAngle, startAngle];
}

/**
 * Determine player starting positions on a circular pattern, with a custom angle for each player.
 * Commonly used for gulf terrains.
 */
function playerPlacementCustomAngle(radius, center, playerAngleFunc)
{
	let playerPosition = [];
	let playerAngle = [];

	let numPlayers = getNumPlayers();

	for (let i = 0; i < numPlayers; ++i)
	{
		playerAngle[i] = playerAngleFunc(i);
		playerPosition[i] = Vector2D.add(center, new Vector2D(radius, 0).rotate(-playerAngle[i])).round();
	}

	return [playerPosition, playerAngle];
}

/**
 * Returns player starting positions equally spaced along an arc.
 */
function playerPlacementArc(playerIDs, center, radius, startAngle, endAngle)
{
	return distributePointsOnCircularSegment(
		playerIDs.length + 2,
		endAngle - startAngle,
		startAngle,
		radius,
		center
	)[0].slice(1, -1).map(p => p.round());
}

/**
 * Returns player starting positions located on two symmetrically placed arcs, with teammates placed on the same arc.
 */
function playerPlacementArcs(playerIDs, center, radius, mapAngle, startAngle, endAngle)
{
	let [east, west] = partitionPlayers(playerIDs);
	let eastPosition = playerPlacementArc(east, center, radius, mapAngle + startAngle, mapAngle + endAngle);
	let westPosition = playerPlacementArc(west, center, radius, mapAngle - startAngle, mapAngle - endAngle);
	return playerIDs.map(playerID => east.indexOf(playerID) != -1 ?
		eastPosition[east.indexOf(playerID)] :
		westPosition[west.indexOf(playerID)]);
}

/**
 * Returns player starting positions located on two parallel lines, typically used by central river maps.
 * If there are two teams with an equal number of players, each team will occupy exactly one line.
 * Angle 0 means the players are placed in north to south direction, i.e. along the Z axis.
 */
function playerPlacementRiver(angle, width, center = undefined)
{
	let numPlayers = getNumPlayers();
	let numPlayersEven = numPlayers % 2 == 0;
	let mapSize = g_Map.getSize();
	let centerPosition = center || g_Map.getCenter();
	let playerPosition = [];

	for (let i = 0; i < numPlayers; ++i)
	{
		let currentPlayerEven = i % 2 == 0;

		let offsetDivident = numPlayersEven || currentPlayerEven ? (i + 1) % 2 : 0;
		let offsetDivisor = numPlayersEven ? 0 : currentPlayerEven ? +1 : -1;

		playerPosition[i] = new Vector2D(
			width * (i % 2) + (mapSize - width) / 2,
			fractionToTiles(((i - 1 + offsetDivident) / 2 + 1) / ((numPlayers + offsetDivisor) / 2 + 1))
		).rotateAround(angle, centerPosition).round();
	}

	return [primeSortAllPlayers(), playerPosition];
}

/**
 * Returns starting positions located on two parallel lines.
 * The locations on the first line are shifted in comparison to the other line.
 */
function playerPlacementLine(angle, center, width)
{
	let playerPosition = [];
	let numPlayers = getNumPlayers();

	for (let i = 0; i < numPlayers; ++i)
		playerPosition[i] = Vector2D.add(
			center,
			new Vector2D(
				fractionToTiles((i + 1) / (numPlayers + 1) - 0.5),
				width * (i % 2 - 1/2)
			).rotate(angle)
		).round();

	return playerPosition;
}

/**
 * Returns a random location for each player that meets the given constraints and
 * orders the playerIDs so that players become grouped by team.
 */
function playerPlacementRandom(playerIDs, constraints = undefined)
{
	let locations = [];
	let attempts = 0;
	let resets = 0;

	let mapCenter = g_Map.getCenter();
	let playerMinDistSquared = Math.square(fractionToTiles(0.25));
	let borderDistance = fractionToTiles(0.08);

	let area = createArea(new MapBoundsPlacer(), undefined, new AndConstraint(constraints));

	for (let i = 0; i < getNumPlayers(); ++i)
	{
		let position = pickRandom(area.getPoints());

		// Minimum distance between initial bases must be a quarter of the map diameter
		if (locations.some(loc => loc.distanceToSquared(position) < playerMinDistSquared) ||
		    position.distanceToSquared(mapCenter) > Math.square(mapCenter.x - borderDistance))
		{
			--i;
			++attempts;

			// Reset if we're in what looks like an infinite loop
			if (attempts > 500)
			{
				locations = [];
				i = -1;
				attempts = 0;
				++resets;

				// Reduce minimum player distance progressively
				if (resets % 25 == 0)
					playerMinDistSquared *= 0.95;

				// If we only pick bad locations, stop trying to place randomly
				if (resets == 500)
					return undefined;
			}
			continue;
		}

		locations[i] = position;
	}
	return groupPlayersByArea(playerIDs, locations);
}

/**
 *  Pick locations from the given set so that teams end up grouped.
 */
function groupPlayersByArea(playerIDs, locations)
{
	playerIDs = sortPlayers(playerIDs);

	let minDist = Infinity;
	let minLocations;

	// Of all permutations of starting locations, find the one where
	// the sum of the distances between allies is minimal, weighted by teamsize.
	heapsPermute(shuffleArray(locations).slice(0, playerIDs.length), v => v.clone(), permutation => {
		let dist = 0;
		let teamDist = 0;
		let teamSize = 0;

		for (let i = 1; i < playerIDs.length; ++i)
		{
			let team1 = getPlayerTeam(playerIDs[i - 1]);
			let team2 = getPlayerTeam(playerIDs[i]);
			++teamSize;
			if (team1 != -1 && team1 == team2)
				teamDist += permutation[i - 1].distanceTo(permutation[i]);
			else
			{
				dist += teamDist / teamSize;
				teamDist = 0;
				teamSize = 0;
			}
		}

		if (teamSize)
			dist += teamDist / teamSize;

		if (dist < minDist)
		{
			minDist = dist;
			minLocations = permutation;
		}
	});

	return [playerIDs, minLocations];
}

/**
 * Sorts the playerIDs so that team members are as close as possible on a ring.
 */
function groupPlayersCycle(startLocations)
{
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
