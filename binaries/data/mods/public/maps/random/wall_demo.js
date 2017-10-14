RMS.LoadLibrary("rmgen");

InitMap();

var mapSize = getMapSize();

////////////////////////////////////////
// Demonstration code for wall placement
////////////////////////////////////////

// Some general notes to the arguments:

// First all the place functions take the coordinates needed to place the wall
// X and Y coordinate are taken in seperate arguments like in placeObject
// Their meaning differs for different placement methods but are mainly self explanatory
// placeLinearWall takes 4 arguments here (2 coordinates) for startX, startY, targetX and targetY

// The next argument is always the 'wall' definition, an array of wall element type strings in most cases
// That looks like ['endLeft', 'wall', 'tower', 'wall', 'endRight', 'entry', 'endLeft', 'wall', 'tower', 'wall', 'endRight']
// For placeCircularWall and placeLinearWall only wall parts are needed like: ['tower', 'wall']
// They will automatically end with the first wall element if that makes sense (e.g. the wall is not closed)
// NOTE: They take further optional arguments to adjust this behaviour (See the wall_builder.js for that)
// placeFortress just takes a fortress type string that includes the wall definition
// The default fortress type strings are made for easy placement of predefined fortresses
// They are chosen like map sizes: 'tiny', 'small', 'medium', 'normal', 'large', 'veryLarge' and 'giant'
// NOTE: To place a custom fortress use placeCustomFortress instead
// It takes an instance of the Fortress class instead of the default fortress type strings

// The next argument is always the wall style string
// Wall styles are chosen by strings so the civ strings got by getCivCode() can be used
// Other styles may be present as well but besides the civ styles only 'palisades' includes all wall element types (yet)

// The next argument is always the index of the player that owns the wall.
// 0 is Gaia, 1 is Player 1 (default color blue), 2 is Player 2 (default color red), ...

// The next argument is an angle defining the orientation of the wall
// placeLinearWall does not need an angle since it's defined by startX/Y and targetX/Y
// Orientation works like the angle argument in placeObject
// 0 is always right (towards positive X)
// Raising the angle will rotate the wall counter-clockwise (mathmatical positive in default 2D)
// PI/2 faces top (positive Y)
// Orientation might be a little confusing for placeWall since it defines where the wall has its 'front' or 'outside' not the direction it will be build to.
// It's because all other methods work like that and it's intuitive there
// That means the walls outside by default (orientation = 0) faces positive X and (without bending wall elements) will be build towards positive Y

// Some other arguments are taken but all of them are optional and in most cases not needed
// One example is maxAngle for placeCircularWall that defines how far the wall will circumvent the center. Default is 2*PI which makes a full circle

// General wall placement setup
const distToMapBorder = 5;
const distToOtherWalls = 10;
var buildableMapSize = mapSize - 2 * distToMapBorder;
var actualX = distToMapBorder;
var actualY = distToMapBorder;
// Wall styles are chosen by strings so the civ strings got by getCivCode() can be used
// Other styles may be present as well but besides the civ styles only 'palisades' includes all wall element types (yet)
const wallStyleList = ["athen", "brit", "cart", "gaul", "iber", "mace", "maur", "pers", "ptol", "rome", "sele", "spart", "rome_siege", "palisades"];

////////////////////////////////////////
// Custom wall placement (element based)
////////////////////////////////////////
var wall = ['endLeft', 'wallLong', 'tower', 'wall', 'outpost', 'wall', 'cornerOut', 'wall', 'cornerIn', 'wall', 'house', 'endRight', 'entryTower', 'endLeft', 'wallShort', 'barracks', 'gate', 'tower', 'wall', 'wallFort', 'wall', 'endRight'];
for (var styleIndex = 0; styleIndex < wallStyleList.length; styleIndex++)
{
	var startX = actualX + styleIndex * buildableMapSize/wallStyleList.length; // X coordinate of the first wall element
	var startY = actualY; // Y coordinate of the first wall element
	var style = wallStyleList[styleIndex]; // // The wall's style like 'cart', 'iber', 'pers', 'rome', 'romeSiege' or 'palisades'
	var orientation = styleIndex * PI/64; // Orientation of the first wall element. 0 means 'outside' or 'front' is right (positive X, like object placement)
	// That means the wall will be build towards top (positive Y) if no corners are used
	var playerId = 0; // Owner of the wall (like in placeObject). 0 is Gaia, 1 is Player 1 (default color blue), ...
	placeWall(startX, startY, wall, style, playerId, orientation); // Actually placing the wall
}
actualX = distToMapBorder; // Reset actualX
actualY += 80 + distToOtherWalls; // Increase actualY for next wall placement method

//////////////////////////////////////////////////////////////
// Default fortress placement (chosen by fortress type string)
//////////////////////////////////////////////////////////////
var fortressRadius = 15; // The space the fortresses take in average. Just for design of this map
for (var styleIndex = 0; styleIndex < wallStyleList.length; styleIndex++)
{
	var centerX = actualX + fortressRadius + styleIndex * buildableMapSize/wallStyleList.length; // X coordinate of the center of the fortress
	var centerY = actualY + fortressRadius; // Y coordinate of the center of the fortress
	var type = 'tiny'; // Default fortress types are like map sizes: 'tiny', 'small', 'medium', 'large', 'veryLarge', 'giant'
	var style = wallStyleList[styleIndex]; // The wall's style like 'cart', 'iber', 'pers', 'rome', 'romeSiege' or 'palisades'
	var playerId = 0; // Owner of the wall. 0 is Gaia, 1 is Player 1 (default color blue), ...
	var orientation = styleIndex * PI/32; // Where the 'main entrance' of the fortress should face (like in placeObject). All fortresses walls should start with an entrance
	placeFortress(centerX, centerY, type, style, playerId, orientation); // Actually placing the fortress
	placeObject(centerX, centerY, 'other/obelisk', 0, 0*PI); // Place visual marker to see the center of the fortress
}
actualX = distToMapBorder; // Reset actualX
actualY += 2 * fortressRadius + 2 * distToOtherWalls; // Increase actualY for next wall placement method

