// Prepare progress calculation
var timeArray = [];
timeArray.push(new Date().getTime());

// Importing rmgen libraries
RMS.LoadLibrary("rmgen");

const BUILDING_ANGlE = -PI/4;

// initialize map

log("Initializing map...");

InitMap();

var numPlayers = getNumPlayers();
var mapSize = getMapSize();


//////////
// Heightmap functionality
//////////

// Some general heightmap settings
const MIN_HEIGHT = - SEA_LEVEL; // 20, should be set in the libs!
const MAX_HEIGHT = 0xFFFF/HEIGHT_UNITS_PER_METRE - SEA_LEVEL; // A bit smaler than 90, should be set in the libs!

// Add random heightmap generation functionality
function getRandomReliefmap(minHeight, maxHeight)
{
	minHeight = (minHeight || MIN_HEIGHT);
	maxHeight = (maxHeight || MAX_HEIGHT);
	if (minHeight < MIN_HEIGHT)
		warn("getRandomReliefmap: Argument minHeight is smaler then the supported minimum height of " + MIN_HEIGHT + " (const MIN_HEIGHT): " + minHeight)
	if (maxHeight > MAX_HEIGHT)
		warn("getRandomReliefmap: Argument maxHeight is smaler then the supported maximum height of " + MAX_HEIGHT + " (const MAX_HEIGHT): " + maxHeight)
	var reliefmap = [];
	for (var x = 0; x <= mapSize; x++)
	{
		reliefmap.push([]);
		for (var y = 0; y <= mapSize; y++)
		{
			reliefmap[x].push(randFloat(minHeight, maxHeight));
		}
	}
	return reliefmap;
}

// Apply a heightmap
function setReliefmap(reliefmap)
{
	// g_Map.height = reliefmap;
	for (var x = 0; x <= mapSize; x++)
	{
		for (var y = 0; y <= mapSize; y++)
		{
			setHeight(x, y, reliefmap[x][y]);
		}
	}
}

// Get minimum and maxumum height used in a heightmap
function getMinAndMaxHeight(reliefmap)
{
	var height = {};
	height.min = Infinity;
	height.max = -Infinity;
	for (var x = 0; x <= mapSize; x++)
	{
		for (var y = 0; y <= mapSize; y++)
		{
			if (reliefmap[x][y] < height.min)
				height.min = reliefmap[x][y];
			else if (reliefmap[x][y] > height.max)
				height.max = reliefmap[x][y];
		}
	}
	return height;
}

// Rescale a heightmap (Waterlevel is not taken into consideration!)
function getRescaledReliefmap(reliefmap, minHeight, maxHeight)
{
	var newReliefmap = deepcopy(reliefmap);
	minHeight = (minHeight || MIN_HEIGHT);
	maxHeight = (maxHeight || MAX_HEIGHT);
	if (minHeight < MIN_HEIGHT)
		warn("getRescaledReliefmap: Argument minHeight is smaler then the supported minimum height of " + MIN_HEIGHT + " (const MIN_HEIGHT): " + minHeight)
	if (maxHeight > MAX_HEIGHT)
		warn("getRescaledReliefmap: Argument maxHeight is smaler then the supported maximum height of " + MAX_HEIGHT + " (const MAX_HEIGHT): " + maxHeight)
	var oldHeightRange = getMinAndMaxHeight(reliefmap);
	for (var x = 0; x <= mapSize; x++)
	{
		for (var y = 0; y <= mapSize; y++)
		{
			newReliefmap[x][y] = minHeight + (reliefmap[x][y] - oldHeightRange.min) / (oldHeightRange.max - oldHeightRange.min) * (maxHeight - minHeight);
		}
	}
	return newReliefmap
}

