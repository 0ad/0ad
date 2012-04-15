////////////////////////////////////////////////////////////////////
// This file contains functionality to place walls on random maps //
////////////////////////////////////////////////////////////////////

// To do:
// Add some more checks and warnings for example if wall elements with bending are used for linear/circular wall placement
// Add some more documentation
// Add further wall elements cornerHalfIn, cornerHalfOut and adjust default fortress types
// Add wall style 'roads'
// Add trsures to 'others'


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
	// In this sense 'left'/'right' means left/right of you when you stand upon the wall and look 'outside'
	// The wall is build towards 'left' so the next wall element will be placed left of the previous one (towards positive Y by default)
	// In this sense the wall's direction is left meaning the 'bending' of a corner is used for the element following the corner
	// With 'inside' and 'outside' defined as above, corners bend 'in'/'out' meaning placemant angle is increased/decreased (counter clockwise like object placement)
	this.type = type;
	// Wall element type documentation:
	// Enlengthening straight blocking (mainly left/right symetric) wall elements (Walls and wall fortifications)
		// 'wall': A blocking straight wall element that mainly lengthens the wall, self-explanatory
		// 'wallShort': self-explanatory. NOTE: Not implemented yet, waiting for finalized templates...
		// 'wallLong': self-explanatory. NOTE: Not implemented yet, waiting for finalized templates...
		// 'tower': A blocking straight wall element with damage potential (but for palisades) that slightly lengthens the wall, exsample: wall tower, palisade tower(No attack)
		// 'wallFort': A blocking straight wall element with massive damage potential that lengthens the wall, exsample: fortress, palisade fort
	// Enlengthening straight non/custom blocking (mainly left/right symetric) wall elements (Gates and entrys)
		// 'gate': A blocking straight wall element with passability determined by owner, example: gate (Functionality not yet implemented)
		// 'entry': A wall element like the gate but without an actual template or just a flag/column/obelisk
		// 'entryTower': A non-blocking straight wall element represented by a single (maybe indented) template, example: defense tower, wall tower, outpost, watchtower
		// 'entryFort': A non-blocking straight wall element represented by a single (maybe indented) template, example: fortress, palisade fort
	// Enlengthening straight blocking non-left/right-symetric wall elements (Wall endings/closings)
		// 'endLeft': A straight wall element that (only visually) finalizes the wall to the left (e.g. right of entrys), example: wall tower, palisade ending
		// 'endRight': A straight wall element that (only visually) finalizes the wall to the right (e.g. left of entrys), example: wall tower, palisade ending
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

////////////////////////////////////////////////////
// Setup data structure for some default wall styles
////////////////////////////////////////////////////

