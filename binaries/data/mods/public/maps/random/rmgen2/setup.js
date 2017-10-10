var g_Amounts = {
	"scarce": 0.2,
	"few": 0.5,
	"normal": 1,
	"many": 1.75,
	"tons": 3
};

var g_Mixes = {
	"same": 0,
	"similar": 0.1,
	"normal": 0.25,
	"varied": 0.5,
	"unique": 0.75
};

var g_Sizes = {
	"tiny": 0.5,
	"small": 0.75,
	"normal": 1,
	"big": 1.25,
	"huge": 1.5,
};

var g_AllAmounts = Object.keys(g_Amounts);
var g_AllMixes = Object.keys(g_Mixes);
var g_AllSizes = Object.keys(g_Sizes);

var g_DefaultTileClasses = [
	"animals",
	"baseResource",
	"berries",
	"bluff",
	"bluffSlope",
	"dirt",
	"fish",
	"food",
	"forest",
	"hill",
	"land",
	"map",
	"metal",
	"mountain",
	"plateau",
	"player",
	"prop",
	"ramp",
	"rock",
	"settlement",
	"spine",
	"valley",
	"water"
];

var g_TileClasses;
var g_Forests;

/**
 * Adds an array of elements to the map.
 */
function addElements(elements)
{
	for (let element of elements)
		element.func(
			[
				avoidClasses.apply(null, element.avoid),
				stayClasses.apply(null, element.stay || null)
			],
			pickSize(element.sizes),
			pickMix(element.mixes),
			pickAmount(element.amounts),
			element.baseHeight || 0);
}

/**
 * Converts "amount" terms to numbers.
 */
function pickAmount(amounts)
{
	let amount = pickRandom(amounts);

	if (amount in g_Amounts)
		return g_Amounts[amount];

	return g_Amounts.normal;
}

/**
 * Converts "mix" terms to numbers.
 */
function pickMix(mixes)
{
	let mix = pickRandom(mixes);

	if (mix in g_Mixes)
		return g_Mixes[mix];

	return g_Mixes.normal;
}

/**
 * Converts "size" terms to numbers.
 */
function pickSize(sizes)
{
	let size = pickRandom(sizes);

	if (size in g_Sizes)
		return g_Sizes[size];

	return g_Sizes.normal;
}

/**
 * Paints the entire map with the given terrain texture, tileclass and elevation.
 */
function resetTerrain(terrain, tileClass, elevation)
{
	let center = Math.round(fractionToTiles(0.5));
	createArea(
		new ClumpPlacer(getMapArea(), 1, 1, 1, center, center),
		[
			new LayeredPainter([terrain], []),
			new SmoothElevationPainter(ELEVATION_SET, elevation, 1),
			paintClass(tileClass)
		],
		null);
}

/**
 * Choose starting locations for all players.
 *
 * @param {string} type - "radial", "line", "stronghold", "random"
 * @param {number} distance - radial distance from the center of the map
 * @param {number} groupedDistance - space between players within a team
 * @param {number} startAngle - determined by the map that might want to place something between players
 * @returns {Array|undefined} - If successful, each element is an object that contains id, angle, x, z for each player
 */
function addBases(type, distance, groupedDistance, startAngle)
{
	let playerIDs = sortAllPlayers();
	let teamsArray = getTeamsArray();

	switch(type)
	{
		case "line":
			return placeLine(teamsArray, distance, groupedDistance, startAngle);
		case "radial":
			return placeRadial(playerIDs, distance, startAngle);
		case "random":
			return placeRandom(playerIDs) || placeRadial(playerIDs, distance, startAngle);
		case "stronghold":
			return placeStronghold(teamsArray, distance, groupedDistance, startAngle);
		default:
			warn("Unknown base placement type:" + type);
			return undefined;
	}
}

/**
 * Create the base for a single player.
 *
 * @param {Object} player - contains id, angle, x, z
 * @param {boolean} walls - Whether or not iberian gets starting walls
 */