//////////////////////////
// Circular wall placement
//////////////////////////
// NOTE: Don't use bending wall elements like corners here!
var radius = min((mapSize - actualY - distToOtherWalls) / 3, (buildableMapSize / wallStyleList.length - distToOtherWalls) / 2); // The radius of wall circle
var centerY = actualY + radius; // Y coordinate of the center of the wall circle
var orientation = 0; // Where the wall circle will be open if maxAngle < 2*PI, see below. Otherwise where the first wall element will be placed
for (var styleIndex = 0; styleIndex < wallStyleList.length; styleIndex++)
{
	var centerX = actualX + radius + styleIndex * buildableMapSize/wallStyleList.length; // X coordinate of the center of the wall circle
	var playerId = 0; // Player ID of the player owning the wall, 0 is Gaia, 1 is the first player (default blue), ...
	var wallPart = ['tower', 'wall', 'house']; // List of wall elements the wall will be build of. Optional, default id ['wall']
	var style = wallStyleList[styleIndex]; // The wall's style like 'cart', 'iber', 'pers', 'rome', 'romeSiege' or 'palisades'
	var maxAngle = PI/2 * (styleIndex%3 + 2); // How far the wall should circumvent the center
	placeCircularWall(centerX, centerY, radius, wallPart, style, playerId, orientation, maxAngle); // Actually placing the wall
	placeObject(centerX, centerY, 'other/obelisk', 0, 0*PI); // Place visual marker to see the center of the wall circle
	orientation += PI/16; // Increasing orientation to see how rotation works (like for object placement)
}
actualX = distToMapBorder; // Reset actualX
actualY += 2 * radius + distToOtherWalls; // Increase actualY for next wall placement method

///////////////////////////
// Polygonal wall placement
///////////////////////////
// NOTE: Don't use bending wall elements like corners here!
var radius = min((mapSize - actualY - distToOtherWalls) / 2, (buildableMapSize / wallStyleList.length - distToOtherWalls) / 2); // The radius of wall polygons
var centerY = actualY + radius; // Y coordinate of the center of the wall polygon
var orientation = 0; // Where the wall circle will be open if ???, see below. Otherwise where the first wall will be placed
for (var styleIndex = 0; styleIndex < wallStyleList.length; styleIndex++)
{
	var centerX = actualX + radius + styleIndex * buildableMapSize/wallStyleList.length; // X coordinate of the center of the wall circle
	var playerId = 0; // Player ID of the player owning the wall, 0 is Gaia, 1 is the first player (default blue), ...
	var cornerWallElement = 'tower'; // With wall element type will be uset for the corners of the polygon
	var wallPart = ['wall', 'tower']; // List of wall elements the wall will be build of. Optional, default id ['wall']
	var style = wallStyleList[styleIndex]; // The wall's style like 'cart', 'iber', 'pers', 'rome', 'romeSiege' or 'palisades'
	var numCorners = (styleIndex)%6 + 3; // How many corners the plogon will have
	var skipFirstWall = true; // If the wall should be open towards orientation
	placePolygonalWall(centerX, centerY, radius, wallPart, cornerWallElement, style, playerId, orientation, numCorners, skipFirstWall);
	placeObject(centerX, centerY, 'other/obelisk', 0, 0*PI); // Place visual marker to see the center of the wall circle
	orientation += PI/16; // Increasing orientation to see how rotation works (like for object placement)
}
actualX = distToMapBorder; // Reset actualX
actualY += 2 * radius + distToOtherWalls; // Increase actualY for next wall placement method

////////////////////////
// Linear wall placement
////////////////////////
// NOTE: Don't use bending wall elements like corners here!
var maxWallLength = (mapSize - actualY - distToMapBorder - distToOtherWalls); // Just for this maps design. How long the longest wall will be
var numWallsPerStyle = floor(buildableMapSize / distToOtherWalls / wallStyleList.length); // Just for this maps design. How many walls of the same style will be placed
for (var styleIndex = 0; styleIndex < wallStyleList.length; styleIndex++)
	for (var wallIndex = 0; wallIndex < numWallsPerStyle; wallIndex++)
	{
		var startX = actualX + (styleIndex * numWallsPerStyle + wallIndex) * distToOtherWalls; // X coordinate the wall will start from
		var startY = actualY; // Y coordinate the wall will start from
		var endX = startX; // X coordinate the wall will end
		var endY = actualY + (wallIndex + 1) * maxWallLength/numWallsPerStyle; // Y coordinate the wall will end
		var playerId = 0; // Player ID of the player owning the wall, 0 is Gaia, 1 is the first player (default blue), ...
		var wallPart = ['tower', 'wall']; // List of wall elements the wall will be build of
		var style = wallStyleList[styleIndex]; // The wall's style like 'cart', 'iber', 'pers', 'rome', 'romeSiege' or 'palisades'
		placeLinearWall(startX, startY, endX, endY, wallPart, style, playerId); // Actually placing the wall
		// placeObject(startX, startY, 'other/obelisk', 0, 0*PI); // Place visual marker to see where exsactly the wall begins
		// placeObject(endX, endY, 'other/obelisk', 0, 0*PI); // Place visual marker to see where exsactly the wall ends
	}

actualX = distToMapBorder; // Reset actualX
actualY += maxWallLength + distToOtherWalls; // Increase actualY for next wall placement method

ExportMap();