// Applying decay errosion (terrain independent)
function getHeightErrosionedReliefmap(reliefmap, strength)
{
	var newReliefmap = deepcopy(reliefmap);
	strength = (strength || 1.0); // Values much higher then 1 (1.32+ for an 8 tile map, 1.45+ for a 12 tile map, 1.62+ @ 20 tile map, 0.99 @ 4 tiles) will result in a resonance disaster/self interference
	var map = [[1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1], [0, -1], [1, -1]]; // Default
	for (var x = 0; x <= mapSize; x++)
	{
		for (var y = 0; y <= mapSize; y++)
		{
			var div = 0;
			for (var i = 0; i < map.length; i++)
				newReliefmap[x][y] += strength / map.length * (reliefmap[(x + map[i][0] + mapSize + 1) % (mapSize + 1)][(y + map[i][1] + mapSize + 1) % (mapSize + 1)] - reliefmap[x][y]); // Not entirely sure if scaling with map.length is perfect but tested values seam to indicate it is
		}
	}
	return newReliefmap;
}


//////////
// Prepare for hightmap munipulation
//////////

// Set target min and max height depending on map size to make average stepness the same on all map sizes
var heightRange = {"min": MIN_HEIGHT * mapSize / 8192, "max": MAX_HEIGHT * mapSize / 8192};

// Set average water coverage
var averageWaterCoverage = 1/3; // NOTE: Since errosion is not predictable actual water coverage might differ much with the same value
if (mapSize < 200) // Sink the waterlevel on tiny maps to ensure enough space
	averageWaterCoverage = 2/3 * averageWaterCoverage;
var waterHeight = -MIN_HEIGHT + heightRange.min + averageWaterCoverage * (heightRange.max - heightRange.min);
var waterHeightAdjusted = waterHeight + MIN_HEIGHT;
setWaterHeight(waterHeight);


//////////
// Prepare terrain texture by height placement
//////////

var textueByHeight = [];

// Deep water
textueByHeight.push({"upperHeightLimit": heightRange.min + 1/3 * (waterHeightAdjusted - heightRange.min), "terrain": "temp_sea_rocks"});
// Medium deep water (with fish)
var terreins = ["temp_sea_weed"];
terreins = terreins.concat(terreins, terreins, terreins, terreins);
terreins = terreins.concat(terreins, terreins, terreins, terreins);
terreins.push("temp_sea_weed|gaia/fauna_fish");
textueByHeight.push({"upperHeightLimit": heightRange.min + 2/3 * (waterHeightAdjusted - heightRange.min), "terrain": terreins});
// Flat Water
textueByHeight.push({"upperHeightLimit": heightRange.min + 3/3 * (waterHeightAdjusted - heightRange.min), "terrain": "temp_mud_a"});
// Water surroundings/bog (with stone/metal some rabits and bushes)
var terreins = ["temp_plants_bog", "temp_plants_bog_aut", "temp_dirt_gravel_plants", "temp_grass_d"];
terreins = terreins.concat(terreins, terreins, terreins, terreins, terreins);
terreins = ["temp_plants_bog|gaia/flora_bush_temperate"].concat(terreins, terreins);
terreins = ["temp_dirt_gravel_plants|gaia/geology_metal_temperate", "temp_dirt_gravel_plants|gaia/geology_stone_temperate", "temp_plants_bog|gaia/fauna_rabbit"].concat(terreins, terreins);
terreins = ["temp_plants_bog_aut|gaia/flora_tree_dead"].concat(terreins, terreins);
textueByHeight.push({"upperHeightLimit": waterHeightAdjusted + 1/6 * (heightRange.max - waterHeightAdjusted), "terrain": terreins});
// Juicy grass near bog
textueByHeight.push({"upperHeightLimit": waterHeightAdjusted + 2/6 * (heightRange.max - waterHeightAdjusted),
	"terrain": ["temp_grass", "temp_grass_d", "temp_grass_long_b", "temp_grass_plants"]});
// Medium level grass
// var testActor = "actor|geology/decal_stone_medit_a.xml";
textueByHeight.push({"upperHeightLimit": waterHeightAdjusted + 3/6 * (heightRange.max - waterHeightAdjusted),
	"terrain": ["temp_grass", "temp_grass_b", "temp_grass_c", "temp_grass_mossy"]});
// Long grass near forest border
textueByHeight.push({"upperHeightLimit": waterHeightAdjusted + 4/6 * (heightRange.max - waterHeightAdjusted),
	"terrain": ["temp_grass", "temp_grass_b", "temp_grass_c", "temp_grass_d", "temp_grass_long_b", "temp_grass_clovers_2", "temp_grass_mossy", "temp_grass_plants"]});
