////////////////////////////////////////////////////////////////////
// This file contains functionality to place walls on random maps //
////////////////////////////////////////////////////////////////////

// To do:
// Rename wall elements to fit he entity names so that entity = 'structures/' + 'civ + '_' + wallElement.type in the common case
// Add roman army camp to style palisades and add upgraded default palisade fortress types matching civ default fortresses
// Add further wall elements cornerHalfIn, cornerHalfOut and adjust default fortress types to better fit in the octagonal territory of a civil center
// Add civil center, corral, farmstead, field, market, mill, temple
// Adjust default fortress types
// Add wall style 'roads'
// Add trsures to 'others'
// Adjust documentation
// ?Use available civ-type wall elements rather than palisades: Remove 'endLeft' and 'endRight' as default wall elements and adjust default palisade fortress types?
// ?Remove endRight, endLeft and adjust generic fortress types palisades?
// ?Think of something to enable splitting walls into two walls so more complex walls can be build and roads can have branches/crossroads?
// ?Readjust placement angle for wall elements with bending when used in linear/circular walls by their bending?


///////////////////////////////
// WallElement class definition
///////////////////////////////

// argument type: Descriptive string, example: 'wall'. NOTE: Not really needed. Mainly for custom wall elements and to get the wall element type in code.
// argument entity: Optional. Template string to be placed, example: 'structures/cart_wall'. Default is undefined (No entity placed)
// argument angle: Optional. Placement angle so that 'outside' is 'right' (towards positive X like a unit placed with angle 0). Default is 0 (0*PI)
// argument width: Optional. The width it lengthens the wall, width because it's the needed space in a right angle to 'outside'. Default is 0
// argument indent: Optional. The indentation means its drawn inside (positive values) or pushed outwards (negative values). Default is 0
// NOTE: Bending is only used for fortresses and custom walls. Linear/circular walls walls use no/generic bending
// argument bending: Optional. How the direction of the wall is changed after this element, positive is bending 'in' (counter clockwise like entity placement)
function WallElement(type, entity, angle, width, indent, bending)
{
	// NOTE: Not all wall elements have a symetry. So there's an direction 'outside' (towards right/positive X by default)
	// In this sense 'left'/'right' means left/right of you when standing upon the wall and look 'outside'
	// The wall is build towards 'left' so the next wall element will be placed left of the previous one (towards positive Y by default)
	// In this sense the wall's direction is left meaning the 'bending' of a corner is used for the element following the corner
	// With 'inside' and 'outside' defined as above, corners bend 'in'/'out' meaning placemant angle is increased/decreased (counter clockwise like object placement)
	this.type = type;
	// Wall element type documentation:
	// Enlengthening straight blocking (mainly left/right symetric) wall elements (Walls and wall fortifications)
		// 'wall': A blocking straight wall element that mainly lengthens the wall, self-explanatory
		// 'wallShort': self-explanatory
		// 'wallLong': self-explanatory
		// 'tower': A blocking straight wall element with damage potential (but for palisades) that slightly lengthens the wall, exsample: wall tower, palisade tower(No attack)
		// 'wallFort': A blocking straight wall element with massive damage potential that lengthens the wall, exsample: fortress, palisade fort
	// Enlengthening straight non/custom blocking (mainly left/right symetric) wall elements (Gates and entrys)
		// 'gate': A blocking straight wall element with passability determined by owner, example: gate (Functionality not yet implemented)
		// 'entry': A non-blocking straight wall element (same width as gate) but without an actual template or just a flag/column/obelisk
		// 'entryTower': A non-blocking straight wall element (same width as gate) represented by a single (maybe indented) template, example: defense tower, wall tower, outpost, watchtower
		// 'entryFort': A non-blocking straight wall element represented by a single (maybe indented) template, example: fortress, palisade fort
	// Bending wall elements (Wall corners)
		// 'cornerIn': A wall element bending the wall by PI/2 'inside' (left, +, see above), example: wall tower, palisade curve
		// 'cornerOut': A wall element bending the wall by PI/2 'outside' (right, -, see above), example: wall tower, palisade curve
		// 'cornerHalfIn': A wall element bending the wall by PI/4 'inside' (left, +, see above), example: wall tower, palisade curve. NOTE: Not yet implemented
		// 'cornerHalfOut': A wall element bending the wall by PI/4 'outside' (right, -, see above), example: wall tower, palisade curve. NOTE: Not yet implemented
	// Zero length straight indented (mainly left/right symetric) wall elements (Outposts/watchtowers and non-defensive base structures)
		// 'outpost': A zero-length wall element without bending far indented so it stands outside the wall, exsample: outpost, defense tower, watchtower
		// 'house': A zero-length wall element without bending far indented so it stands inside the wall that grants population bonus, exsample: house, hut, longhouse
		// 'barracks': A zero-length wall element without bending far indented so it stands inside the wall that grants unit production, exsample: barracks, tavern, ...
	this.entity = entity;
	this.angle = (angle !== undefined) ? angle : 0*PI;
	this.width = (width !== undefined) ? width : 0;
	this.indent = (indent !== undefined) ? indent : 0;
	this.bending = (bending !== undefined) ? bending : 0*PI;
}


////////////////////////////
// Fortress class definition
////////////////////////////

// A list would do for symetric fortresses but if 'getCenter' don't do sufficient the center can be set manually
// argument type: Descriptive string, example: 'tiny'. Not really needed (WallTool.wallTypes['type string'] is used). Mainly for custom wall elements.
// argument wall: Optional. Array of wall element strings. Can be set afterwards. Default is an epty array.
	// Example: ['entrance', 'wall', 'cornerIn', 'wall', 'gate', 'wall', 'entrance', 'wall', 'cornerIn', 'wall', 'gate', 'wall', 'cornerIn', 'wall']
// argument center: Optional. Array of 2 floats determinig the vector from the center to the 1st wall element. Can be set afterwards. Default is [0, 0]. (REALLY???)
	// NOTE: The center will be recalculated when WallTool.setFortress is called. To avoid this set WallTool.calculateCenter to false.
