/**
 * @file These functions locate and place the starting entities of players.
 */

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
 * Places the default starting entities as defined by the civilization definition and walls for Iberians.
 */
function placeCivDefaultEntities(fx, fz, playerID, kwargs, dist = 6, orientation = BUILDING_ORIENTATION)
{
	placeStartingEntities(fx, fz, playerID, getStartingEntities(playerID), dist, orientation);

	let civ = getCivCode(playerID);
	if (civ == 'iber' && getMapSize() > 128)
	{
		if (kwargs && kwargs.iberWall == 'towers')
			placePolygonalWall(fx, fz, 15, ['entry'], 'tower', civ, playerID, orientation, 7);
		else if (!kwargs || kwargs.iberWall)
			placeGenericFortress(fx, fz, 20, playerID);
	}
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
function radialPlayerPlacement(radius = 0.35, startingAngle = undefined, centerX = 0.5, centerZ = 0.5)
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
