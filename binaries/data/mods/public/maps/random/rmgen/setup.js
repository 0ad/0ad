const g_Amounts = {
	"scarce": 0.2,
	"few": 0.5,
	"normal": 1,
	"many": 1.75,
	"tons": 3
};

const g_Mixes = {
	"same": 0,
	"similar": 0.1,
	"normal": 0.25,
	"varied": 0.5,
	"unique": 0.75
};

const g_Sizes = {
	"tiny": 0.5,
	"small": 0.75,
	"normal": 1,
	"big": 1.25,
	"huge": 1.5,
};

const g_AllAmounts = Object.keys(g_Amounts);
const g_AllMixes = Object.keys(g_Mixes);
const g_AllSizes = Object.keys(g_Sizes);

const g_DefaultTileClasses = [
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

var g_MapInfo;
var g_TileClasses;

var g_Terrains;
var g_Gaia;
var g_Decoratives;
var g_Forests;

/**
 * Adds an array of elements to the map.
 */
function addElements(els)
{
	for (var i = 0; i < els.length; ++i)
	{
		var stay = null;
		if (els[i].stay !== undefined)
			stay = els[i].stay;

		els[i].func(
			[avoidClasses.apply(null, els[i].avoid), stayClasses.apply(null, stay)],
			pickSize(els[i].sizes),
			pickMix(els[i].mixes),
			pickAmount(els[i].amounts)
		);
	}
}

/**
 * Converts "amount" terms to numbers.
 */
function pickAmount(amounts)
{
	var amount = amounts[randInt(amounts.length)];

	if (amount in g_Amounts)
		return g_Amounts[amount];

	return g_Mixes.normal;
}

/**
 * Converts "mix" terms to numbers.
 */
function pickMix(mixes)
{
	var mix = mixes[randInt(mixes.length)];

	if (mix in g_Mixes)
		return g_Mixes[mix];

	return g_Mixes.normal;
}

/**
 * Converts "size" terms to numbers.
 */
function pickSize(sizes)
{
	var size = sizes[randInt(sizes.length)];

	if (size in g_Sizes)
		return g_Sizes[size];

	return g_Sizes.normal;
}

/**
 * Paints the entire map with a single tile type.
 */
function resetTerrain(terrain, tc, elevation)
{
	g_MapInfo.mapSize = getMapSize();
	g_MapInfo.mapArea = g_MapInfo.mapSize * g_MapInfo.mapSize;
	g_MapInfo.centerOfMap = Math.floor(g_MapInfo.mapSize / 2);
	g_MapInfo.mapRadius = -PI / 4;

	var placer = new ClumpPlacer(g_MapInfo.mapArea, 1, 1, 1, g_MapInfo.centerOfMap, g_MapInfo.centerOfMap);
	var terrainPainter = new LayeredPainter([terrain], []);
	var elevationPainter = new SmoothElevationPainter(ELEVATION_SET, elevation, 1);
	createArea(placer, [terrainPainter, elevationPainter, paintClass(tc)], null);

	g_MapInfo.mapHeight = elevation;
}

/**
 * Euclidian distance between two points.
 */
function euclid_distance(x1, z1, x2, z2)
{
	return Math.sqrt(Math.pow(x2 - x1, 2) + Math.pow(z2 - z1, 2));
}

/**
 * Chose starting locations for the given players.
 *
 * @param {string} type - "radial", "stacked", "stronghold", "random"
 * @param {number} distance - radial distance from the center of the map
 */
function addBases(type, distance, groupedDistance)
{
	type = type || "radial";
	distance = distance || 0.3;
	groupedDistance = groupedDistance || 0.05;

	var playerIDs = randomizePlayers();
	var players = {};

	switch(type)
	{
		case "line":
			players = placeLine(playerIDs, distance, groupedDistance);
			break;
		case "radial":
			players = placeRadial(playerIDs, distance);
			break;
		case "random":
			players = placeRandom(playerIDs);
			break;
		case "stronghold":
			players = placeStronghold(playerIDs, distance, groupedDistance);
			break;
	}

	return players;
}

/**
 * Create the base for a single player.
 *
 * @param {Object} - contains id, angle, x, z
 * @param {boolean} - Whether or not iberian gets starting walls
 */ 
function createBase(player, walls)
{
	// Get the x and z in tiles
	var fx = fractionToTiles(player.x);
	var fz = fractionToTiles(player.z);
	var ix = round(fx);
	var iz = round(fz);
	addToClass(ix, iz, g_TileClasses.player);
	addToClass(ix + 5, iz, g_TileClasses.player);
	addToClass(ix, iz + 5, g_TileClasses.player);
	addToClass(ix - 5, iz, g_TileClasses.player);
	addToClass(ix, iz - 5, g_TileClasses.player);

	// Create starting units
	if ((walls || walls === undefined) && g_MapInfo.mapSize > 192)
		placeCivDefaultEntities(fx, fz, player.id, g_MapInfo.mapRadius);
	else
		placeCivDefaultEntities(fx, fz, player.id, g_MapInfo.mapRadius, { 'iberWall': false });

	// Create the city patch
	var cityRadius = scaleByMapSize(15, 25) / 3;
	var placer = new ClumpPlacer(PI * cityRadius * cityRadius, 0.6, 0.3, 10, ix, iz);
	var painter = new LayeredPainter([g_Terrains.roadWild, g_Terrains.road], [1]);
	createArea(placer, painter, null);

	// Create initial berry bushes at random angle
	var bbAngle = randFloat(0, TWO_PI);
	var bbDist = 10;
	var bbX = round(fx + bbDist * cos(bbAngle));
	var bbZ = round(fz + bbDist * sin(bbAngle));
	group = new SimpleGroup(
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

	// Create initial chicken
	for (var j = 0; j < 2; ++j)
	{
		for (var tries = 0; tries < 10; ++tries)
		{
			var aAngle = randFloat(0, TWO_PI);
			var aDist = 9;
			var aX = round(fx + aDist * cos(aAngle));
			var aZ = round(fz + aDist * sin(aAngle));

			var group = new SimpleGroup(
				[new SimpleObject(g_Gaia.chicken, 5, 5, 0, 2)],
				true, g_TileClasses.baseResource, aX, aZ
			);

			if (createObjectGroup(group, 0, avoidClasses(g_TileClasses.baseResource, 4)))
				break;
		}
	}

	var hillSize = PI * g_MapInfo.mapRadius * g_MapInfo.mapRadius;

	// Create starting trees
	var num = 25;
	for (var tries = 0; tries < 10; ++tries)
	{
		var tAngle = randFloat(0, TWO_PI);
		var tDist = randFloat(12, 13);
		var tX = round(fx + tDist * cos(tAngle));
		var tZ = round(fz + tDist * sin(tAngle));
	
		group = new SimpleGroup(
			[new SimpleObject(g_Gaia.tree1, num, num, 0, 3)],
			false, g_TileClasses.baseResource, tX, tZ
		);

		if (createObjectGroup(group, 0, avoidClasses(g_TileClasses.baseResource, 4)))
			break;
	}

	// Create grass tufts
	var num = hillSize / 250;
	for (var j = 0; j < num; ++j)
	{
		var gAngle = randFloat(0, TWO_PI);
		var gDist = g_MapInfo.mapRadius - (5 + randInt(7));
		var gX = round(fx + gDist * cos(gAngle));
		var gZ = round(fz + gDist * sin(gAngle));
		group = new SimpleGroup(
			[new SimpleObject(g_Decoratives.grassShort, 2, 5, 0, 1, -PI / 8, PI / 8)],
			false, g_TileClasses.baseResource, gX, gZ
		);
		createObjectGroup(group, 0, avoidClasses(g_TileClasses.baseResource, 4));
	}
}

/**
 * Return an array where each element is an array of playerIndices of a team.
 */
function getTeams(numPlayers)
{
	// Group players by team
	var teams = [];
	for (var i = 0; i < numPlayers; ++i)
	{
		let team = getPlayerTeam(i);
		if (team == -1)
			continue;

		if (!teams[team])
			teams[team] = [];

		teams[team].push(i+1);
	}

	// Players without a team get a custom index
	for (var i = 0; i < numPlayers; ++i)
		if (getPlayerTeam(i) == -1)
			teams.push([i+1]);

	// Remove unused indices
	return teams.filter(team => true);
}

/**
 * Chose a random pattern for placing the bases of the players.
 */ 
function randomStartingPositionPattern()
{
	var formats = ["radial"];

	// Enable stronghold if we have a few teams and a big enough map
	if (g_MapInfo.teams.length >= 2 && g_MapInfo.numPlayers >= 4 && g_MapInfo.mapSize >= 256)
		formats.push("stronghold");

	// Enable random if we have enough teams or enough players on a big enough map
	if (g_MapInfo.mapSize >= 256 && (g_MapInfo.teams.length >= 3 || g_MapInfo.numPlayers > 4))
		formats.push("random");

	// Enable line if we have enough teams and players on a big enough map
	if (g_MapInfo.teams.length >= 2 && g_MapInfo.numPlayers >= 4 && g_MapInfo.mapSize >= 384)
		formats.push("line");

	return {
		"setup": formats[randInt(formats.length)],
		"distance": randFloat(0.2, 0.35),
		"separation": randFloat(0.05, 0.1)
	};
}

/**
 * Mix player indices but sort by team.
 *
 * @returns {Array} - every item is an array of player indices
 */
function randomizePlayers()
{
	var playerIDs = [];
	for (var i = 0; i < g_MapInfo.numPlayers; ++i)
		playerIDs.push(i + 1);

	return sortPlayers(playerIDs);
}

/**
 * Place teams in a line-pattern.
 *
 * @param {Array} playerIDs - typically randomized indices of players of a single team
 * @param {number} distance - radial distance from the center of the map
 * @param {number} groupedDistance - distance between players
 *
 * @returns {Array} - contains id, angle, x, z for every player
 */
function placeLine(playerIDs, distance, groupedDistance)
{
	var players = [];

	for (var i = 0; i < g_MapInfo.teams.length; ++i)
	{
		var safeDist = distance;
		if (distance + g_MapInfo.teams[i].length * groupedDistance > 0.45)
			safeDist = 0.45 - g_MapInfo.teams[i].length * groupedDistance;

		var teamAngle = g_MapInfo.startAngle + (i + 1) * TWO_PI / g_MapInfo.teams.length;

		// Create player base
		for (var p = 0; p < g_MapInfo.teams[i].length; ++p)
		{
			players[g_MapInfo.teams[i][p]] = {
				"id": g_MapInfo.teams[i][p],
				"angle": g_MapInfo.startAngle + (p + 1) * TWO_PI / g_MapInfo.teams[i].length,
				"x": 0.5 + (safeDist + p * groupedDistance) * cos(teamAngle),
				"z": 0.5 + (safeDist + p * groupedDistance) * sin(teamAngle)
			};
			createBase(players[g_MapInfo.teams[i][p]], false);
		}
	}

	return players;
}

/**
 * Place players in a circle-pattern.
 *
 * @param {number} distance - radial distance from the center of the map
 */
function placeRadial(playerIDs, distance)
{
	var players = new Array(g_MapInfo.numPlayers);

	for (var i = 0; i < g_MapInfo.numPlayers; ++i)
	{
		var angle = g_MapInfo.startAngle + i * TWO_PI / g_MapInfo.numPlayers;
		players[i] = {
			"id": playerIDs[i],
			"angle": angle,
			"x": 0.5 + distance * cos(angle),
			"z": 0.5 + distance * sin(angle)
		};
		createBase(players[i]);
	}

	return players;
}

/**
 * Chose arbitrary starting locations.
 */
function placeRandom(playerIDs)
{
	var players = [];
	var placed = [];

	for (var i = 0; i < g_MapInfo.numPlayers; ++i)
	{
		var attempts = 0;
		var playerAngle = randFloat(0, TWO_PI);
		var distance = randFloat(0, 0.42);
		var x = 0.5 + distance * cos(playerAngle);
		var z = 0.5 + distance * sin(playerAngle);

		var tooClose = false;
		for (var j = 0; j < placed.length; ++j)
		{
			var sep = euclid_distance(x, z, placed[j].x, placed[j].z);
			if (sep < 0.25)
			{
				tooClose = true;
				break;
			}
		}

		if (tooClose)
		{
			--i;
			++attempts;

			// Reset if we're in what looks like an infinite loop
			if (attempts > 100)
			{
				players = [];
				placed = [];
				i = -1;
				attempts = 0;
			}

			continue;
		}

		players[i] = {
			"id": playerIDs[i],
			"angle": playerAngle,
			"x": x,
			"z": z
		};

		placed.push(players[i]);
	}

	// Create the bases
	for (var i = 0; i < g_MapInfo.numPlayers; ++i)
		createBase(players[i]);

	return players;
}

/**
 * Place given players in a stronghold-pattern.
 *
 * @param distance - radial distance from the center of the map
 * @param groupedDistance - distance between neighboring players
 */
function placeStronghold(playerIDs, distance, groupedDistance)
{
	var players = [];

	for (var i = 0; i < g_MapInfo.teams.length; ++i)
	{
		var teamAngle = g_MapInfo.startAngle + (i + 1) * TWO_PI / g_MapInfo.teams.length;
		var fractionX = 0.5 + distance * cos(teamAngle);
		var fractionZ = 0.5 + distance * sin(teamAngle);

		// If we have a team of above average size, make sure they're spread out
		if (g_MapInfo.teams[i].length > 4)
			groupedDistance = randFloat(0.08, 0.12);

		// If we have a team of below average size, make sure they're together
		if (g_MapInfo.teams[i].length < 3)
			groupedDistance = randFloat(0.04, 0.06);

		// If we have a solo player, place them on the center of the team's location
		if (g_MapInfo.teams[i].length == 1)
			groupedDistance = 0;

		// Create player base
		for (var p = 0; p < g_MapInfo.teams[i].length; ++p)
		{
			var angle = g_MapInfo.startAngle + (p + 1) * TWO_PI / g_MapInfo.teams[i].length;
			players[g_MapInfo.teams[i][p]] = {
				"id": g_MapInfo.teams[i][p],
				"angle": angle,
				"x": fractionX + groupedDistance * cos(angle),
				"z": fractionZ + groupedDistance * sin(angle)
			};
			createBase(players[g_MapInfo.teams[i][p]], false);
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

/**
 * Get biome-specific names of entities and terrain after randomization.
 */
function initBiome()
{
	g_Terrains = {
		"mainTerrain": rBiomeT1(),
		"forestFloor1": rBiomeT2(),
		"forestFloor2": rBiomeT3(),
		"cliff": rBiomeT4(),
		"tier1Terrain": rBiomeT5(),
		"tier2Terrain": rBiomeT6(),
		"tier3Terrain": rBiomeT7(),
		"hill": rBiomeT8(),
		"dirt": rBiomeT9(),
		"road": rBiomeT10(),
		"roadWild": rBiomeT11(),
		"tier4Terrain": rBiomeT12(),
		"shoreBlend": rBiomeT13(),
		"shore": rBiomeT14(),
		"water": rBiomeT15()
	};

	g_Gaia = {
		"tree1": rBiomeE1(),
		"tree2": rBiomeE2(),
		"tree3": rBiomeE3(),
		"tree4": rBiomeE4(),
		"tree5": rBiomeE5(),
		"fruitBush": rBiomeE6(),
		"chicken": rBiomeE7(),
		"mainHuntableAnimal": rBiomeE8(),
		"fish": rBiomeE9(),
		"secondaryHuntableAnimal": rBiomeE10(),
		"stoneLarge": rBiomeE11(),
		"stoneSmall": rBiomeE12(),
		"metalLarge": rBiomeE13()
	};

	g_Decoratives = {
		"grass": rBiomeA1(),
		"grassShort": rBiomeA2(),
		"reeds": rBiomeA3(),
		"lillies": rBiomeA4(),
		"rockLarge": rBiomeA5(),
		"rockMedium": rBiomeA6(),
		"bushMedium": rBiomeA7(),
		"bushSmall": rBiomeA8(),
		"tree": rBiomeA9()
	};

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

/**
 * Creates an object of commonly used functions.
 */
function initMapSettings()
{
	initBiome();

	let numPlayers = getNumPlayers();
	g_MapInfo = {
		"biome": biomeID,
		"numPlayers": numPlayers,
		"teams": getTeams(numPlayers),
		"startAngle": randFloat(0, TWO_PI)
	};
}