// Forest border (With wood/food plants/deer/rabits)
var terreins = ["temp_grass_plants|gaia/flora_tree_euro_beech", "temp_grass_mossy|gaia/flora_tree_poplar", "temp_grass_mossy|gaia/flora_tree_poplar_lombardy",
	"temp_grass_long|gaia/flora_bush_temperate", "temp_mud_plants|gaia/flora_bush_temperate", "temp_mud_plants|gaia/flora_bush_badlands",
	"temp_grass_long|gaia/flora_tree_apple", "temp_grass_clovers|gaia/flora_bush_berry", "temp_grass_clovers_2|gaia/flora_bush_grapes",
	"temp_grass_plants|gaia/fauna_deer", "temp_grass_long_b|gaia/fauna_rabbit"];
var numTerreins = terreins.length;
for (var i = 0; i < numTerreins; i++)
	terreins.push("temp_grass_plants");
textueByHeight.push({"upperHeightLimit": waterHeightAdjusted + 5/6 * (heightRange.max - waterHeightAdjusted), "terrain": terreins});
// Unpassable woods
textueByHeight.push({"upperHeightLimit": waterHeightAdjusted + 6/6 * (heightRange.max - waterHeightAdjusted),
	"terrain": ["temp_grass_mossy|gaia/flora_tree_oak", "temp_forestfloor_pine|gaia/flora_tree_pine",
	"temp_grass_mossy|gaia/flora_tree_oak", "temp_forestfloor_pine|gaia/flora_tree_pine",
	"temp_mud_plants|gaia/flora_tree_dead", "temp_plants_bog|gaia/flora_tree_oak_large",
	"temp_dirt_gravel_plants|gaia/flora_tree_aleppo_pine", "temp_forestfloor_autumn|gaia/flora_tree_carob"]});
var minTerrainDistToBorder = 3;

// Time check 1
timeArray.push(new Date().getTime());
RMS.SetProgress(5);