function Fortress(type, wall, center)
{
	this.type = type; // Only usefull to get the type of the actual fortress (by 'WallTool.fortress.type')
	this.wall = (wall !== undefined) ? wall : [];
	this.center = [0, 0]; // X/Z offset (in default orientation) from first wall element to center, perhaps should be the other way around...
}


///////////////////////////////////////////////
// Setup data structure for default wall styles
///////////////////////////////////////////////

// A wall style is an associative array with all wall elements of that style in it associated with the wall element type string.
// wallStyles holds all the wall styles within an associative array while a wall style is associated with the civ string or another descriptive strings like 'palisades', 'fence', 'cart', 'celt'...
var wallStyles = {};

// Generic civ dependent wall style definition. 'rome_siege' needs some tweek...
var scaleByCiv = {'athen' : 1.5, 'cart' : 1.8, 'celt' : 1.5, 'hele' : 1.5, 'iber' : 1.5, 'mace' : 1.5, 'pers' : 1.5, 'rome' : 1.5, 'spart' : 1.5, 'rome_siege' : 1.5};
for (var style in scaleByCiv)
{
	var civ = style;
	if (style == 'rome_siege')
		civ = 'rome';
	wallStyles[style] = {};
	// Default wall elements
	wallStyles[style]['tower'] = new WallElement('tower', 'structures/' + style + '_wall_tower', PI, scaleByCiv[style]);
	wallStyles[style]['endLeft'] = new WallElement('endLeft', 'structures/' + style + '_wall_tower', PI, scaleByCiv[style]); // Same as tower. To be compatible with palisades...
	wallStyles[style]['endRight'] = new WallElement('endRight', 'structures/' + style + '_wall_tower', PI, scaleByCiv[style]); // Same as tower. To be compatible with palisades...
	wallStyles[style]['cornerIn'] = new WallElement('cornerIn', 'structures/' + style + '_wall_tower', 5*PI/4, 0, 0.35*scaleByCiv[style], PI/2); // 2^0.5 / 4 ~= 0.35 ~= 1/3
	wallStyles[style]['cornerOut'] = new WallElement('cornerOut', 'structures/' + style + '_wall_tower', 3*PI/4, 0.71*scaleByCiv[style], 0, -PI/2); // // 2^0.5 / 2 ~= 0.71 ~= 2/3
	wallStyles[style]['wallShort'] = new WallElement('wallShort', 'structures/' + style + '_wall_short', 0*PI, 2*scaleByCiv[style]);
	wallStyles[style]['wall'] = new WallElement('wall', 'structures/' + style + '_wall_medium', 0*PI, 4*scaleByCiv[style]);
	wallStyles[style]['wallLong'] = new WallElement('wallLong', 'structures/' + style + '_wall_long', 0*PI, 6*scaleByCiv[style]);
	// Gate and entrance wall elements
	if (style == 'cart')
		var gateWidth = 3.5*scaleByCiv[style];
	else if (style == 'celt')
		var gateWidth = 4*scaleByCiv[style];
	// else if (style == 'iber')
		// var gateWidth = 5.5*scaleByCiv[style];
	else
		var gateWidth = 6*scaleByCiv[style];
	wallStyles[style]['gate'] = new WallElement('gate', 'structures/' + style + '_wall_gate', 0*PI, gateWidth);
	wallStyles[style]['entry'] = new WallElement('entry', undefined, 0*PI, gateWidth);
	if (civ == 'iber') // Adjust iberians to have no upkeep at entries with a tower for convinience ATM, may be changed
		wallStyles[style]['entryTower'] = new WallElement('entryTower', 'structures/' + civ + '_wall_tower', PI, gateWidth, -4*scaleByCiv[style]);
	else
		wallStyles[style]['entryTower'] = new WallElement('entryTower', 'structures/' + civ + '_defense_tower', PI, gateWidth, -4*scaleByCiv[style]);
	wallStyles[style]['entryFort'] = new WallElement('entryFort', 'structures/' + civ + '_fortress', 0*PI, 8*scaleByCiv[style], 6*scaleByCiv[style]);
	// Defensive wall elements with 0 width outside the wall
	wallStyles[style]['outpost'] = new WallElement('outpost', 'structures/' + civ + '_outpost', PI, 0, -4*scaleByCiv[style]);
	wallStyles[style]['defenseTower'] = new WallElement('defenseTower', 'structures/' + civ + '_defenseTower', PI, 0, -4*scaleByCiv[style]);
	// Base buildings wall elements with 0 width inside the wall
	wallStyles[style]['barracks'] = new WallElement('barracks', 'structures/' + civ + '_barracks', PI, 0, 4.5*scaleByCiv[style]);
	wallStyles[style]['civilCentre'] = new WallElement('civilCentre', 'structures/' + civ + '_civil_centre', PI, 0, 4.5*scaleByCiv[style]);
	wallStyles[style]['farmstead'] = new WallElement('farmstead', 'structures/' + civ + '_farmstead', PI, 0, 4.5*scaleByCiv[style]);
	wallStyles[style]['field'] = new WallElement('field', 'structures/' + civ + '_field', PI, 0, 4.5*scaleByCiv[style]);
	wallStyles[style]['fortress'] = new WallElement('fortress', 'structures/' + civ + '_fortress', PI, 0, 4.5*scaleByCiv[style]);
	wallStyles[style]['house'] = new WallElement('house', 'structures/' + civ + '_house', PI, 0, 4.5*scaleByCiv[style]);
	wallStyles[style]['market'] = new WallElement('market', 'structures/' + civ + '_market', PI, 0, 4.5*scaleByCiv[style]);
	wallStyles[style]['mill'] = new WallElement('mill', 'structures/' + civ + '_mill', PI, 0, 4.5*scaleByCiv[style]);
	wallStyles[style]['temple'] = new WallElement('temple', 'structures/' + civ + '_temple', PI, 0, 4.5*scaleByCiv[style]);
	// Generic space/gap wall elements
	wallStyles[style]['space1'] = new WallElement('space1', undefined, 0*PI, scaleByCiv[style]);
	wallStyles[style]['space2'] = new WallElement('space2', undefined, 0*PI, 2*scaleByCiv[style]);
	wallStyles[style]['space3'] = new WallElement('space3', undefined, 0*PI, 3*scaleByCiv[style]);
	wallStyles[style]['space4'] = new WallElement('space4', undefined, 0*PI, 4*scaleByCiv[style]);
}

