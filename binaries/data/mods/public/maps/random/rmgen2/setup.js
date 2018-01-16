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
	placePlayerBase({
		"playerID": player.id,
		"playerPosition": player.position,
		"PlayerTileClass": g_TileClasses.player,
		"BaseResourceClass": g_TileClasses.baseResource,
		"Walls": getMapSize() > 192 && walls,
		"CityPatch": {
			"outerTerrain": g_Terrains.roadWild,
			"innerTerrain": g_Terrains.road,
			"painters": [
				paintClass(g_TileClasses.player)
			]
		},
		"Chicken": {
			"template": g_Gaia.chicken
		},
		"Berries": {
			"template": g_Gaia.fruitBush
		},
		"Mines": {
			"types": [
				{ "template": g_Gaia.metalLarge },
				{ "template": g_Gaia.stoneLarge }
			]
		},
		"Trees": {
			"template": g_Gaia.tree1,
			"count": currentBiome() == "savanna" ? 5 : 15
		},
		"Decoratives": {
			"template": g_Decoratives.grassShort
		}
	});
}

/**
 * Return an array where each element is an array of playerIndices of a team.
 */
function getTeamsArray()
{
	var playerIDs = sortAllPlayers();
	var numPlayers = getNumPlayers();

	// Group players by team
	var teams = [];
	for (let i = 0; i < numPlayers; ++i)
	{
		let team = getPlayerTeam(playerIDs[i]);
		if (team == -1)
			continue;

		if (!teams[team])
			teams[team] = [];

		teams[team].push(playerIDs[i]);
	}

	// Players without a team get a custom index
	for (let i = 0; i < numPlayers; ++i)
		if (getPlayerTeam(playerIDs[i]) == -1)
			teams.push([playerIDs[i]]);

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
		"distance": fractionToTiles(randFloat(0.2, 0.35)),
		"separation": fractionToTiles(randFloat(0.05, 0.1))
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
	let players = [];
	let mapCenter = getMapCenter();
	let dist = fractionToTiles(0.45);

	for (let i = 0; i < teamsArray.length; ++i)
	{
		var safeDist = distance;
		if (distance + teamsArray[i].length * groupedDistance > dist)
			safeDist = dist - teamsArray[i].length * groupedDistance;

		var teamAngle = startAngle + (i + 1) * 2 * Math.PI / teamsArray.length;

		// Create player base
		for (let p = 0; p < teamsArray[i].length; ++p)
		{
			players[teamsArray[i][p]] = {
				"id": teamsArray[i][p],
				"position": Vector2D.add(mapCenter, new Vector2D(safeDist + p * groupedDistance, 0).rotate(-teamAngle)).round()
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
	let mapCenter = getMapCenter();
	let players = [];
	let numPlayers = getNumPlayers();

	for (let i = 0; i < numPlayers; ++i)
	{
		let angle = startAngle + i * 2 * Math.PI / numPlayers;
		players[i] = {
			"id": playerIDs[i],
			"position": Vector2D.add(mapCenter, new Vector2D(distance, 0).rotate(-angle)).round()
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
	var mapCenter = getMapCenter();

	for (let i = 0; i < getNumPlayers(); ++i)
	{
		let position = Vector2D.add(mapCenter, new Vector2D(fractionToTiles(randFloat(0, 0.42)), 0).rotate(randomAngle())).round();

		// Minimum distance between initial bases must be a quarter of the map diameter
		if (locations.some(loc => loc.distanceTo(position) < fractionToTiles(0.25)))
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

		locations[i] = position;
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
 *  @param {Array} locations - array of Vector2D of possible starting locations.
 */
function groupPlayersByLocations(playerIDs, locations)
{
	playerIDs = sortPlayers(playerIDs);

	let minDist = Infinity;
	let minLocations;

	// Of all permutations of starting locations, find the one where
	// the sum of the distances between allies is minimal, weighted by teamsize.
	heapsPermute(shuffleArray(locations).slice(0, playerIDs.length), v => v.clone(), permutation =>
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

	let players = [];
	for (let i = 0; i < playerIDs.length; ++i)
		players[i] = {
			"id": playerIDs[i],
			"position": minLocations[i]
		};

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
	var mapCenter = getMapCenter();

	for (let i = 0; i < teamsArray.length; ++i)
	{
		var teamAngle = startAngle + (i + 1) * 2 * Math.PI / teamsArray.length;
		var teamPosition = Vector2D.add(mapCenter, new Vector2D(distance, 0).rotate(-teamAngle));
		var teamGroupDistance = groupedDistance;

		// If we have a team of above average size, make sure they're spread out
		if (teamsArray[i].length > 4)
			teamGroupDistance = Math.max(fractionToTiles(0.08), groupedDistance);

		// If we have a solo player, place them on the center of the team's location
		if (teamsArray[i].length == 1)
			teamGroupDistance = fractionToTiles(0);

		// TODO: Ensure players are not placed outside of the map area, similar to placeLine

		// Create player base
		for (var p = 0; p < teamsArray[i].length; ++p)
		{
			var angle = startAngle + (p + 1) * 2 * Math.PI / teamsArray[i].length;
			players[teamsArray[i][p]] = {
				"id": teamsArray[i][p],
				"position": Vector2D.add(teamPosition, new Vector2D(teamGroupDistance, 0).rotate(-angle)).round()
			};
			createBase(players[teamsArray[i][p]], false);
		}
	}

	return players;
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