// START THE GIANT WHILE LOOP:
// - Generate Heightmap
// - Search valid start position tiles
// - Choose a good start position derivation (largest distance between closest players)
// - Restart the loop if start positions are invalid or two players are to close to each other
var goodStartPositionsFound = false;
var minDistBetweenPlayers = 16 + mapSize / 16; // Don't set this higher than 25 for tiny maps! It will take forever with 8 players!
var enoughTiles = false;
var tries = 0;
while (!goodStartPositionsFound)
{
	tries++;
	log("Starting giant while loop try " + tries);
	// Generate reliefmap
	var myReliefmap = getRandomReliefmap(heightRange.min, heightRange.max);
	for (var i = 0; i < 50 + mapSize/4; i++) // Cycles depend on mapsize (more cycles -> bigger structures)
		myReliefmap = getHeightErrosionedReliefmap(myReliefmap, 1);
	myReliefmap = getRescaledReliefmap(myReliefmap, heightRange.min, heightRange.max);
	setReliefmap(myReliefmap);
	
	// Find good start position tiles
	var startPositions = [];
	var possibleStartPositions = [];
	var neededDistance = 7;
	var distToBorder = 2 * neededDistance; // Has to be greater than neededDistance! Otherwise the check if low/high ground is near will fail...
	var lowerHeightLimit = textueByHeight[3].upperHeightLimit;
	var upperHeightLimit = textueByHeight[6].upperHeightLimit;
	// Check for valid points by height
	for (var x = distToBorder + minTerrainDistToBorder; x < mapSize - distToBorder - minTerrainDistToBorder; x++)
	{
		for (var y = distToBorder + minTerrainDistToBorder; y < mapSize - distToBorder - minTerrainDistToBorder; y++)
		{
			var actualHeight = getHeight(x, y);
			if (actualHeight > lowerHeightLimit && actualHeight < upperHeightLimit)
			{
				// Check for points within a valid area by height (rectangular since faster)
				var isPossible = true;
				for (var offX = - neededDistance; offX <= neededDistance; offX++)
				{
					for (var offY = - neededDistance; offY <= neededDistance; offY++)
					{
						var testHeight = getHeight(x + offX, y + offY);
						if (testHeight <= lowerHeightLimit || testHeight >= upperHeightLimit)
						{
							isPossible = false;
							break;
						}
					}
				}
				if (isPossible)
				{
					possibleStartPositions.push([x, y]);
					// placeTerrain(x, y, "blue"); // For debug reasons. Plz don't remove. // Only works properly for 1 loop
				}
			}
		}
	}
	
	// Trying to reduce the number of possible start locations...
	
	// Reduce to tiles in a circle of mapSize / 2 distance to the center (to avoid players placed in corners)
	var possibleStartPositionsTemp = [];
	var maxDistToCenter = mapSize / 2;
	for (var i = 0; i < possibleStartPositions.length; i++)
	{
		var deltaX = possibleStartPositions[i][0] - mapSize / 2;
		var deltaY = possibleStartPositions[i][1] - mapSize / 2;
		var distToCenter = Math.pow(Math.pow(deltaX, 2) + Math.pow(deltaY, 2), 1/2);
		if (distToCenter < maxDistToCenter)
		{
			possibleStartPositionsTemp.push(possibleStartPositions[i]);
			// placeTerrain(possibleStartPositions[i][0], possibleStartPositions[i][1], "purple"); // Only works properly for 1 loop
		}
	}
	possibleStartPositions = deepcopy(possibleStartPositionsTemp);
	
	// Reduce to tiles near low and high ground (Rectangular check since faster) to make sure each player has access to all resource types.
	var possibleStartPositionsTemp = [];
	var maxDistToResources = distToBorder; // Has to be <= distToBorder!
	var minNumLowTiles = 10;
	var minNumHighTiles = 10;
	for (var i = 0; i < possibleStartPositions.length; i++)
	{
		var numLowTiles = 0;
		var numHighTiles = 0;
		for (var dx = - maxDistToResources; dx < maxDistToResources; dx++)
		{
			for (var dy = - maxDistToResources; dy < maxDistToResources; dy++)
			{
				var testHeight = getHeight(possibleStartPositions[i][0] + dx, possibleStartPositions[i][1] + dy);
				if (testHeight < lowerHeightLimit)
					numLowTiles++;
				if (testHeight > upperHeightLimit)
					numHighTiles++;
				if (numLowTiles > minNumLowTiles && numHighTiles > minNumHighTiles)
					break;
			}
			if (numLowTiles > minNumLowTiles && numHighTiles > minNumHighTiles)
				break;
		}
		if (numLowTiles > minNumLowTiles && numHighTiles > minNumHighTiles)
		{
			possibleStartPositionsTemp.push(possibleStartPositions[i]);
			// placeTerrain(possibleStartPositions[i][0], possibleStartPositions[i][1], "red"); // Only works properly for 1 loop
		}
	}
	possibleStartPositions = deepcopy(possibleStartPositionsTemp);
	
	if(possibleStartPositions.length > numPlayers)
		enoughTiles = true;
	else
	{
		enoughTiles = false;
		log("possibleStartPositions.length < numPlayers, possibleStartPositions.length = " + possibleStartPositions.length + ", numPlayers = " + numPlayers);
	}
	
	// Find a good start position derivation
	if (enoughTiles)
	{
		// Get some random start location derivations. NOTE: Itterating over all possible derivations is just to much (valid points ** numPlayers)
		var maxTries = 100000; // floor(800000 / (Math.pow(numPlayers, 2) / 2));
		var possibleDerivations = [];
		for (var i = 0; i < maxTries; i++)
		{
			var vector = [];
			for (var p = 0; p < numPlayers; p++)
				vector.push(randInt(possibleStartPositions.length));
			possibleDerivations.push(vector);
		}
		
		// Choose the start location derivation with the greatest minimum distance between players
		var maxMinDist = 0;
		for (var d = 0; d < possibleDerivations.length; d++)
		{
			var minDist = 2 * mapSize;
			for (var p1 = 0; p1 < numPlayers - 1; p1++)
			{
				for (var p2 = p1 + 1; p2 < numPlayers; p2++)
				{
					if (p1 != p2)
					{
						var StartPositionP1 = possibleStartPositions[possibleDerivations[d][p1]];
						var StartPositionP2 = possibleStartPositions[possibleDerivations[d][p2]];
						var actualDist = Math.pow(Math.pow(StartPositionP1[0] - StartPositionP2[0], 2) + Math.pow(StartPositionP1[1] - StartPositionP2[1], 2), 1/2);
						if (actualDist < minDist)
							minDist = actualDist;
						if (minDist < maxMinDist)
							break;
					}
				}
				if (minDist < maxMinDist)
					break;
			}
			if (minDist > maxMinDist)
			{
				maxMinDist = minDist;
				var bestDerivation = possibleDerivations[d];
			}
		}
		if (maxMinDist > minDistBetweenPlayers)
		{
			goodStartPositionsFound = true;
			log("Exiting giant while loop after " +  tries + " tries with a minimum player distance of " + maxMinDist);
		}
		else
			log("maxMinDist <= " + minDistBetweenPlayers + ", maxMinDist = " + maxMinDist);
	} // End of derivation check
} // END THE GIANT WHILE LOOP