// Add wall fortresses for all generic styles
wallStyles['athen']['wallFort'] = new WallElement('wallFort', 'structures/athen_fortress', 2*PI/2 /* PI/2 */, 5.1 /* 5.6 */, 1.9 /* 1.9 */);
wallStyles['cart']['wallFort'] = new WallElement('wallFort', 'structures/cart_fortress', PI, 5.1, 1.6);
wallStyles['celt']['wallFort'] = new WallElement('wallFort', 'structures/celt_fortress_g', PI, 4.2, 1.5);
wallStyles['hele']['wallFort'] = new WallElement('wallFort', 'structures/hele_fortress', 2*PI/2 /* PI/2 */, 5.1 /* 5.6 */, 1.9 /* 1.9 */);
wallStyles['iber']['wallFort'] = new WallElement('wallFort', 'structures/iber_fortress', PI, 5, 0.2);
wallStyles['mace']['wallFort'] = new WallElement('wallFort', 'structures/mace_fortress', 2*PI/2 /* PI/2 */, 5.1 /* 5.6 */, 1.9 /* 1.9 */);
wallStyles['pers']['wallFort'] = new WallElement('wallFort', 'structures/pers_fortress', PI, 5.6/*5.5*/, 1.9/*1.7*/);
wallStyles['rome']['wallFort'] = new WallElement('wallFort', 'structures/rome_fortress', PI, 6.3, 2.1);
wallStyles['spart']['wallFort'] = new WallElement('wallFort', 'structures/spart_fortress', 2*PI/2 /* PI/2 */, 5.1 /* 5.6 */, 1.9 /* 1.9 */);
// Adjust 'rome_siege' style
wallStyles['rome_siege']['wallFort'] = new WallElement('wallFort', 'structures/rome_army_camp', PI, 7.2, 2);
wallStyles['rome_siege']['entryFort'] = new WallElement('entryFort', 'structures/rome_army_camp', PI, 12, 7);
wallStyles['rome_siege']['house'] = new WallElement('house', 'structures/rome_tent', PI, 0, 4);

// Add special wall styles not well to implement generic (and to show how custom styles can be added)

// Add special wall style 'palisades'
wallStyles['palisades'] = {};
wallStyles['palisades']['wall'] = new WallElement('wall', 'other/palisades_rocks_medium', 0*PI, 2.3);
wallStyles['palisades']['wallLong'] = new WallElement('wall', 'other/palisades_rocks_long', 0*PI, 3.5);
wallStyles['palisades']['wallShort'] = new WallElement('wall', 'other/palisades_rocks_short', 0*PI, 1.2);
wallStyles['palisades']['tower'] = new WallElement('tower', 'other/palisades_rocks_tower', -PI/2, 0.7);
wallStyles['palisades']['wallFort'] = new WallElement('wallFort', 'other/palisades_rocks_fort', PI, 1.7);
wallStyles['palisades']['gate'] = new WallElement('gate', 'other/palisades_rocks_gate', 0*PI, 3.6);
wallStyles['palisades']['entry'] = new WallElement('entry', undefined, wallStyles['palisades']['gate'].angle, wallStyles['palisades']['gate'].width);
wallStyles['palisades']['entryTower'] = new WallElement('entryTower', 'other/palisades_rocks_watchtower', 0*PI, wallStyles['palisades']['gate'].width, -3);
wallStyles['palisades']['entryFort'] = new WallElement('entryFort', 'other/palisades_rocks_fort', PI, 6, 3);
wallStyles['palisades']['cornerIn'] = new WallElement('cornerIn', 'other/palisades_rocks_curve', 3*PI/4, 2.1, 0.7, PI/2);
wallStyles['palisades']['cornerOut'] = new WallElement('cornerOut', 'other/palisades_rocks_curve', 5*PI/4, 2.1, -0.7, -PI/2);
wallStyles['palisades']['outpost'] = new WallElement('outpost', 'other/palisades_rocks_outpost', PI, 0, -2);
wallStyles['palisades']['house'] = new WallElement('house', 'other/celt_hut', PI, 0, 5);
wallStyles['palisades']['barracks'] = new WallElement('barracks', 'other/celt_tavern', PI, 0, 5);
wallStyles['palisades']['endRight'] = new WallElement('endRight', 'other/palisades_rocks_end', -PI/2, 0.2);
wallStyles['palisades']['endLeft'] = new WallElement('endLeft', 'other/palisades_rocks_end', PI/2, 0.2);

// Add special wall style 'road'

// Add special wall element collection 'other'
// NOTE: This is not a wall style in the common sense. Use with care!
wallStyles['other'] = {};
wallStyles['other']['fence'] = new WallElement('fence', 'other/fence_long', -PI/2, 3.1);
wallStyles['other']['fence_short'] = new WallElement('fence_short', 'other/fence_short', -PI/2, 1.5);
wallStyles['other']['fence_stone'] = new WallElement('fence_stone', 'other/fence_stone', -PI/2, 2.5);
wallStyles['other']['palisade'] = new WallElement('palisade', 'other/palisades_rocks_short', 0, 1.2);
wallStyles['other']['column'] = new WallElement('column', 'other/column_doric', 0, 1);
wallStyles['other']['obelisk'] = new WallElement('obelisk', 'other/obelisk', 0, 2);
wallStyles['other']['spike'] = new WallElement('spike', 'other/palisades_angle_spike', -PI/2, 1);
wallStyles['other']['bench'] = new WallElement('bench', 'other/bench', PI/2, 1.5);
wallStyles['other']['benchForTable'] = new WallElement('benchForTable', 'other/bench', 0, 0.5);
wallStyles['other']['table'] = new WallElement('table', 'other/table_rectangle', 0, 1);
wallStyles['other']['table_square'] = new WallElement('table_square', 'other/table_square', PI/2, 1);
wallStyles['other']['flag'] = new WallElement('flag', 'special/rallypoint', PI, 1);
wallStyles['other']['standing_stone'] = new WallElement('standing_stone', 'gaia/special_ruins_standing_stone', PI, 1);
wallStyles['other']['settlement'] = new WallElement('settlement', 'gaia/special_settlement', PI, 6);
wallStyles['other']['gap'] = new WallElement('gap', undefined, 0, 2);
wallStyles['other']['gapSmall'] = new WallElement('gapSmall', undefined, 0, 1);
wallStyles['other']['gapLarge'] = new WallElement('gapLarge', undefined, 0, 4);
wallStyles['other']['cornerIn'] = new WallElement('cornerIn', undefined, 0, 0, 0, PI/2);
wallStyles['other']['cornerOut'] = new WallElement('cornerOut', undefined, 0, 0, 0, -PI/2);