function createBase(player, walls = true)
{
	var mapSize = getMapSize();

	// Get the x and z in tiles
	var fx = fractionToTiles(player.x);
	var fz = fractionToTiles(player.z);
	var ix = round(fx);
	var iz = round(fz);

	addCivicCenterAreaToClass(ix, iz, g_TileClasses.player);

	if (walls && mapSize > 192)
		placeCivDefaultEntities(fx, fz, player.id);
	else
		placeCivDefaultEntities(fx, fz, player.id, { 'iberWall': false });

	// Create the city patch
	var radius = scaleByMapSize(15, 25);
	var cityRadius = radius / 3;
	var placer = new ClumpPlacer(PI * cityRadius * cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([g_Terrains.roadWild, g_Terrains.road], [1]);
	createArea(placer, painter, null);

	// TODO: retry loops are needed as resources might conflict with neighboring ones

	// Create initial berry bushes at random angle
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 10;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	var group = new SimpleGroup(
		[new SimpleObject(g_Gaia.fruitBush, 5, 5, 0, 3)],
		true, g_TileClasses.baseResource, bbX, bbZ
	);
	createObjectGroup(group, 0, avoidClasses(g_TileClasses.baseResource, 2));

	// Create metal mine at a different angle
	var mAngle = bbAngle;
	while(abs(mAngle - bbAngle) < PI / 3)
		mAngle = randFloat(0, TWO_PI);

	var mDist = 12;
	var mX = round(fx + mDist * cos(mAngle));
	var mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(g_Gaia.metalLarge, 1, 1, 0, 0)],
		true, g_TileClasses.baseResource, mX, mZ
	);
	createObjectGroup(group, 0, avoidClasses(g_TileClasses.baseResource, 2));

	// Create stone mine beside metal
	mAngle += randFloat(PI / 8, PI / 4);
	mX = round(fx + mDist * cos(mAngle));
	mZ = round(fz + mDist * sin(mAngle));
	group = new SimpleGroup(
		[new SimpleObject(g_Gaia.stoneLarge, 1, 1, 0, 2)],
		true, g_TileClasses.baseResource, mX, mZ
	);
	createObjectGroup(group, 0, avoidClasses(g_TileClasses.baseResource, 2));

	placeDefaultChicken(
		fx,
		fz,
		g_TileClasses.baseResource,
		avoidClasses(g_TileClasses.baseResource, 4),
		g_Gaia.chicken
	);

	// Create starting trees
	var num = currentBiome() == "savanna" ? 5 : 15;
	for (var tries = 0; tries < 10; ++tries)
	{
		var tAngle = randFloat(0, TWO_PI);
		var tDist = randFloat(12, 13);
		var tX = round(fx + tDist * cos(tAngle));
		var tZ = round(fz + tDist * sin(tAngle));

		group = new SimpleGroup(
			[new SimpleObject(g_Gaia.tree1, num, num, 1, 3)],
			false, g_TileClasses.baseResource, tX, tZ
		);

		if (createObjectGroup(group, 0, avoidClasses(g_TileClasses.baseResource, 4)))
			break;
	}

	placeDefaultDecoratives(
		fx,
		fz,
		g_Decoratives.grassShort,
		g_TileClasses.baseResource,
		radius,
		avoidClasses(g_TileClasses.baseResource, 4));
}

/**
 * Return an array where each element is an array of playerIndices of a team.
 */
function getTeamsArray()
{
	var numPlayers = getNumPlayers();

	// Group players by team
	var teams = [];
	for (let i = 0; i < numPlayers; ++i)
	{
		let team = getPlayerTeam(i);
		if (team == -1)
			continue;

		if (!teams[team])
			teams[team] = [];

		teams[team].push(i+1);
	}

	// Players without a team get a custom index
	for (let i = 0; i < numPlayers; ++i)
		if (getPlayerTeam(i) == -1)
			teams.push([i+1]);

	// Remove unused indices
	return teams.filter(team => true);
}

/**
 * Choose a random pattern for placing the bases of the players.
 */
function randomStartingPositionPattern(teamsArray)
{
	var formats = ["radial"];
	var mapSize = getMapSize();
	var numPlayers = getNumPlayers();

	// Enable stronghold if we have a few teams and a big enough map
	if (teamsArray.length >= 2 && numPlayers >= 4 && mapSize >= 256)
		formats.push("stronghold");

	// Enable random if we have enough teams or enough players on a big enough map
	if (mapSize >= 256 && (teamsArray.length >= 3 || numPlayers > 4))
		formats.push("random");

	// Enable line if we have enough teams and players on a big enough map
	if (teamsArray.length >= 2 && numPlayers >= 4 && mapSize >= 384)
		formats.push("line");

	return {
		"setup": pickRandom(formats),
		"distance": randFloat(0.2, 0.35),
		"separation": randFloat(0.05, 0.1)
	};
}

/**
 * Place teams in a line-pattern.
 *
 * @param {Array} playerIDs - typically randomized indices of players of a single team
 * @param {number} distance - radial distance from the center of the map
 * @param {number} groupedDistance - distance between players
 * @param {number} startAngle - determined by the map that might want to place something between players.
 *
 * @returns {Array} - contains id, angle, x, z for every player
 */