// Time check 2
timeArray.push(new Date().getTime());
RMS.SetProgress(60);


////////
// Paint terrain by height and add props
////////

var propDensity = 1; // 1 means as determined in the loop, less for large maps as set below
if (mapSize > 500)
	propDensity = 1/4;
else if (mapSize > 400)
	propDensity = 3/4;
for(var x = minTerrainDistToBorder; x < mapSize - minTerrainDistToBorder; x++)
{
	for (var y = minTerrainDistToBorder; y < mapSize - minTerrainDistToBorder; y++)
	{
		var textureMinHeight = heightRange.min;
		for (var i = 0; i < textueByHeight.length; i++)
		{
			if (getHeight(x, y) >= textureMinHeight && getHeight(x, y) <= textueByHeight[i].upperHeightLimit)
			{
				placeTerrain(x, y, textueByHeight[i].terrain);
				// Add some props at...
				if (i == 0) // ...deep water
				{
					if (randFloat() < 1/100 * propDensity)
						placeObject(x, y, "actor|props/flora/pond_lillies_large.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/40 * propDensity)
						placeObject(x, y, "actor|props/flora/water_lillies.xml", 0, randFloat(0, 2*PI));
				}
				if (i == 1) // ...medium water (with fish)
				{
					if (randFloat() < 1/200 * propDensity)
						placeObject(x, y, "actor|props/flora/pond_lillies_large.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/100 * propDensity)
						placeObject(x, y, "actor|props/flora/water_lillies.xml", 0, randFloat(0, 2*PI));
				}
				if (i == 2) // ...low water/mud
				{
					if (randFloat() < 1/200 * propDensity)
						placeObject(x, y, "actor|props/flora/water_log.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/100 * propDensity)
						placeObject(x, y, "actor|props/flora/water_lillies.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/40 * propDensity)
						placeObject(x, y, "actor|geology/highland_c.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/20 * propDensity)
						placeObject(x, y, "actor|props/flora/reeds_pond_lush_b.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/10 * propDensity)
						placeObject(x, y, "actor|props/flora/reeds_pond_lush_a.xml", 0, randFloat(0, 2*PI));
				}
				if (i == 3) // ...water suroundings/bog
				{
					if (randFloat() < 1/200 * propDensity)
						placeObject(x, y, "actor|props/flora/water_log.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/100 * propDensity)
						placeObject(x, y, "actor|geology/highland_c.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/40 * propDensity)
						placeObject(x, y, "actor|props/flora/reeds_pond_lush_a.xml", 0, randFloat(0, 2*PI));
				}
				if (i == 4) // ...low height grass
				{
					if (randFloat() < 1/800 * propDensity)
						placeObject(x, y, "actor|props/flora/grass_field_flowering_tall.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/400 * propDensity)
						placeObject(x, y, "actor|geology/gray_rock1.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/200 * propDensity)
						placeObject(x, y, "actor|props/flora/bush_tempe_sm_lush.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/100 * propDensity)
						placeObject(x, y, "actor|props/flora/bush_tempe_b.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/40 * propDensity)
						placeObject(x, y, "actor|props/flora/grass_soft_small_tall.xml", 0, randFloat(0, 2*PI));
				}
				if (i == 5) // ...medium height grass
				{
					if (randFloat() < 1/800 * propDensity)
						placeObject(x, y, "actor|geology/decal_stone_medit_a.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/400 * propDensity)
						placeObject(x, y, "actor|props/flora/decals_flowers_daisies.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/200 * propDensity)
						placeObject(x, y, "actor|props/flora/bush_tempe_underbrush.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/100 * propDensity)
						placeObject(x, y, "actor|props/flora/grass_soft_small_tall.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/40 * propDensity)
						placeObject(x, y, "actor|props/flora/grass_temp_field.xml", 0, randFloat(0, 2*PI));
				}
				if (i == 6) // ...high height grass
				{
					if (randFloat() < 1/400 * propDensity)
						placeObject(x, y, "actor|geology/stone_granite_boulder.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/200 * propDensity)
						placeObject(x, y, "actor|props/flora/foliagebush.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/100 * propDensity)
						placeObject(x, y, "actor|props/flora/bush_tempe_underbrush.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/40 * propDensity)
						placeObject(x, y, "actor|props/flora/grass_soft_small_tall.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/20 * propDensity)
						placeObject(x, y, "actor|props/flora/ferns.xml", 0, randFloat(0, 2*PI));
				}
				if (i == 7) // ...forest border (with wood/food plants/deer/rabits)
				{
					if (randFloat() < 1/400 * propDensity)
						placeObject(x, y, "actor|geology/highland_c.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/200 * propDensity)
						placeObject(x, y, "actor|props/flora/bush_tempe_a.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/100 * propDensity)
						placeObject(x, y, "actor|props/flora/ferns.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/40 * propDensity)
						placeObject(x, y, "actor|props/flora/grass_soft_tuft_a.xml", 0, randFloat(0, 2*PI));
				}
				if (i == 8) // ...woods
				{
					if (randFloat() < 1/200 * propDensity)
						placeObject(x, y, "actor|geology/highland2_moss.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/100 * propDensity)
						placeObject(x, y, "actor|props/flora/grass_soft_tuft_a.xml", 0, randFloat(0, 2*PI));
					else if (randFloat() < 1/40 * propDensity)
						placeObject(x, y, "actor|props/flora/ferns.xml", 0, randFloat(0, 2*PI));
				}
				break;
			}
			else
			{
				textureMinHeight = textueByHeight[i].upperHeightLimit;
			}
		}
	}
}

// Time check 3
timeArray.push(new Date().getTime());
RMS.SetProgress(90);


////////
// Place players and start resources
////////

for (var p = 0; p < numPlayers; p++)
{
	var actualX = possibleStartPositions[bestDerivation[p]][0];
	var actualY = possibleStartPositions[bestDerivation[p]][1];
	placeCivDefaultEntities(actualX, actualY, p + 1, BUILDING_ANGlE, {"iberWall" : false});
	// Place some start resources
	var uDist = 8;
	var uSpace = 1;
	for (var j = 1; j <= 4; ++j)
	{
		var uAngle = BUILDING_ANGlE - PI * (2-j) / 2;
		var count = 4;
		for (var numberofentities = 0; numberofentities < count; numberofentities++)
		{
			var ux = actualX + uDist * cos(uAngle) + numberofentities * uSpace * cos(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * cos(uAngle + PI/2));
			var uz = actualY + uDist * sin(uAngle) + numberofentities * uSpace * sin(uAngle + PI/2) - (0.75 * uSpace * floor(count / 2) * sin(uAngle + PI/2));
			if (j % 2 == 0)
				placeObject(ux, uz, "gaia/flora_bush_berry", 0, randFloat(0, 2*PI)); 
			else
				placeObject(ux, uz, "gaia/flora_tree_cypress", 0, randFloat(0, 2*PI)); 
		}
	}
}


// Export map data
ExportMap();

// Time check 7
timeArray.push(new Date().getTime());

// Calculate progress percentage with the time checks
var generationTime = timeArray[timeArray.length - 1] - timeArray[0];
log("Total generation time (ms): " + generationTime);
for (var i = 0; i < timeArray.length; i++)
{
	var timeSinceStart = timeArray[i] - timeArray[0];
	var progressPercentage = 100 * timeSinceStart / generationTime;
	log("Time check " + i + ": Progress (%): " + progressPercentage);
}