///////////////////////////////////////////////////////
// Setup data structure for some default fortress types
///////////////////////////////////////////////////////

// A fortress type is just an instance of the fortress class with actually something in it.
// fortressTypes holds all the fortressess within an associative array while a fortress is associated with a descriptive string maching the map sizes, example: 'tiny', 'giant'
var fortressTypes = {};
// Setup some default fortress types
// Add fortress type 'tiny'
fortressTypes['tiny'] = new Fortress('tiny');
var wallPart = ['entry', 'wall', 'cornerIn', 'wall'];
fortressTypes['tiny'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'small'
fortressTypes['small'] = new Fortress('small');
var wallPart = ['entry', 'endLeft', 'wall', 'cornerIn', 'wall', 'endRight'];
fortressTypes['small'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'medium'
fortressTypes['medium'] = new Fortress('medium');
var wallPart = ['entry', 'endLeft', 'wall', 'outpost', 'wall',
	'cornerIn', 'wall', 'outpost', 'wall', 'endRight'];
fortressTypes['medium'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'normal'
fortressTypes['normal'] = new Fortress('normal');
var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall',
	'cornerIn', 'wall', 'tower', 'wall', 'endRight'];
fortressTypes['normal'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'large'
fortressTypes['large'] = new Fortress('large');
var wallPart = ['entry', 'endLeft', 'wall', 'outpost', 'wall', 'cornerIn', 'wall',
	'cornerOut', 'wall', 'cornerIn', 'wall', 'outpost', 'wall', 'endRight'];
fortressTypes['large'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'veryLarge'
fortressTypes['veryLarge'] = new Fortress('veryLarge');
var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall', 'cornerIn', 'wall',
	'cornerOut', 'wall', 'cornerIn', 'wall', 'tower', 'wall', 'endRight'];
fortressTypes['veryLarge'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'giant'
fortressTypes['giant'] = new Fortress('giant');
var wallPart = ['entry', 'endLeft', 'wall', 'outpost', 'wall', 'cornerIn', 'wall', 'outpost', 'wall',
	'cornerOut', 'wall', 'outpost', 'wall', 'cornerIn', 'wall', 'outpost', 'wall', 'endRight'];
fortressTypes['giant'].wall = wallPart.concat(wallPart, wallPart, wallPart);

// Add (some) iberian civ bonus fortress (The civ can still be set, just an uncommon default fortress)
var wall = ['gate', 'tower', 'wall', 'cornerIn', 'wallLong', 'tower',
	'gate', 'tower', 'wallLong', 'cornerIn', 'wall', 'tower',
	'gate', 'wall', 'cornerIn', 'wall', 'cornerOut', 'wall', 'cornerIn', 'wallLong', 'tower',
	'gate', 'tower', 'wallLong', 'cornerIn', 'wall', 'tower', 'wall', 'cornerIn', 'wall'];
fortressTypes['iberCivBonus'] = new Fortress('iberCivBonus', wall);
var wall = ['gate', 'tower', 'wall', 'cornerIn', 'wall', 'cornerOut', 'wall', 'cornerIn', 'wallLong', 'tower',
	'gate', 'tower', 'wallLong', 'cornerIn', 'wall', 'tower', 'wall', 'cornerIn', 'wall', 'cornerOut',
	'gate', 'tower', 'wall', 'cornerIn', 'wallLong', 'tower',
	'gate', 'tower', 'wallLong', 'cornerIn', 'wall', 'tower'];
fortressTypes['iberCivBonus2'] = new Fortress('iberCivBonus2', wall);
var wall = ['gate', 'tower', 'wall', 'cornerOut', 'wallShort', 'cornerIn', 'wall', 'cornerIn', 'wallLong', 'cornerIn', 'wallShort', 'cornerOut', 'wall', 'tower',
	'gate', 'tower', 'wallLong', 'cornerIn', 'wallLong', 'cornerIn', 'wallShort', 'cornerOut',
	'gate', 'tower', 'wall', 'cornerIn', 'wall', 'cornerOut', 'wallShort', 'cornerIn', 'wall', 'tower',
	'gate', 'tower', 'wallShort', 'cornerIn', 'wall', 'tower', 'wallShort', 'tower'];
fortressTypes['iberCivBonus3'] = new Fortress('iberCivBonus3', wall);


// Setup some semi default fortresses for 'palisades' style
var fortressTypeKeys = ['tiny', 'small', 'medium', 'normal', 'large', 'veryLarge', 'giant'];
for (var i = 0; i < fortressTypeKeys.length; i++)
{
	var newKey = fortressTypeKeys[i] + 'Palisades';
	var oldWall = fortressTypes[fortressTypeKeys[i]].wall;
	fortressTypes[newKey] = new Fortress(newKey, []); // [] just to make sure it's an array though it's an array by default
	var fillTowersBetween = ['wall', 'endLeft', 'endRight', 'cornerIn', 'cornerOut'];
	for (var j = 0; j < oldWall.length; j++)
	{
		fortressTypes[newKey].wall.push(oldWall[j]); // Only works if the first element is an entry or gate (not in fillTowersBetween)
		if (j+1 < oldWall.length)
			if (fillTowersBetween.indexOf(oldWall[j]) > -1 && fillTowersBetween.indexOf(oldWall[j+1]) > -1) // ... > -1 means 'exists' here
				fortressTypes[newKey].wall.push('tower');
	}
}


///////////////////////////////////////////////////////////////////////////////
// Define some helper functions: getWallAlignment, getWallCenter, getWallLength
///////////////////////////////////////////////////////////////////////////////

// Get alignment of a wall 
// Returns a list of lists of most arguments needed to place the different wall elements for a given wall
// Placing the first wall element at startX/startY placed with angle given by orientation
// An alignement can be used to get the center of a 'wall' (more likely used for closed walls like fortresses) with the getWallCenter function
function getWallAlignment(startX, startY, wall, style, orientation)
{
	orientation = (orientation !== undefined) ? orientation : 0*PI;
	var alignment = []
	var wallX = startX;
	var wallY = startY;
	for (var i = 0; i < wall.length; i++)
	{
		if (wallStyles[style][wall[i]] === undefined)
			warn('No valid wall element: ' + wall[i]);
		var element = wallStyles[style][wall[i]]
		// Indentation
		var placeX = wallX - element.indent * cos(orientation);
		var placeY = wallY - element.indent * sin(orientation);
		// Add element alignment
		alignment.push([placeX, placeY, element.entity, orientation + element.angle]);
		// Preset vars for the next wall element
		if (i+1 < wall.length)
		{
			orientation += element.bending;
			if (wallStyles[style][wall[i+1]] === undefined)
				warn('No valid wall element: ' + wall[i+1]);
			var nextElement = wallStyles[style][wall[i+1]];
			var distance = (element.width + nextElement.width)/2;
			// Corrections for elements with indent AND bending
			if (element.bending !== 0 && element.indent !== 0)
			{
				// Indent correction to adjust distance
				distance += element.indent*sin(element.bending);
				// Indent correction to normalize indentation
				wallX += element.indent * cos(orientation);
				wallY += element.indent * sin(orientation);
			}
			// Set the next coordinates of the next element in the wall (meaning without indentation adjustment)
			wallX -= distance * sin(orientation);
			wallY += distance * cos(orientation);
		}
	}
	return alignment;
}

// Get the center of a wall (mainly usefull for closed walls like fortresses)
// Center calculation works like getting the center of mass assuming all wall elements have the same 'waight'
// It returns the vector (array [x, y]) from the first wall element to the center
function getWallCenter(alignment)
{
	var x = 0;
	var y = 0;
	for (var i = 0; i < alignment.length; i++)
	{
		x += alignment[i][0]/alignment.length;
		y += alignment[i][1]/alignment.length;
	}
	var center = [x, y];
	return center;
}

// Get the length of a wall used by placeIrregularPolygonalWall
// NOTE: Does not support bending wall elements like corners!
function getWallLength(wall, style)
{
	var length = 0;
	for (var i = 0; i < wall.length; i++)
	{
		length += wallStyles[style][wall[i]].width;
	}
	return length;
}


/////////////////////////////////////////////
// Define the different wall placer functions
/////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////
// Place simple wall starting with the first wall element placed at startX/startY
/////////////////////////////////////////////////////////////////////////////////
// orientation: 0 means 'outside' or 'front' of the wall is right (positive X) like placeObject
// It will then be build towards top (positive Y) if no bending wall elements like corners are used
// Raising orientation means the wall is rotated counter-clockwise like placeObject
function placeWall(startX, startY, wall, style, playerId, orientation)
{
	var AM = getWallAlignment(startX, startY, wall, style, orientation);
	for (var iWall = 0; iWall < wall.length; iWall++)
	{
		if (AM[iWall][2] !== undefined)
			placeObject(AM[iWall][0], AM[iWall][1], AM[iWall][2], playerId, AM[iWall][3]);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Place a fortress (mainly a closed wall build like placeWall) with the center at centerX/centerY
//////////////////////////////////////////////////////////////////////////////////////////////////
// Should always start with the main entrance (like 'entry' or 'gate') to get the orientation right (like placeObject)
function placeCustomFortress(centerX, centerY, fortress, style, playerId, orientation, scipGetCenter)
{
	if (!scipGetCenter)
	{
		var alignment = getWallAlignment(0, 0, fortress.wall, style);
		var center = getWallCenter(alignment);
		var startX = centerX - center[0] * cos(orientation) + center[1] * sin(orientation);
		var startY = centerY - center[1] * cos(orientation) - center[0] * sin(orientation);
	}
	else
	{
		var startX = centerX + fortress.center[0];
		var startY = centerY + fortress.center[1];
	}
	placeWall(startX, startY, fortress.wall, style, playerId, orientation)
}

function placeFortress(centerX, centerY, type, style, playerId, orientation, scipGetCenter)
{
	placeCustomFortress(centerX, centerY, fortressTypes[type], style, playerId, orientation, scipGetCenter);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Place a linear wall of repeatant wall elements given in the argument wallPart from startX/startY to targetX/targetY
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// startX: x coordinate of the beginning of the wall
// startY: y coordinate of the beginning of the wall
// targetX: x coordinate of the ending of the wall
// targetY: y coordinate of the ending of the wall
// NOTE: Start and target coordinates are exact (besides the scale offset) meaning the wall will begin/end at the given coordinates (not the first/last entity is placed there)
// wallPart: Optional. An array of wall element type strings (see WallElement.type). Default is ['wall']
// NOTE: Don't use wall elements with bending like corners!
// style: Optional. An wall style string (like defined in the 'wallStyles' dictionary). Default is 'palisades'
// playerId: Optional. The walls owners player ID (like in the function 'placeObject' so 0 is gaia, 1 is the first player), Default is 0 (gaia)
// endWithFirst: Optional. A boolean value. If true the 1st wall element in the wallPart array will finalize the wall. Default is true
function placeLinearWall(startX, startY, targetX, targetY, wallPart, style, playerId, endWithFirst)
{
	// Setup optional arguments to the default
	wallPart = (wallPart || ['wall']);
	style = (style || 'palisades');
	playerId = (playerId || 0);
	// endWithFirst = (endWithFirst || true);
	endWithFirst = typeof endWithFirst == 'undefined' ? true : endWithFirst;
	log("endWithFirst = " + endWithFirst);
	// Check arguments
	for (var elementIndex = 0; elementIndex < wallPart.length; elementIndex++)
		if (wallStyles[style][wallPart[elementIndex]].bending != 0)
			warn("Bending is not supported by placeLinearWall but a bending wall element is used: " + wallPart[elementIndex] + " -> wallStyles[style][wallPart[elementIndex]].entity");
	// Setup number of wall parts
	var totalLength = getDistance(startX, startY, targetX, targetY);
	var wallPartLength = 0;
	for (var elementIndex = 0; elementIndex < wallPart.length; elementIndex++)
		wallPartLength += wallStyles[style][wallPart[elementIndex]].width;
	var numParts = 0;
	if (endWithFirst == true)
		numParts = ceil((totalLength - wallStyles[style][wallPart[0]].width) / wallPartLength)
	else
		numParts = ceil(totalLength / wallPartLength);
	// Setup scale factor
	var scaleFactor = 1;
	if (endWithFirst == true)
		scaleFactor = totalLength / (numParts * wallPartLength + wallStyles[style][wallPart[0]].width)
	else
		scaleFactor = totalLength / (numParts * wallPartLength);
	// Setup angle
	var wallAngle = getAngle(startX, startY, targetX, targetY); // NOTE: function 'getAngle()' is about to be changed...
	var placeAngle = wallAngle - PI/2;
	// Place wall entities
	var x = startX;
	var y = startY;
	for (var partIndex = 0; partIndex < numParts; partIndex++)
	{
		for (var elementIndex = 0; elementIndex < wallPart.length; elementIndex++)
		{
			var wallEle = wallStyles[style][wallPart[elementIndex]];
			// Width correction
			x += scaleFactor * wallEle.width/2 * cos(wallAngle);
			y += scaleFactor * wallEle.width/2 * sin(wallAngle);
			// Indent correction
			var placeX = x - wallEle.indent * sin(wallAngle);
			var placeY = y + wallEle.indent * cos(wallAngle);
			// Placement
			if (wallEle.entity !== undefined)
				placeObject(placeX, placeY, wallEle.entity, playerId, placeAngle + wallEle.angle);
			x += scaleFactor * wallEle.width/2 * cos(wallAngle);
			y += scaleFactor * wallEle.width/2 * sin(wallAngle);
		}
	}
	if (endWithFirst == true)
	{
		var wallEle = wallStyles[style][wallPart[0]];
		x += scaleFactor * wallEle.width/2 * cos(wallAngle);
		y += scaleFactor * wallEle.width/2 * sin(wallAngle);
		if (wallEle.entity !== undefined)
			placeObject(x, y, wallEle.entity, playerId, placeAngle + wallEle.angle);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Place a circular wall of repeatant wall elements given in the argument wallPart arround centerX/centerY with the given radius
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The wall is not necessarily closed depending on the optional argument maxAngle (better name?)
// NOTE: Don't use wall elements with bending like corners!
// Todo: add eccentricity
function placeCircularWall(centerX, centerY, radius, wallPart, style, playerId, orientation, maxAngle, endWithFirst, maxBendOff)
{
	// Setup optional arguments to the default
	wallPart = (wallPart || ['wall']);
	style = (style || 'palisades');
	playerId = (playerId || 0);
	orientation = (orientation || 0);
	maxAngle = (maxAngle || 2*PI);
	if (endWithFirst === undefined)
	{
		if (maxAngle >= 2*PI - 0.001) // Can this be done better?
		{
			endWithFirst = false;
		}
		else
		{
			endWithFirst = true;
		}
	}
	maxBendOff = (maxBendOff || 0);
	// Check arguments
	if (maxBendOff > PI/2 || maxBendOff < 0)
		warn('placeCircularWall maxBendOff sould satisfy 0 < maxBendOff < PI/2 (~1.5) but it is: ' + maxBendOff);
	for (var elementIndex = 0; elementIndex < wallPart.length; elementIndex++)
		if (wallStyles[style][wallPart[elementIndex]].bending != 0)
			warn("Bending is not supported by placeCircularWall but a bending wall element is used: " + wallPart[elementIndex]);
	// Setup number of wall parts
	var totalLength = maxAngle * radius;
	var wallPartLength = 0;
	for (var elementIndex = 0; elementIndex < wallPart.length; elementIndex++)
		wallPartLength += wallStyles[style][wallPart[elementIndex]].width;
	var numParts = 0;
	if (endWithFirst == true)
	{
		numParts = ceil((totalLength - wallStyles[style][wallPart[0]].width) / wallPartLength);
	}
	else
	{
		numParts = ceil(totalLength / wallPartLength);
	}
	// Setup scale factor
	var scaleFactor = 1;
	if (endWithFirst == true)
		scaleFactor = totalLength / (numParts * wallPartLength + wallStyles[style][wallPart[0]].width)
	else
		scaleFactor = totalLength / (numParts * wallPartLength);
	// Place wall entities
	var actualAngle = orientation + (2*PI - maxAngle) / 2;
	var x = centerX + radius*cos(actualAngle);
	var y = centerY + radius*sin(actualAngle);
	for (var partIndex = 0; partIndex < numParts; partIndex++)
	{
		for (var elementIndex = 0; elementIndex < wallPart.length; elementIndex++)
		{
			var wallEle = wallStyles[style][wallPart[elementIndex]];
			// Width correction
			var addAngle = scaleFactor * wallEle.width / radius;
			var targetX = centerX + radius * cos(actualAngle + addAngle);
			var targetY = centerY + radius * sin(actualAngle + addAngle);
			var placeX = x + (targetX - x)/2;
			var placeY = y + (targetY - y)/2;
			var placeAngle = actualAngle + addAngle/2;
			// Indent correction
			placeX -= wallEle.indent * cos(placeAngle);
			placeY -= wallEle.indent * sin(placeAngle);
			// Placement
			if (wallEle.entity !== undefined)
				placeObject(placeX, placeY, wallEle.entity, playerId, placeAngle + wallEle.angle);
			// Prepare for the next wall element
			actualAngle += addAngle;
			x = centerX + radius*cos(actualAngle);
			y = centerY + radius*sin(actualAngle);
		}
	}
	if (endWithFirst == true)
	{
		var wallEle = wallStyles[style][wallPart[0]];
		var addAngle = scaleFactor * wallEle.width / radius;
		var targetX = centerX + radius * cos(actualAngle + addAngle);
		var targetY = centerY + radius * sin(actualAngle + addAngle);
		var placeX = x + (targetX - x)/2;
		var placeY = y + (targetY - y)/2;
		var placeAngle = actualAngle + addAngle/2;
		placeObject(placeX, placeY, wallEle.entity, playerId, placeAngle + wallEle.angle);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Place a polygonal wall of repeatant wall elements given in the argument wallPart arround centerX/centerY with the given radius
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: Don't use wall elements with bending like corners!
// TODO: Check some arguments
// TODO: Add eccentricity and irregularity (how far the corners can differ from a regulare convex poygon)
function placePolygonalWall(centerX, centerY, radius, wallPart, cornerWallElement, style, playerId, orientation, numCorners, skipFirstWall)
{
	// Setup optional arguments to the default
	wallPart = (wallPart || ['wall']);
	cornerWallElement = (cornerWallElement || 'tower'); // Don't use wide elements for this. Not supported well...
	style = (style || 'palisades');
	playerId = (playerId || 0);
	orientation = (orientation || 0);
	numCorners = (numCorners || 8);
	skipFirstWall = (skipFirstWall || false);
	// Setup angles
	var angleAdd = 2*PI/numCorners;
	var angleStart = orientation - angleAdd/2;
	// Setup corners
	var corners = [];
	for (var i = 0; i < numCorners; i++)
		corners.push([centerX + radius*cos(angleStart + i*angleAdd), centerY + radius*sin(angleStart + i*angleAdd)]);
	// Place Corners and walls
	for (var i = 0; i < numCorners; i++)
	{
		var angleToCorner = getAngle(corners[i][0], corners[i][1], centerX, centerY);
		placeObject(corners[i][0], corners[i][1], wallStyles[style][cornerWallElement].entity, playerId, angleToCorner);
		if (!(skipFirstWall && i == 0))
		{
			placeLinearWall(
				// Adjustment to the corner element width (approximately)
				corners[i][0] + wallStyles[style][cornerWallElement].width/2 * sin(angleToCorner + angleAdd/2), // startX
				corners[i][1] - wallStyles[style][cornerWallElement].width/2 * cos(angleToCorner + angleAdd/2), // startY
				corners[(i+1)%numCorners][0] - wallStyles[style][cornerWallElement].width/2 * sin(angleToCorner + angleAdd/2), // targetX
				corners[(i+1)%numCorners][1] + wallStyles[style][cornerWallElement].width/2 * cos(angleToCorner + angleAdd/2), // targetY
				wallPart, style, playerId);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Place an irregular polygonal wall of some wall parts to choose from arround centerX/centerY with the given radius
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: Under construction!!!
// NOTE: wallPartList is put to the end because it's hardest to set
// NOTE: Don't use wall elements with bending like corners!
// TODO: Check some arguments
// TODO: Add eccentricity and irregularity (how far the corners can differ from a regulare convex poygon)
function placeIrregularPolygonalWall(centerX, centerY, radius, cornerWallElement, style, playerId, orientation, numCorners, irregularity, skipFirstWall, wallPartsAssortment)
{
	// Generating a generic wall part assortment with each wall part including 1 gate enlengthend by walls and towers
	// NOTE: It might be a good idea to write an own function for that...
	var defaultWallPartsAssortment = [['wallShort'], ['wall'], ['wallLong'], ['gate', 'tower', 'wallShort']];
	var centeredWallPart = ['entry']; // NOTE: Since gates are not functional yet entrys are used instead...
	var extandingWallPartAssortment = [['tower', 'wallLong'], ['tower', 'wall']];
	defaultWallPartsAssortment.push(centeredWallPart)
	for (var i = 0; i < extandingWallPartAssortment.length; i++)
	{
		var wallPart = centeredWallPart;
		for (var j = 0; j < radius; j++)
		{
			if (j%2 == 0)
				wallPart = wallPart.concat(extandingWallPartAssortment[i]);
			else
			{
				extandingWallPartAssortment[i].reverse();
				wallPart = extandingWallPartAssortment[i].concat(wallPart);
				extandingWallPartAssortment[i].reverse();
			}
			defaultWallPartsAssortment.push(wallPart);
		}
	}
	// Setup optional arguments to the default
	wallPartsAssortment = (wallPartsAssortment || defaultWallPartsAssortment);
	cornerWallElement = (cornerWallElement || 'tower'); // Don't use wide elements for this. Not supported well...
	style = (style || 'palisades');
	playerId = (playerId || 0);
	orientation = (orientation || 0);
	numCorners = (numCorners || randInt(5, 7));
	irregularity = (irregularity || 0.5);
	skipFirstWall = (skipFirstWall || false);
	// Setup angles
	var angleToCover = 2*PI;
	var angleAddList = [];
	for (var i = 0; i < numCorners; i++)
	{
		// Randomize covered angles. Variety scales down with raising angle though...
		angleAddList.push(angleToCover/(numCorners-i) * (1 + randFloat(-irregularity, irregularity)));
		angleToCover -= angleAddList[angleAddList.length - 1];
	}
	// Setup corners
	var corners = [];
	var angleActual = orientation - angleAddList[0]/2;
	for (var i = 0; i < numCorners; i++)
	{
		corners.push([centerX + radius*cos(angleActual), centerY + radius*sin(angleActual)]);
		if (i < numCorners - 1)
			angleActual += angleAddList[i+1]
	}
	// Setup best wall parts for the different walls (a bit confusing naming...)
	var wallPartLengths = [];
	var maxWallPartLength = 0;
	for (var partIndex = 0; partIndex < wallPartsAssortment.length; partIndex++)
	{
		wallPartLengths.push(getWallLength(wallPartsAssortment[partIndex], style));
		log("wallPartLengths = " + wallPartLengths);
		if (wallPartLengths[partIndex] > maxWallPartLength)
			maxWallPartLength = wallPartLengths[partIndex];
	}
	var wallPartList = []; // This is the list of the wall parts to use for the walls between the corners, not to confuse with wallPartsAssortment!
	for (var i = 0; i < numCorners; i++)
	{
		var bestWallPart = []; // This is a simpel wall part not a wallPartsAssortment!
		var bestWallLength = 99999999;
		// NOTE: This is not exsactly like the length the wall will be in the end. Has to be tweeked...
		var wallLength = getDistance(corners[i][0], corners[i][1], corners[(i+1)%numCorners][0], corners[(i+1)%numCorners][1])
		var numWallParts = ceil(wallLength/maxWallPartLength);
		log("numWallParts = " + numWallParts);
		log("wallLength = " + wallLength);
		for (var partIndex = 0; partIndex < wallPartsAssortment.length; partIndex++)
		{
			if (numWallParts*wallPartLengths[partIndex] < bestWallLength && numWallParts*wallPartLengths[partIndex] > wallLength)
			{
				bestWallPart = wallPartsAssortment[partIndex];
				bestWallLength = numWallParts*wallPartLengths[partIndex];
			}
		}
		wallPartList.push(bestWallPart)
		log("bestWallLength = " + bestWallLength);
		log("bestWallPart = " + bestWallPart);
	}
	log("getWallLength(['wall', 'gate'], style) = " + getWallLength(['wall', 'gate'], style));
	log("wallPartList = " + wallPartList);
	// Place Corners and walls
	for (var i = 0; i < numCorners; i++)
	{
		log("wallPartList[" + i + "] = " + wallPartList[i]);
		var angleToCorner = getAngle(corners[i][0], corners[i][1], centerX, centerY);
		placeObject(corners[i][0], corners[i][1], wallStyles[style][cornerWallElement].entity, playerId, angleToCorner);
		if (!(skipFirstWall && i == 0))
		{
			placeLinearWall(
				// Adjustment to the corner element width (approximately)
				corners[i][0] + wallStyles[style][cornerWallElement].width/2 * sin(angleToCorner + angleAddList[i]/2), // startX
				corners[i][1] - wallStyles[style][cornerWallElement].width/2 * cos(angleToCorner + angleAddList[i]/2), // startY
				corners[(i+1)%numCorners][0] - wallStyles[style][cornerWallElement].width/2 * sin(angleToCorner + angleAddList[(i+1)%numCorners]/2), // targetX
				corners[(i+1)%numCorners][1] + wallStyles[style][cornerWallElement].width/2 * cos(angleToCorner + angleAddList[(i+1)%numCorners]/2), // targetY
				wallPartList[i], style, playerId, false);
		}
	}
}

// Just a test function, USE WITH CARE!
// radius: A float seting the aproximate radius of the fortress
// maxBendOff: Optional, The maximum random bending offset of the wall elements NOTE: If maxBendOff > PI/2 the wall might never close!!!
// wallPart: Optional, An array of wall element string, example: ["tower", "wall"] NOTE: Don't use wall elements with bending!!!
function placeGenericFortress(centerX, centerY, radius, playerId, wallPart, style, maxBendOff)
{
	if (wallPart == undefined)
		wallPart = ['tower', 'wall', 'tower', 'entry', 'tower', 'wall'];
	if (maxBendOff > PI/2 || maxBendOff < 0)
		warn("setGenericFortress maxBendOff sould satisfy 0 < maxBendOff < PI/2 (~1.5) but it is: " + maxBendOff);
	if (maxBendOff === undefined)
		maxBendOff = 0;
	var wallLength = 0;
	for (var eleIndex = 0; eleIndex < wallPart.length; eleIndex++)
	{
		wallLength += wallStyles[style][wallPart[eleIndex]].width
	}
	if (wallLength > 2*PI*radius || wallLength <= 0)
		warn("setGenericFortress: sum of the width of wall's elements should satisfy 0 < total length < 2*PI*radius but: radius = " + radius + ", wallPart = " + wallPart + " with total length of " + wallLength);
	
	var minEleWidth = 1000000; // Assuming the smallest element is about as wide as deep.
	for (var eleIndex = 0; eleIndex < wallPart.length; eleIndex++)
	{
		eleWidth = wallStyles[style][wallPart[eleIndex]].width;
		if (eleWidth < minEleWidth)
			minEleWidth = eleWidth;
	}
	var widthSafty = minEleWidth/4; // Can be done better...
	
	var x = radius;
	var y = 0;
	var angle = 0;
	var targetX = radius;
	var targetY = 0;
	var targetReached = false;
	var eleIndex = 0;
	while (targetReached == false)
	{
		var wallElement = wallStyles[style][wallPart[eleIndex % wallPart.length]];
		var eleWidth = wallElement.width - widthSafty;
		// Stabalized bendOff
		var actualRadius = getDistance(centerX, centerY, centerX + x, centerY + y);
		var bendOff = randFloat(-maxBendOff*radius/actualRadius, maxBendOff*actualRadius/radius);
		// Element width in radians
		var eleAngleWidth = eleWidth*cos(bendOff) / radius; // A/L = da/dl -> da = A * dl / L -> da = 2*PI * eleWidth / (2*PI * radius) -> da = eleWidth/radius
		var eleAngle = angle + eleAngleWidth/2 + bendOff;
		var placeX = x - eleWidth/2 * sin(eleAngle);
		var placeY = y + eleWidth/2 * cos(eleAngle);
		if (wallElement.entity)
			placeObject(centerX + placeX, centerY + placeY, wallElement.entity, playerId, eleAngle + wallElement.angle);
		x = placeX - eleWidth/2 * sin(eleAngle);
		y = placeY + eleWidth/2 * cos(eleAngle);
		angle += eleAngleWidth;
		if (eleIndex % wallPart.length == 0 && eleIndex > wallPart.length && (getDistance(x, y, targetX, targetY) < wallLength || angle > 2*PI))
			targetReached = true;
		eleIndex++;
		// if (eleIndex == 10)
			// break;
	}
	placeLinearWall(centerX + x, centerY + y, centerX + targetX, centerY + targetY, ['wall', 'tower'], style, playerId, true);
}