function placeLine(teamsArray, distance, groupedDistance, startAngle)
{
	var players = [];

	for (let i = 0; i < teamsArray.length; ++i)
	{
		var safeDist = distance;
		if (distance + teamsArray[i].length * groupedDistance > 0.45)
			safeDist = 0.45 - teamsArray[i].length * groupedDistance;

		var teamAngle = startAngle + (i + 1) * 2 * Math.PI / teamsArray.length;

		// Create player base
		for (var p = 0; p < teamsArray[i].length; ++p)
		{
			players[teamsArray[i][p]] = {
				"id": teamsArray[i][p],
				"x": 0.5 + (safeDist + p * groupedDistance) * cos(teamAngle),
				"z": 0.5 + (safeDist + p * groupedDistance) * sin(teamAngle)
			};
			createBase(players[teamsArray[i][p]], false);
		}
	}

	return players;
}

/**
 * Place players in a circle-pattern.
 *
 * @param {Array} playerIDs - order of playerIDs to be placed
 * @param {number} distance - radial distance from the center of the map
 * @param {number} startAngle - determined by the map that might want to place something between players
 */
function placeRadial(playerIDs, distance, startAngle)
{
	let players = [];
	let numPlayers = getNumPlayers();

	for (let i = 0; i < numPlayers; ++i)
	{
		let angle = startAngle + i * 2 * Math.PI / numPlayers;
		players[i] = {
			"id": playerIDs[i],
			"x": 0.5 + distance * cos(angle),
			"z": 0.5 + distance * sin(angle)
		};
		createBase(players[i]);
	}

	return players;
}

/**
 * Choose arbitrary starting locations.
 */
function placeRandom(playerIDs)
{
	var locations = [];
	var attempts = 0;
	var resets = 0;

	for (let i = 0; i < getNumPlayers(); ++i)
	{
		var playerAngle = randFloat(0, TWO_PI);

		// Distance from the center of the map in percent
		// Mapsize being used as a diameter, so 0.5 is the edge of the map
		var distance = randFloat(0, 0.42);
		var x = 0.5 + distance * cos(playerAngle);
		var z = 0.5 + distance * sin(playerAngle);

		// Minimum distance between initial bases must be a quarter of the map diameter
		if (locations.some(loc => getDistance(x, z, loc.x, loc.z) < 0.25))
		{
			--i;
			++attempts;

			// Reset if we're in what looks like an infinite loop
			if (attempts > 100)
			{
				locations = [];
				i = -1;
				attempts = 0;
				++resets;

				// If we only pick bad locations, stop trying to place randomly
				if (resets == 100)
					return undefined;
			}
			continue;
		}

		locations[i] = {
			"x": x,
			"z": z
		};
	}

	let players = groupPlayersByLocations(playerIDs, locations);
	for (let player of players)
		createBase(player);

	return players;
}

/**
 *  Pick locations from the given set so that teams end up grouped.
 *
 *  @param {Array} playerIDs - sorted by teams.
 *  @param {Array} locations - array of x/z pairs of possible starting locations.
 */