// A wall style is an associative array with all wall elements of that style in it associated with the wall element type string.
// wallStyles holds all the wall styles within an associative array while a wall style is associated with the civ string or another descriptive strings like 'palisades', 'fence', 'cart', 'celt'...
var wallStyles = {};
// Add civilisation wall style 'cart'
wallStyles['cart'] = {};
wallStyles['cart']['wall'] = new WallElement('wall', 'structures/cart_wall', 0*PI, 6.2);
wallStyles['cart']['tower'] = new WallElement('tower', 'structures/cart_wall_tower', PI, 2.7);
wallStyles['cart']['wallFort'] = new WallElement('wallFort', 'structures/cart_fortress', PI, 5.1, 1.6);
wallStyles['cart']['gate'] = new WallElement('gate', 'structures/cart_wall_gate', 0*PI, 6.2);
wallStyles['cart']['entry'] = new WallElement('entry', undefined, wallStyles['cart']['gate'].angle, wallStyles['cart']['gate'].width);
wallStyles['cart']['entryTower'] = new WallElement('entryTower', 'structures/cart_defense_tower', PI, 6.2, -5);
wallStyles['cart']['entryFort'] = new WallElement('entryFort', 'structures/cart_fortress', PI, 12, 7);
wallStyles['cart']['endRight'] = new WallElement('endRight', wallStyles['cart']['tower'].entity, wallStyles['cart']['tower'].angle, wallStyles['cart']['tower'].width);
wallStyles['cart']['endLeft'] = new WallElement('endLeft', wallStyles['cart']['tower'].entity, wallStyles['cart']['tower'].angle, wallStyles['cart']['tower'].width);
wallStyles['cart']['cornerIn'] = new WallElement('cornerIn', 'structures/cart_wall_tower', 5*PI/4, 0, 0.8, PI/2);
wallStyles['cart']['cornerOut'] = new WallElement('cornerOut', 'structures/cart_wall_tower', 3*PI/4, 1.3 /*1.6*/, 0, -PI/2);
wallStyles['cart']['outpost'] = new WallElement('outpost', 'structures/cart_outpost', PI, 0, -5);
wallStyles['cart']['house'] = new WallElement('house', 'structures/cart_house', PI, 0, 7);
wallStyles['cart']['barracks'] = new WallElement('barracks', 'structures/cart_barracks', PI, 0, 7);
// Add civilisation wall style 'celt'
wallStyles['celt'] = {};
wallStyles['celt']['wall'] = new WallElement('wall', 'structures/celt_wall', 0*PI, 5.9);
wallStyles['celt']['tower'] = new WallElement('tower', 'structures/celt_wall_tower', PI, 1.9);
wallStyles['celt']['wallFort'] = new WallElement('wallFort', 'structures/celt_fortress_g', PI, 4.2, 1.5);
wallStyles['celt']['gate'] = new WallElement('gate', 'structures/celt_wall_gate', 0*PI, 5.6);
wallStyles['celt']['entry'] = new WallElement('entry', undefined, wallStyles['celt']['gate'].angle, wallStyles['celt']['gate'].width);
wallStyles['celt']['entryTower'] = new WallElement('entryTower', 'structures/celt_defense_tower', PI, 6, -5);
wallStyles['celt']['entryFort'] = new WallElement('entryFort', 'structures/celt_fortress_b', PI, 8, 5);
wallStyles['celt']['endRight'] = new WallElement('endRight', wallStyles['celt']['tower'].entity, wallStyles['celt']['tower'].angle, wallStyles['celt']['tower'].width);
wallStyles['celt']['endLeft'] = new WallElement('endLeft', wallStyles['celt']['tower'].entity, wallStyles['celt']['tower'].angle, wallStyles['celt']['tower'].width);
wallStyles['celt']['cornerIn'] = new WallElement('cornerIn', 'structures/celt_wall_tower', 5*PI/4, 0, 0.7, PI/2);
wallStyles['celt']['cornerOut'] = new WallElement('cornerOut', 'structures/celt_wall_tower', 3*PI/4, 1.3, 0, -PI/2);
wallStyles['celt']['outpost'] = new WallElement('outpost', 'structures/celt_outpost', PI, 0, -5);
wallStyles['celt']['house'] = new WallElement('house', 'structures/celt_house', PI, 0, 3);
wallStyles['celt']['barracks'] = new WallElement('barracks', 'structures/celt_barracks', PI, 0, 7);
// Add civilisation wall style 'hele'
wallStyles['hele'] = {};
wallStyles['hele']['wall'] = new WallElement('wall', 'structures/hele_wall', 0*PI, 5.95);
wallStyles['hele']['tower'] = new WallElement('tower', 'structures/hele_wall_tower', PI, 1.5);
wallStyles['hele']['wallFort'] = new WallElement('wallFort', 'structures/hele_fortress', 2*PI/2 /* PI/2 */, 5.1 /* 5.6 */, 1.9 /* 1.9 */);
wallStyles['hele']['gate'] = new WallElement('gate', 'structures/hele_wall_gate', 0*PI, 9.1);
wallStyles['hele']['entry'] = new WallElement('entry', undefined, wallStyles['hele']['gate'].angle, wallStyles['hele']['gate'].width);
wallStyles['hele']['entryTower'] = new WallElement('entry', 'structures/hele_defense_tower', PI, 9.1, -5);
wallStyles['hele']['entryFort'] = new WallElement('entryFort', 'structures/hele_fortress', 5*PI/2, 12, 7);
wallStyles['hele']['endRight'] = new WallElement('endRight', wallStyles['hele']['tower'].entity, wallStyles['hele']['tower'].angle, wallStyles['hele']['tower'].width);
wallStyles['hele']['endLeft'] = new WallElement('endLeft', wallStyles['hele']['tower'].entity, wallStyles['hele']['tower'].angle, wallStyles['hele']['tower'].width);
wallStyles['hele']['cornerIn'] = new WallElement('cornerIn', 'structures/hele_wall_tower', 5*PI/4, 0, 0.5, PI/2);
wallStyles['hele']['cornerOut'] = new WallElement('cornerOut', 'structures/hele_wall_tower', 3*PI/4, 1, 0, -PI/2);
wallStyles['hele']['outpost'] = new WallElement('outpost', 'structures/hele_outpost', PI, 0, -5);
wallStyles['hele']['house'] = new WallElement('house', 'structures/hele_house', 3*PI/2, 0, 6);
wallStyles['hele']['barracks'] = new WallElement('barracks', 'structures/hele_barracks', PI, 0, 6);
// Add civilisation wall style 'iber'
wallStyles['iber'] = {};
wallStyles['iber']['wall'] = new WallElement('wall', 'structures/iber_wall', 0*PI, 6);
wallStyles['iber']['tower'] = new WallElement('tower', 'structures/iber_wall_tower', PI, 1.7);
wallStyles['iber']['wallFort'] = new WallElement('wallFort', 'structures/iber_fortress', PI, 4.6, 0.7);
wallStyles['iber']['gate'] = new WallElement('gate', 'structures/iber_wall_gate', 0*PI, 7.9);
wallStyles['iber']['entry'] = new WallElement('entry', undefined, wallStyles['iber']['gate'].angle, wallStyles['iber']['gate'].width);
wallStyles['iber']['entryTower'] = new WallElement('entryTower', 'structures/iber_wall_tower', PI, 6.9, -5);
wallStyles['iber']['entryFort'] = new WallElement('entryFort', 'structures/iber_fortress', PI/2, 12, 7);
wallStyles['iber']['endRight'] = new WallElement('endRight', wallStyles['iber']['tower'].entity, wallStyles['iber']['tower'].angle, wallStyles['iber']['tower'].width);
wallStyles['iber']['endLeft'] = new WallElement('endLeft', wallStyles['iber']['tower'].entity, wallStyles['iber']['tower'].angle, wallStyles['iber']['tower'].width);
wallStyles['iber']['cornerIn'] = new WallElement('cornerIn', 'structures/iber_wall_tower', 5*PI/4, 1.7, 0, PI/2);
wallStyles['iber']['cornerOut'] = new WallElement('cornerOut', 'structures/iber_wall_tower', 3*PI/4, 1.7, 0, -PI/2);
wallStyles['iber']['outpost'] = new WallElement('outpost', 'structures/iber_outpost', PI, 0, -5);
wallStyles['iber']['house'] = new WallElement('house', 'structures/iber_house', PI, 0, 4);
wallStyles['iber']['barracks'] = new WallElement('barracks', 'structures/iber_barracks', PI, 0, 7);
// Add civilisation wall style 'pers'
wallStyles['pers'] = {};
wallStyles['pers']['wall'] = new WallElement('wall', 'structures/pers_wall', 0*PI, 5.9);
wallStyles['pers']['tower'] = new WallElement('tower', 'structures/pers_wall_tower', PI, 1.7);
wallStyles['pers']['wallFort'] = new WallElement('wallFort', 'structures/pers_fortress', PI, 5.6/*5.5*/, 1.9/*1.7*/);
wallStyles['pers']['gate'] = new WallElement('gate', 'structures/pers_wall_gate', 0*PI, 6);
wallStyles['pers']['entry'] = new WallElement('entry', undefined, wallStyles['pers']['gate'].angle, wallStyles['pers']['gate'].width);
wallStyles['pers']['entryTower'] = new WallElement('entry', 'structures/pers_defense_tower', PI, 6, -5);
wallStyles['pers']['entryFort'] = new WallElement('entryFort', 'structures/pers_fortress', PI, 12, 7);
wallStyles['pers']['endRight'] = new WallElement('endRight', wallStyles['pers']['tower'].entity, wallStyles['pers']['tower'].angle, wallStyles['pers']['tower'].width);
wallStyles['pers']['endLeft'] = new WallElement('endLeft', wallStyles['pers']['tower'].entity, wallStyles['pers']['tower'].angle, wallStyles['pers']['tower'].width);
wallStyles['pers']['cornerIn'] = new WallElement('cornerIn', 'structures/pers_wall_tower', 5*PI/4, 0.2, 0.5, PI/2);
wallStyles['pers']['cornerOut'] = new WallElement('cornerOut', 'structures/pers_wall_tower', 3*PI/4, 0.8, 0, -PI/2);
wallStyles['pers']['outpost'] = new WallElement('outpost', 'structures/pers_outpost', PI, 0, -5);
wallStyles['pers']['house'] = new WallElement('house', 'structures/pers_house', PI, 0, 6);
wallStyles['pers']['barracks'] = new WallElement('barracks', 'structures/pers_barracks', PI, 0, 7);
// Add civilisation wall style 'rome'
wallStyles['rome'] = {};
wallStyles['rome']['wall'] = new WallElement('wall', 'structures/rome_wall', 0*PI, 5.9);
wallStyles['rome']['tower'] = new WallElement('tower', 'structures/rome_wall_tower', PI, 2.1);
wallStyles['rome']['wallFort'] = new WallElement('wallFort', 'structures/rome_fortress', PI, 6.3, 2.1);
wallStyles['rome']['gate'] = new WallElement('gate', 'structures/rome_wall_gate', 0*PI, 5.9);
wallStyles['rome']['entry'] = new WallElement('entry', undefined, wallStyles['rome']['gate'].angle, wallStyles['rome']['gate'].width);
wallStyles['rome']['entryTower'] = new WallElement('entryTower', 'structures/rome_defense_tower', PI, 5.9, -5);
wallStyles['rome']['entryFort'] = new WallElement('entryFort', 'structures/rome_fortress', PI, 12, 7);
wallStyles['rome']['endRight'] = new WallElement('endRight', wallStyles['rome']['tower'].entity, wallStyles['rome']['tower'].angle, wallStyles['rome']['tower'].width);
wallStyles['rome']['endLeft'] = new WallElement('endLeft', wallStyles['rome']['tower'].entity, wallStyles['rome']['tower'].angle, wallStyles['rome']['tower'].width);
wallStyles['rome']['cornerIn'] = new WallElement('cornerIn', 'structures/rome_wall_tower', 5*PI/4, 0, 0.7, PI/2);
wallStyles['rome']['cornerOut'] = new WallElement('cornerOut', 'structures/rome_wall_tower', 3*PI/4, 1.1, 0, -PI/2);
wallStyles['rome']['outpost'] = new WallElement('outpost', 'structures/rome_outpost', PI, 0, -5);
wallStyles['rome']['house'] = new WallElement('house', 'structures/rome_house', PI, 0, 7);
wallStyles['rome']['barracks'] = new WallElement('barracks', 'structures/rome_barracks', PI, 0, 6);
// Add special wall style 'romeSiege'
wallStyles['romeSiege'] = {};
wallStyles['romeSiege']['wall'] = new WallElement('wall', 'structures/rome_siege_wall', 0*PI, 6.2);
wallStyles['romeSiege']['tower'] = new WallElement('tower', 'structures/rome_siege_wall_tower', PI, 0);
wallStyles['romeSiege']['wallFort'] = new WallElement('wallFort', 'structures/rome_army_camp', PI, 7.2, 2);
wallStyles['romeSiege']['gate'] = new WallElement('gate', 'structures/rome_siege_wall_gate', 0*PI, 5.9);
wallStyles['romeSiege']['entry'] = new WallElement('entry', undefined, wallStyles['romeSiege']['gate'].angle, wallStyles['romeSiege']['gate'].width);
wallStyles['romeSiege']['entryTower'] = new WallElement('entryTower', 'structures/rome_defense_tower', PI, 5.9, -4);
wallStyles['romeSiege']['entryFort'] = new WallElement('entryFort', 'structures/rome_army_camp', PI, 12, 7);
wallStyles['romeSiege']['endRight'] = new WallElement('endRight', wallStyles['romeSiege']['tower'].entity, wallStyles['romeSiege']['tower'].angle, wallStyles['romeSiege']['tower'].width);
wallStyles['romeSiege']['endLeft'] = new WallElement('endLeft', wallStyles['romeSiege']['tower'].entity, wallStyles['romeSiege']['tower'].angle, wallStyles['romeSiege']['tower'].width);
wallStyles['romeSiege']['cornerIn'] = new WallElement('cornerIn', 'structures/rome_defense_tower', 5*PI/4, 0.1, 0.3, PI/2);
wallStyles['romeSiege']['cornerIn2'] = new WallElement('cornerIn2', undefined, 5*PI/4, -1.6, 0, PI/2); // Grafic glitch
wallStyles['romeSiege']['cornerOut'] = new WallElement('cornerOut', 'structures/rome_siege_wall_tower', 3*PI/4, 1.4, -0.2, -PI/2);
wallStyles['romeSiege']['outpost'] = new WallElement('outpost', 'structures/rome_outpost', PI, 0, -5);
wallStyles['romeSiege']['house'] = new WallElement('house', 'structures/rome_tent', PI, 0, 4);
wallStyles['romeSiege']['barracks'] = new WallElement('barracks', 'structures/rome_barracks', PI, 0, 6);
// Add special wall style 'palisades'
wallStyles['palisades'] = {};
wallStyles['palisades']['wall'] = new WallElement('wall', 'other/palisades_rocks_straight', -PI/2, 2.5);
wallStyles['palisades']['tower'] = new WallElement('tower', 'other/palisades_rocks_tower', -PI/2, 0.7);
wallStyles['palisades']['wallFort'] = new WallElement('wallFort', 'other/palisades_rocks_fort', PI, 1.7);
wallStyles['palisades']['gate'] = new WallElement('gate', 'other/palisades_rocks_gate', 0*PI, 3.6);
wallStyles['palisades']['entry'] = new WallElement('entry', undefined, wallStyles['palisades']['gate'].angle, wallStyles['palisades']['gate'].width);
wallStyles['palisades']['entryTower'] = new WallElement('entryTower', 'other/palisades_rocks_watchtower', 0*PI, 3.6, -2);
wallStyles['palisades']['entryFort'] = new WallElement('entryFort', 'other/palisades_rocks_fort', PI, 6, 3);
wallStyles['palisades']['endRight'] = new WallElement('endRight', 'other/palisades_rocks_end', -PI/2, 0.2);
wallStyles['palisades']['endLeft'] = new WallElement('endLeft', 'other/palisades_rocks_end', PI/2, 0.2);
wallStyles['palisades']['cornerIn'] = new WallElement('cornerIn', 'other/palisades_rocks_curve', 3*PI/4, 2.1, 0.7, PI/2);
wallStyles['palisades']['cornerOut'] = new WallElement('cornerOut', 'other/palisades_rocks_curve', 5*PI/4, 2.1, -0.7, -PI/2);
wallStyles['palisades']['outpost'] = new WallElement('outpost', 'other/palisades_rocks_outpost', PI, 0, -2);
wallStyles['palisades']['house'] = new WallElement('house', 'other/celt_hut', PI, 0, 5);
wallStyles['palisades']['barracks'] = new WallElement('barracks', 'other/celt_tavern', PI, 0, 5);
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
var wallPart = ['entry', 'endLeft', 'wall', 'cornerIn', 'wall', 'endRight'];
fortressTypes['tiny'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'small'
fortressTypes['small'] = new Fortress('small');
var wallPart = ['entry', 'endLeft', 'wall', 'outpost', 'wall',
	'cornerIn', 'wall', 'outpost', 'wall', 'endRight'];
fortressTypes['small'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'medium'
fortressTypes['medium'] = new Fortress('medium');
var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall',
	'cornerIn', 'wall', 'tower', 'wall', 'endRight'];
fortressTypes['medium'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'normal'
fortressTypes['normal'] = new Fortress('normal');
var wallPart = ['entry', 'endLeft', 'wall', 'outpost', 'wall', 'cornerIn', 'wall',
	'cornerOut', 'wall', 'cornerIn', 'wall', 'outpost', 'wall', 'endRight'];
fortressTypes['normal'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'large'
fortressTypes['large'] = new Fortress('large');
var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall', 'cornerIn', 'wall',
	'cornerOut', 'wall', 'cornerIn', 'wall', 'tower', 'wall', 'endRight'];
fortressTypes['large'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'veryLarge'
fortressTypes['veryLarge'] = new Fortress('veryLarge');
var wallPart = ['entry', 'endLeft', 'wall', 'outpost', 'wall', 'cornerIn', 'wall', 'outpost', 'wall',
	'cornerOut', 'wall', 'outpost', 'wall', 'cornerIn', 'wall', 'outpost', 'wall', 'endRight'];
fortressTypes['veryLarge'].wall = wallPart.concat(wallPart, wallPart, wallPart);
// Add fortress type 'giant'
fortressTypes['giant'] = new Fortress('giant');
var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall', 'cornerIn', 'wall', 'tower', 'wall',
	'cornerOut', 'wall', 'tower', 'wall', 'cornerIn', 'wall', 'tower', 'wall', 'endRight'];
fortressTypes['giant'].wall = wallPart.concat(wallPart, wallPart, wallPart);

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


//////////////////////////////////////////////////////////////////
// Define the abstract getWallAlignment and getWallCenter function
//////////////////////////////////////////////////////////////////

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


/////////////////////////////////////////////////////////////////////////////
// Get the center of a wall (mainly usefull for closed walls like fortresses)
/////////////////////////////////////////////////////////////////////////////

// Center calculation works like getting the center of mass assuming all wall elements have the same 'waight'
// It returns the vector (array [x, y]) from the first wall element to the center
// So if used to preset the center in an instance of the fortress class use the negative values (I think)
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


/////////////////////////////////////////////
// Define the different wall placer functions
/////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////
// Place simple wall starting with the first wall element placed at startX/startY
/////////////////////////////////////////////////////////////////////////////////

// orientation: 0 means 'outside' or 'front' of the wall is right (positive X) like placeObject
// 	It will then be build towards top (positive Y) if no bending wall elements like corners are used
//	Raising orientation means the wall is rotated counter-clockwise like placeObject
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
		var startX = centerX - center[0] * cos(orientation) - center[1] * sin(orientation);
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
	endWithFirst = (endWithFirst || true);
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
	if (maxBendOff > PI/2 || maxBendOff < 0)
		warn('placeCircularWall maxBendOff sould satisfy 0 < maxBendOff < PI/2 (~1.5) but it is: ' + maxBendOff);
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
