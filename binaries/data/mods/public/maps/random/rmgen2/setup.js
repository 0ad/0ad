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
	"bluffIgnore", // performance improvement
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

var g_PlayerbaseTypes = {
	"line": {
		"getPosition": (distance, groupedDistance, startAngle) => placeLine(getTeamsArray(), distance, groupedDistance, startAngle),
		"distance": fractionToTiles(randFloat(0.2, 0.35)),
		"groupedDistance": fractionToTiles(randFloat(0.08, 0.1)),
		"walls": false
	},
	"radial": {
		"getPosition": (distance, groupedDistance, startAngle) => playerPlacementCircle(distance, startAngle),
		"distance": fractionToTiles(randFloat(0.25, 0.35)),
		"groupedDistance": fractionToTiles(randFloat(0.08, 0.1)),
		"walls": true
	},
	"randomGroup": {
		"getPosition": (distance, groupedDistance, startAngle) => playerPlacementRandom(sortAllPlayers()) || playerPlacementCircle(distance, startAngle),
		"distance": fractionToTiles(randFloat(0.25, 0.35)),
		"groupedDistance": fractionToTiles(randFloat(0.08, 0.1)),
		"walls": true
	},
	"stronghold": {
		"getPosition": (distance, groupedDistance, startAngle) => placeStronghold(getTeamsArray(), distance, groupedDistance, startAngle),
		"distance": fractionToTiles(randFloat(0.2, 0.35)),
		"groupedDistance": fractionToTiles(randFloat(0.08, 0.1)),
		"walls": false
	}
};

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
 * Choose starting locations for all players.
 *
 * @param {string} type - "radial", "line", "stronghold", "randomGroup"
 * @param {number} distance - radial distance from the center of the map
 * @param {number} groupedDistance - space between players within a team
 * @param {number} startAngle - determined by the map that might want to place something between players
 * @returns {Array|undefined} - If successful, each element is an object that contains id, angle, x, z for each player
 */
function createBasesByPattern(type, distance, groupedDistance, startAngle)
{
	return createBases(...g_PlayerbaseTypes[type].getPosition(distance, groupedDistance, startAngle), g_PlayerbaseTypes[type].walls);
}

function createBases(playerIDs, playerPosition, walls)
{
	g_Map.log("Creating bases");

	for (let i = 0; i < getNumPlayers(); ++i)
		createBase(playerIDs[i], playerPosition[i], walls);

	return [playerIDs, playerPosition];
}

/**
 * Create the base for a single player.
 *
 * @param {Object} player - contains id, angle, x, z
 * @param {boolean} walls - Whether or not iberian gets starting walls
 */
function createBase(playerID, playerPosition, walls)
{
	placePlayerBase({
		"playerID": playerID,
		"playerPosition": playerPosition,
		"PlayerTileClass": g_TileClasses.player,
		"BaseResourceClass": g_TileClasses.baseResource,
		"baseResourceConstraint": avoidClasses(g_TileClasses.water, 0, g_TileClasses.mountain, 0),
		"Walls": g_Map.getSize() > 192 && walls,
		"CityPatch": {
			"outerTerrain": g_Terrains.roadWild,
			"innerTerrain": g_Terrains.road,
			"painters": [
				new TileClassPainter(g_TileClasses.player)
			]
		},
		"StartingAnimal": {
			"template": g_Gaia.startingAnimal
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
			"count": currentBiome() == "generic/savanna" ? 5 : 15
		},
		"Decoratives": {
			"template": g_Decoratives.grassShort
		}
	});
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

	if (newClasses)
		classNames = classNames.concat(newClasses);

	g_TileClasses = {};
	for (var className of classNames)
		g_TileClasses[className] = g_Map.createTileClass();
}