function groupPlayersByLocations(playerIDs, locations)
{
	playerIDs = sortPlayers(playerIDs);

	let minDist = Infinity;
	let minLocations;

	// Of all permutations of starting locations, find the one where
	// the sum of the distances between allies is minimal, weighted by teamsize.
	heapsPermute(shuffleArray(locations).slice(0, playerIDs.length), function(permutation)
	{
		let dist = 0;
		let teamDist = 0;
		let teamSize = 0;

		for (let i = 1; i < playerIDs.length; ++i)
		{
			let team1 = g_MapSettings.PlayerData[playerIDs[i - 1]].Team;
			let team2 = g_MapSettings.PlayerData[playerIDs[i]].Team;
			++teamSize;

			if (team1 != -1 && team1 == team2)
				teamDist += getDistance(permutation[i - 1].x, permutation[i - 1].z, permutation[i].x, permutation[i].z);
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

	let players = [];
	for (let i = 0; i < playerIDs.length; ++i)
	{
		let player = minLocations[i];
		player.id = playerIDs[i];
		players.push(player);
	}
	return players;
}

/**
 * Place given players in a stronghold-pattern.
 *
 * @param teamsArray - each item is an array of playerIDs placed per stronghold
 * @param distance - radial distance from the center of the map
 * @param groupedDistance - distance between neighboring players
 * @param {number} startAngle - determined by the map that might want to place something between players
 */
function placeStronghold(teamsArray, distance, groupedDistance, startAngle)
{
	var players = [];

	for (let i = 0; i < teamsArray.length; ++i)
	{
		var teamAngle = startAngle + (i + 1) * 2 * Math.PI / teamsArray.length;
		var fractionX = 0.5 + distance * cos(teamAngle);
		var fractionZ = 0.5 + distance * sin(teamAngle);
		var teamGroupDistance = groupedDistance;

		// If we have a team of above average size, make sure they're spread out
		if (teamsArray[i].length > 4)
			teamGroupDistance = Math.max(0.08, groupedDistance);

		// If we have a solo player, place them on the center of the team's location
		if (teamsArray[i].length == 1)
			teamGroupDistance = 0;

		// TODO: Ensure players are not placed outside of the map area, similar to placeLine

		// Create player base
		for (var p = 0; p < teamsArray[i].length; ++p)
		{
			var angle = startAngle + (p + 1) * 2 * Math.PI / teamsArray[i].length;
			players[teamsArray[i][p]] = {
				"id": teamsArray[i][p],
				"x": fractionX + teamGroupDistance * cos(angle),
				"z": fractionZ + teamGroupDistance * sin(angle)
			};
			createBase(players[teamsArray[i][p]], false);
		}
	}

	return players;
}

/**
 * Places players either randomly or in a stronghold-pattern at a set of given heightmap coordinates.
 *
 * @param teamsArray - Array where each item is an array of playerIDs, possibly going to be grouped.
 * @param singleBases - pair of coordinates of the heightmap to place isolated bases.
 * @param singleBases - pair of coordinates of the heightmap to place team bases.
 * @param groupedDistance - distance between neighboring players.
 * @param func - A function called for every player base or stronghold placed.
 */
function randomPlayerPlacementAt(teamsArray, singleBases, strongholdBases, heightmapScale, groupedDistance, func)
{
	let strongholdBasesRandom = shuffleArray(strongholdBases);
	let mapSize = getMapSize();

	if (randBool(1/3) &&
	    mapSize >= 256 &&
	    teamsArray.length >= 2 &&
	    teamsArray.length < getNumPlayers() &&
	    teamsArray.length <= strongholdBasesRandom.length)
	{
		let startAngle = randFloat(0, 2 * Math.PI);

		for (let t = 0; t < teamsArray.length; ++t)
		{
			let tileX = Math.floor(strongholdBasesRandom[t][0] / heightmapScale);
			let tileY = Math.floor(strongholdBasesRandom[t][1] / heightmapScale);

			let x = tileX / mapSize;
			let z = tileY / mapSize;

			let team = teamsArray[t].map(playerID => ({ "id": playerID }));
			let players = [];

			if (func)
				func(tileX, tileY);

			for (let p = 0; p < team.length; ++p)
			{
				let angle = startAngle + (p + 1) * TWO_PI / team.length;

				players[p] = {
					"id": team[p].id,
					"x": x + groupedDistance * cos(angle),
					"z": z + groupedDistance * sin(angle)
				};

				createBase(players[p], false);
			}
		}
	}
	else
	{
		let players = groupPlayersByLocations(sortAllPlayers(), singleBases.map(l => ({
			"x": l[0] / heightmapScale / mapSize,
			"z": l[1] / heightmapScale / mapSize
		})));

		for (let player of players)
		{
			if (func)
				func(Math.floor(player.x * mapSize), Math.floor(player.z * mapSize));

			createBase(player);
		}
	}
}

/**
 * Creates tileClass for the default classes and every class given.
 *
 * @param {Array} newClasses
 * @returns {Object} - maps from classname to ID
 */
function initTileClasses(newClasses)
{
	var classNames = g_DefaultTileClasses;

	if (newClasses !== undefined)
		classNames = classNames.concat(newClasses);

	g_TileClasses = {};
	for (var className of classNames)
		g_TileClasses[className] = createTileClass();
}

/**
 * Get biome-specific names of entities and terrain after randomization.
 */
function initForestFloor()
{
	g_Forests = {
		"forest1": [
			g_Terrains.forestFloor2 + TERRAIN_SEPARATOR + g_Gaia.tree1,
			g_Terrains.forestFloor2 + TERRAIN_SEPARATOR + g_Gaia.tree2,
			g_Terrains.forestFloor2
		],
		"forest2": [
			g_Terrains.forestFloor1 + TERRAIN_SEPARATOR + g_Gaia.tree4,
			g_Terrains.forestFloor1 + TERRAIN_SEPARATOR + g_Gaia.tree5,
			g_Terrains.forestFloor1
		]
	};
}
