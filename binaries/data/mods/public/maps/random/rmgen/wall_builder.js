// Functionality to create walls and fortresses
// Wall object
function wallElement(type)
{
	// NOTE: Not all wall elements have a symetry. So there's an direction 'outside' (towards top/positive Z by default)
	// In this sense 'right'/'left' means right/left of you when you stand upon the wall and look 'outside'
	// The wall is build towards 'right' so the next wall element will be placed right of the previous one
	// In this sense the wall's direction is right meaning the 'bending' of a corner is used for the element following the corner
	// With 'inside' and 'outside' defined as above, corners bend 'in'/'out' meaning placemant angle is increased/decreased (circlewise like building placement)
	this.type = type; // Descriptive string for easy to add wall styles. Not really needed (wallTool.wallTypes['type string'] is used)
	// Wall type documentation:
	// wall: A blocking straight wall element that mainly lengthens the wall, self-explanatory
	// tower: A blocking straight wall element with damage potential that slightly lengthens the wall, exsample: wall tower, palisade tower(No attack?)
	// wallFort: A blocking straight wall element with massive damage potential that lengthens the wall, exsample: fortress, palisade fort
		// NOTE: Not really needed but for palisades, I don't find it tragic that palisades are much less powerfull
	// outpost: A zero-length wall element without bending far indented so it stands outside the wall, exsample: outpost, defense tower, watchtower
	// house: A zero-length wall element without bending far indented so it stands inside the wall, exsample: house, hut, longhouse
	// gate: A blocking straight wall element with passability determined by owner, example: gate
	// entry: A non-blocking straight wall element represented by a single indented template, example: defense tower, wall tower, outpost, watchtower, nothing
	// wideEntry: A non-blocking straight wall element represented by a single indented template, example: fortress? a unit?
		// NOTE: Not really needed...
	// endRight: A straight wall element that (only) visually ends the wall to the right (e.g. left of entrys), example: wall tower, palisade ending
	// endLeft: A straight wall element that (only) visually ends the wall to the left (e.g. right of entrys), example: wall tower, palisade ending
	// cornerIn: A wall element bending the wall PI/2 (90°) 'inside' (see above), example: wall tower, palisade curve
	// cornerOut: A wall element bending the wall PI/2 (90°) 'outside' (see above), example: wall tower, palisade curve (placed the other way arround)
	// barracks: Adding in progress...
	this.entity = undefined; // Template (string) that will represent the wall element, example: "other/palisades_rocks_straight"
	this.angle = -PI/2; // Placement angle so that 'outside' is 'top' (directs towards positive z, like buildings)
	this.width = 0; // The width it lengthens the wall, width because it's the needed space in a right angle to 'outside'
	this.indent = 0; // The indentation means its drawn inside (positive values) or pressed outwards (negative values)
	this.bending = 0*PI; // How the angle of the wall is changed by this element for the following element, positive is bending 'in'
};

// Fortress object. A list would do for symetric fortresses but if 'getCenter' don't do sufficient the center can be set manually.
function fortress(type)
{
	this.type = type; // Only usefull to get the type of the actual fortress (by 'wallTool.fortress.type')
	this.wall = [];
	this.center = [0, 0]; // X/Z offset (in default orientation) from first wall element to center, perhaps should be the other way around...
	// NOTE: The 'angle' is not implemented yet!!!
	this.angle = 0; // Angle the first element has to be placed so that fortress orientation is with main entrance top (default), should always be 0 though.
};

// The wall tool
function wallTool(style, fortressType)
{
	// General setup
	this.angle = 0; // Not really needed, may be useful to rotate fortresses after set...
	this.wallTypes = {}; // An associative array including all wall elements associated with 'wallElement.type'.
	this.wall = []; // A list of strings (wall element types) for simple wall building
	this.center = [0, 0] // X/Z offset from first wall element to center, perhaps should be the other way around...
	
	// Presets needed for wall style
	this.wallStyles = {};
	this.style = "palisades"; // Not really needed, perhaps helpfull to check current style
	if (style !== undefined)
		this.style = style;
	
	// Presets needed for tortess types
	this.fortressTypes = {};
	this.fortress = false; // Not really needed, perhaps usefull for getting fortress type, should be renamed somehow but simply 'type' is not very descriptive and 'fortressType' may be mistaken as 'fortressTypes'
	if (fortressType !== undefined)
		this.fortress = fortressType;
	this.calculateCenter = true; // If setFortress(Type) shall recalculate the center
	
	// Adding wall styles
	// Wall style 'palisades'
	this.wallStyles["palisades"] = {};
	this.wallStyles["palisades"]["wall"] = new wallElement("wall");
	this.wallStyles["palisades"]["wall"].entity = "other/palisades_rocks_straight";
	this.wallStyles["palisades"]["wall"].angle = -PI/2;
	this.wallStyles["palisades"]["wall"].width = 2.5;
	
	this.wallStyles["palisades"]["tower"] = new wallElement("tower");
	this.wallStyles["palisades"]["tower"].entity = "other/palisades_rocks_tower";
	this.wallStyles["palisades"]["tower"].angle = -PI/2;
	this.wallStyles["palisades"]["tower"].width = 0.7;
	
	this.wallStyles["palisades"]["wallFort"] = new wallElement("wallFort");
	this.wallStyles["palisades"]["wallFort"].entity = "other/palisades_rocks_fort";
	this.wallStyles["palisades"]["wallFort"].angle = PI;
	this.wallStyles["palisades"]["wallFort"].width = 1.7;
	
	this.wallStyles["palisades"]["outpost"] = new wallElement("outpost");
	this.wallStyles["palisades"]["outpost"].entity = "other/palisades_rocks_outpost";
	this.wallStyles["palisades"]["outpost"].angle = PI;
	this.wallStyles["palisades"]["outpost"].indent = -2;
	
	this.wallStyles["palisades"]["house"] = new wallElement("house");
	this.wallStyles["palisades"]["house"].entity = "other/celt_hut";
	this.wallStyles["palisades"]["house"].angle = PI;
	this.wallStyles["palisades"]["house"].indent = 5;
	
	this.wallStyles["palisades"]["gate"] = new wallElement("gate");
	this.wallStyles["palisades"]["gate"].entity = "other/palisades_rocks_gate";
	this.wallStyles["palisades"]["gate"].angle = -PI/2;
	this.wallStyles["palisades"]["gate"].width = 3.6;
	
	this.wallStyles["palisades"]["entry"] = new wallElement("entry");
	this.wallStyles["palisades"]["entry"].entity = "other/palisades_rocks_watchtower";
	this.wallStyles["palisades"]["entry"].angle = 0*PI;
	this.wallStyles["palisades"]["entry"].width = 3.6;
	this.wallStyles["palisades"]["entry"].indent = -2;
	
	this.wallStyles["palisades"]["wideEntry"] = new wallElement("wideEntry");
	this.wallStyles["palisades"]["wideEntry"].entity = "other/palisades_rocks_fort";
	this.wallStyles["palisades"]["wideEntry"].angle = PI;
	this.wallStyles["palisades"]["wideEntry"].width = 6;
	this.wallStyles["palisades"]["wideEntry"].indent = -3;
	
	this.wallStyles["palisades"]["endRight"] = new wallElement("endRight");
	this.wallStyles["palisades"]["endRight"].entity = "other/palisades_rocks_end";
	this.wallStyles["palisades"]["endRight"].angle = -PI/2;
	this.wallStyles["palisades"]["endRight"].width = 0.2;
	
	this.wallStyles["palisades"]["endLeft"] = new wallElement("endLeft");
	this.wallStyles["palisades"]["endLeft"].entity = "other/palisades_rocks_end";
	this.wallStyles["palisades"]["endLeft"].angle = PI/2;
	this.wallStyles["palisades"]["endLeft"].width = 0.2;
	
	this.wallStyles["palisades"]["cornerIn"] = new wallElement("cornerIn");
	this.wallStyles["palisades"]["cornerIn"].entity = "other/palisades_rocks_curve";
	this.wallStyles["palisades"]["cornerIn"].angle = -PI/4;
	this.wallStyles["palisades"]["cornerIn"].width = 2.1;
	this.wallStyles["palisades"]["cornerIn"].indent = 0.7;
	this.wallStyles["palisades"]["cornerIn"].bending = PI/2;
	
	this.wallStyles["palisades"]["cornerOut"] = new wallElement("cornerOut");
	this.wallStyles["palisades"]["cornerOut"].entity = "other/palisades_rocks_curve";
	this.wallStyles["palisades"]["cornerOut"].angle = PI/4;
	this.wallStyles["palisades"]["cornerOut"].width = 2.1;
	this.wallStyles["palisades"]["cornerOut"].indent = -0.7;
	this.wallStyles["palisades"]["cornerOut"].bending = -PI/2;
	
	// Wall style 'cart'
	this.wallStyles["cart"] = {};
	this.wallStyles["cart"]["wall"] = new wallElement("wall");
	this.wallStyles["cart"]["wall"].entity = "structures/cart_wall";
	this.wallStyles["cart"]["wall"].angle = 0*PI;
	this.wallStyles["cart"]["wall"].width = 6.2;
	
	this.wallStyles["cart"]["tower"] = new wallElement("tower");
	this.wallStyles["cart"]["tower"].entity = "structures/cart_wall_tower";
	this.wallStyles["cart"]["tower"].angle = PI;
	this.wallStyles["cart"]["tower"].width = 2.7;
	
	this.wallStyles["cart"]["wallFort"] = new wallElement("wallFort");
	this.wallStyles["cart"]["wallFort"].entity = "structures/cart_fortress";
	this.wallStyles["cart"]["wallFort"].angle = PI;
	this.wallStyles["cart"]["wallFort"].width = 5.1;
	this.wallStyles["cart"]["wallFort"].indent = 1.6;
	
	this.wallStyles["cart"]["outpost"] = new wallElement("outpost");
	this.wallStyles["cart"]["outpost"].entity = "structures/cart_outpost";
	this.wallStyles["cart"]["outpost"].angle = PI;
	this.wallStyles["cart"]["outpost"].indent = -5;
	
	this.wallStyles["cart"]["house"] = new wallElement("house");
	this.wallStyles["cart"]["house"].entity = "structures/cart_house";
	this.wallStyles["cart"]["house"].angle = PI;
	this.wallStyles["cart"]["house"].indent = 7;
	
	this.wallStyles["cart"]["gate"] = new wallElement("gate");
	this.wallStyles["cart"]["gate"].entity = "structures/cart_wall_gate";
	this.wallStyles["cart"]["gate"].angle = 0*PI;
	this.wallStyles["cart"]["gate"].width = 6.2;
	
	this.wallStyles["cart"]["entry"] = new wallElement("entry");
	this.wallStyles["cart"]["entry"].entity = "structures/cart_defense_tower";
	this.wallStyles["cart"]["entry"].angle = PI;
	this.wallStyles["cart"]["entry"].width = 6.2;
	this.wallStyles["cart"]["entry"].indent = -5;
	
	this.wallStyles["cart"]["wideEntry"] = new wallElement("wideEntry");
	this.wallStyles["cart"]["wideEntry"].entity = "structures/cart_fortress";
	this.wallStyles["cart"]["wideEntry"].angle = PI;
	this.wallStyles["cart"]["wideEntry"].width = 12;
	this.wallStyles["cart"]["wideEntry"].indent = 7;
	
	this.wallStyles["cart"]["endRight"] = new wallElement("endRight");
	this.wallStyles["cart"]["endRight"].entity = "structures/cart_wall_tower";
	this.wallStyles["cart"]["endRight"].angle = PI;
	this.wallStyles["cart"]["endRight"].width = 2.7;
	
	this.wallStyles["cart"]["endLeft"] = new wallElement("endLeft");
	this.wallStyles["cart"]["endLeft"].entity = "structures/cart_wall_tower";
	this.wallStyles["cart"]["endLeft"].angle = PI;
	this.wallStyles["cart"]["endLeft"].width = 2.7;
	
	this.wallStyles["cart"]["cornerIn"] = new wallElement("cornerIn");
	this.wallStyles["cart"]["cornerIn"].entity = "structures/cart_wall_tower";
	this.wallStyles["cart"]["cornerIn"].angle = 5*PI/4;
	this.wallStyles["cart"]["cornerIn"].width = 0.1;
	this.wallStyles["cart"]["cornerIn"].indent = 0.9;
	this.wallStyles["cart"]["cornerIn"].bending = PI/2;
	
	this.wallStyles["cart"]["cornerOut"] = new wallElement("cornerOut");
	this.wallStyles["cart"]["cornerOut"].entity = "structures/cart_wall_tower";
	this.wallStyles["cart"]["cornerOut"].angle = 3*PI/4;
	this.wallStyles["cart"]["cornerOut"].width = 1.3; // 1.6
	this.wallStyles["cart"]["cornerOut"].bending = -PI/2;
	
	this.wallStyles["cart"]["barracks"] = new wallElement("barracks");
	this.wallStyles["cart"]["barracks"].entity = "structures/cart_barracks";
	this.wallStyles["cart"]["barracks"].angle = PI;
	this.wallStyles["cart"]["barracks"].indent = 7;
	
	// Wall style 'celt'
	this.wallStyles["celt"] = {};
	this.wallStyles["celt"]["wall"] = new wallElement("wall");
	this.wallStyles["celt"]["wall"].entity = "structures/celt_wall";
	this.wallStyles["celt"]["wall"].angle = 0*PI;
	this.wallStyles["celt"]["wall"].width = 5;
	this.wallStyles["celt"]["wall"].indent = 0.5;
	
	this.wallStyles["celt"]["tower"] = new wallElement("tower");
	this.wallStyles["celt"]["tower"].entity = "structures/celt_wall_tower";
	this.wallStyles["celt"]["tower"].angle = PI;
	this.wallStyles["celt"]["tower"].width = 0.7;
	
	this.wallStyles["celt"]["wallFort"] = new wallElement("wallFort");
	this.wallStyles["celt"]["wallFort"].entity = "structures/celt_fortress_g";
	this.wallStyles["celt"]["wallFort"].angle = PI;
	this.wallStyles["celt"]["wallFort"].width = 4.2;
	this.wallStyles["celt"]["wallFort"].indent = 2;
	
	this.wallStyles["celt"]["outpost"] = new wallElement("outpost");
	this.wallStyles["celt"]["outpost"].entity = "structures/celt_outpost";
	this.wallStyles["celt"]["outpost"].angle = PI;
	this.wallStyles["celt"]["outpost"].indent = -5;
	
	this.wallStyles["celt"]["house"] = new wallElement("house");
	this.wallStyles["celt"]["house"].entity = "structures/celt_house";
	this.wallStyles["celt"]["house"].angle = PI;
	this.wallStyles["celt"]["house"].indent = 5;
	
	this.wallStyles["celt"]["gate"] = new wallElement("gate");
	this.wallStyles["celt"]["gate"].entity = "structures/celt_wall_gate";
	this.wallStyles["celt"]["gate"].angle = 0*PI;
	this.wallStyles["celt"]["gate"].width = 3.2;
	
	this.wallStyles["celt"]["entry"] = new wallElement("entry");
	this.wallStyles["celt"]["entry"].entity = "structures/celt_defense_tower";
	this.wallStyles["celt"]["entry"].angle = PI;
	this.wallStyles["celt"]["entry"].width = 6;
	this.wallStyles["celt"]["entry"].indent = -5;
	
	this.wallStyles["celt"]["wideEntry"] = new wallElement("wideEntry");
	this.wallStyles["celt"]["wideEntry"].entity = "structures/celt_fortress_b";
	this.wallStyles["celt"]["wideEntry"].angle = PI;
	this.wallStyles["celt"]["wideEntry"].width = 12;
	this.wallStyles["celt"]["wideEntry"].indent = 7;
	
	this.wallStyles["celt"]["endRight"] = new wallElement("endRight");
	this.wallStyles["celt"]["endRight"].entity = "structures/celt_wall_tower";
	this.wallStyles["celt"]["endRight"].angle = PI;
	this.wallStyles["celt"]["endRight"].width = 0.7;
	
	this.wallStyles["celt"]["endLeft"] = new wallElement("endLeft");
	this.wallStyles["celt"]["endLeft"].entity = "structures/celt_wall_tower";
	this.wallStyles["celt"]["endLeft"].angle = PI;
	this.wallStyles["celt"]["endLeft"].width = 0.7;
	
	this.wallStyles["celt"]["cornerIn"] = new wallElement("cornerIn");
	this.wallStyles["celt"]["cornerIn"].entity = "structures/celt_wall_tower";
	this.wallStyles["celt"]["cornerIn"].angle = PI;
	this.wallStyles["celt"]["cornerIn"].width = 0.7;
	this.wallStyles["celt"]["cornerIn"].indent = 0;
	this.wallStyles["celt"]["cornerIn"].bending = PI/2;
	
	this.wallStyles["celt"]["cornerOut"] = new wallElement("cornerOut");
	this.wallStyles["celt"]["cornerOut"].entity = "structures/celt_wall_tower";
	this.wallStyles["celt"]["cornerOut"].angle = PI;
	this.wallStyles["celt"]["cornerOut"].width = 0.7;
	this.wallStyles["celt"]["cornerOut"].indent = 0;
	this.wallStyles["celt"]["cornerOut"].bending = -PI/2;
	
	this.wallStyles["celt"]["barracks"] = new wallElement("barracks");
	this.wallStyles["celt"]["barracks"].entity = "structures/celt_barracks";
	this.wallStyles["celt"]["barracks"].angle = PI;
	this.wallStyles["celt"]["barracks"].indent = 7;
	
	// Wall style 'hele'
	this.wallStyles["hele"] = {};
	this.wallStyles["hele"]["wall"] = new wallElement("wall");
	this.wallStyles["hele"]["wall"].entity = "structures/hele_wall";
	this.wallStyles["hele"]["wall"].angle = 0*PI;
	this.wallStyles["hele"]["wall"].width = 5.95;
	
	this.wallStyles["hele"]["tower"] = new wallElement("tower");
	this.wallStyles["hele"]["tower"].entity = "structures/hele_wall_tower";
	this.wallStyles["hele"]["tower"].angle = PI;
	this.wallStyles["hele"]["tower"].width = 1.5;
	
	this.wallStyles["hele"]["wallFort"] = new wallElement("wallFort");
	this.wallStyles["hele"]["wallFort"].entity = "structures/hele_fortress";
	this.wallStyles["hele"]["wallFort"].angle = 2*PI/2; // PI/2
	this.wallStyles["hele"]["wallFort"].width = 5.1; // 5.6
	this.wallStyles["hele"]["wallFort"].indent = 1.9; // 1.9
	
	this.wallStyles["hele"]["outpost"] = new wallElement("outpost");
	this.wallStyles["hele"]["outpost"].entity = "structures/hele_outpost";
	this.wallStyles["hele"]["outpost"].angle = PI;
	this.wallStyles["hele"]["outpost"].indent = -5;
	
	this.wallStyles["hele"]["house"] = new wallElement("house");
	this.wallStyles["hele"]["house"].entity = "structures/hele_house";
	this.wallStyles["hele"]["house"].angle = PI/2;
	this.wallStyles["hele"]["house"].indent = 6;
	
	this.wallStyles["hele"]["gate"] = new wallElement("gate");
	this.wallStyles["hele"]["gate"].entity = "structures/hele_wall_gate";
	this.wallStyles["hele"]["gate"].angle = 0*PI;
	this.wallStyles["hele"]["gate"].width = 9.1;
	
	this.wallStyles["hele"]["entry"] = new wallElement("entry");
	this.wallStyles["hele"]["entry"].entity = "structures/hele_defense_tower";
	this.wallStyles["hele"]["entry"].angle = PI;
	this.wallStyles["hele"]["entry"].width = 9.1;
	this.wallStyles["hele"]["entry"].indent = -4;
	
	this.wallStyles["hele"]["wideEntry"] = new wallElement("wideEntry");
	this.wallStyles["hele"]["wideEntry"].entity = "structures/hele_fortress";
	this.wallStyles["hele"]["wideEntry"].angle = 5*PI/2;
	this.wallStyles["hele"]["wideEntry"].width = 12;
	this.wallStyles["hele"]["wideEntry"].indent = 7;
	
	this.wallStyles["hele"]["endRight"] = new wallElement("endRight");
	this.wallStyles["hele"]["endRight"].entity = "structures/hele_wall_tower";
	this.wallStyles["hele"]["endRight"].angle = PI;
	this.wallStyles["hele"]["endRight"].width = 1.5;
	
	this.wallStyles["hele"]["endLeft"] = new wallElement("endLeft");
	this.wallStyles["hele"]["endLeft"].entity = "structures/hele_wall_tower";
	this.wallStyles["hele"]["endLeft"].angle = PI;
	this.wallStyles["hele"]["endLeft"].width = 1.5;
	
	this.wallStyles["hele"]["cornerIn"] = new wallElement("cornerIn");
	this.wallStyles["hele"]["cornerIn"].entity = "structures/hele_wall_tower";
	this.wallStyles["hele"]["cornerIn"].angle = 5*PI/4;
	this.wallStyles["hele"]["cornerIn"].width = 0;
	this.wallStyles["hele"]["cornerIn"].indent = 0.5;
	this.wallStyles["hele"]["cornerIn"].bending = PI/2;
	
	this.wallStyles["hele"]["cornerOut"] = new wallElement("cornerOut");
	this.wallStyles["hele"]["cornerOut"].entity = "structures/hele_wall_tower";
	this.wallStyles["hele"]["cornerOut"].angle = 3*PI/4;
	this.wallStyles["hele"]["cornerOut"].width = 1;
	this.wallStyles["hele"]["cornerOut"].bending = -PI/2;
	
	this.wallStyles["hele"]["barracks"] = new wallElement("barracks");
	this.wallStyles["hele"]["barracks"].entity = "structures/hele_barracks";
	this.wallStyles["hele"]["barracks"].angle = PI;
	this.wallStyles["hele"]["barracks"].indent = 6;
	
	// Wall style 'iber'
	this.wallStyles["iber"] = {};
	this.wallStyles["iber"]["wall"] = new wallElement("wall");
	this.wallStyles["iber"]["wall"].entity = "structures/iber_wall";
	this.wallStyles["iber"]["wall"].angle = 0*PI;
	this.wallStyles["iber"]["wall"].width = 6.9;
	
	this.wallStyles["iber"]["tower"] = new wallElement("tower");
	this.wallStyles["iber"]["tower"].entity = "structures/iber_wall_tower";
	this.wallStyles["iber"]["tower"].angle = PI;
	this.wallStyles["iber"]["tower"].width = 2.1;
	
	this.wallStyles["iber"]["wallFort"] = new wallElement("wallFort");
	this.wallStyles["iber"]["wallFort"].entity = "structures/iber_fortress";
	this.wallStyles["iber"]["wallFort"].angle = PI;
	this.wallStyles["iber"]["wallFort"].width = 4.5;
	this.wallStyles["iber"]["wallFort"].indent = 0.7;
	
	this.wallStyles["iber"]["outpost"] = new wallElement("outpost");
	this.wallStyles["iber"]["outpost"].entity = "structures/iber_outpost";
	this.wallStyles["iber"]["outpost"].angle = PI;
	this.wallStyles["iber"]["outpost"].indent = -5;
	
	this.wallStyles["iber"]["house"] = new wallElement("house");
	this.wallStyles["iber"]["house"].entity = "structures/iber_house";
	this.wallStyles["iber"]["house"].angle = PI;
	this.wallStyles["iber"]["house"].indent = 4;
	
	this.wallStyles["iber"]["gate"] = new wallElement("gate");
	this.wallStyles["iber"]["gate"].entity = "structures/iber_wall_gate";
	this.wallStyles["iber"]["gate"].angle = 0*PI;
	this.wallStyles["iber"]["gate"].width = 9.2;
	
	this.wallStyles["iber"]["entry"] = new wallElement("entry");
	this.wallStyles["iber"]["entry"].entity = "structures/iber_wall_tower";
	this.wallStyles["iber"]["entry"].angle = PI;
	this.wallStyles["iber"]["entry"].width = 6.9;
	this.wallStyles["iber"]["entry"].indent = -5;
	
	this.wallStyles["iber"]["wideEntry"] = new wallElement("wideEntry");
	this.wallStyles["iber"]["wideEntry"].entity = "structures/iber_fortress";
	this.wallStyles["iber"]["wideEntry"].angle = PI/2;
	this.wallStyles["iber"]["wideEntry"].width = 9.2;
	this.wallStyles["iber"]["wideEntry"].indent = 7;
	
	this.wallStyles["iber"]["endRight"] = new wallElement("endRight");
	this.wallStyles["iber"]["endRight"].entity = "structures/iber_wall_tower";
	this.wallStyles["iber"]["endRight"].angle = PI;
	this.wallStyles["iber"]["endRight"].width = 2.1;
	
	this.wallStyles["iber"]["endLeft"] = new wallElement("endLeft");
	this.wallStyles["iber"]["endLeft"].entity = "structures/iber_wall_tower";
	this.wallStyles["iber"]["endLeft"].angle = PI;
	this.wallStyles["iber"]["endLeft"].width = 2.1;
	
	this.wallStyles["iber"]["cornerIn"] = new wallElement("cornerIn");
	this.wallStyles["iber"]["cornerIn"].entity = "structures/iber_wall_tower";
	this.wallStyles["iber"]["cornerIn"].angle = PI;
	this.wallStyles["iber"]["cornerIn"].width = 2.1;
	this.wallStyles["iber"]["cornerIn"].bending = PI/2;
	
	this.wallStyles["iber"]["cornerIn2"] = new wallElement("cornerIn2");
	this.wallStyles["iber"]["cornerIn2"].entity = "structures/iber_defense_tower";
	this.wallStyles["iber"]["cornerIn2"].angle = 5*PI/4;
	this.wallStyles["iber"]["cornerIn2"].width = 1;
	this.wallStyles["iber"]["cornerIn2"].indent = 0.2;
	this.wallStyles["iber"]["cornerIn2"].bending = PI/2;
	
	this.wallStyles["iber"]["cornerIn3"] = new wallElement("cornerIn3");
	this.wallStyles["iber"]["cornerIn3"].entity = "structures/iber_wall_tower";
	this.wallStyles["iber"]["cornerIn3"].angle = 5*PI/4;
	this.wallStyles["iber"]["cornerIn3"].width = 0.3;
	this.wallStyles["iber"]["cornerIn3"].indent = 0.5;
	this.wallStyles["iber"]["cornerIn3"].bending = PI/2;
	
	this.wallStyles["iber"]["cornerOut"] = new wallElement("cornerOut");
	this.wallStyles["iber"]["cornerOut"].entity = "structures/iber_wall_tower";
	this.wallStyles["iber"]["cornerOut"].angle = 3*PI/4;
	this.wallStyles["iber"]["cornerOut"].width = 1.3;
	this.wallStyles["iber"]["cornerOut"].indent = 0;
	this.wallStyles["iber"]["cornerOut"].bending = -PI/2;
	
	this.wallStyles["iber"]["barracks"] = new wallElement("barracks");
	this.wallStyles["iber"]["barracks"].entity = "structures/iber_barracks";
	this.wallStyles["iber"]["barracks"].angle = PI;
	this.wallStyles["iber"]["barracks"].indent = 7;
	
	// Wall style 'pers'
	this.wallStyles["pers"] = {};
	this.wallStyles["pers"]["wall"] = new wallElement("wall");
	this.wallStyles["pers"]["wall"].entity = "structures/pers_wall";
	this.wallStyles["pers"]["wall"].angle = 0*PI;
	this.wallStyles["pers"]["wall"].width = 5.9;
	
	this.wallStyles["pers"]["tower"] = new wallElement("tower");
	this.wallStyles["pers"]["tower"].entity = "structures/pers_wall_tower";
	this.wallStyles["pers"]["tower"].angle = PI;
	this.wallStyles["pers"]["tower"].width = 1.7;
	
	this.wallStyles["pers"]["wallFort"] = new wallElement("wallFort");
	this.wallStyles["pers"]["wallFort"].entity = "structures/pers_fortress";
	this.wallStyles["pers"]["wallFort"].angle = PI;
	this.wallStyles["pers"]["wallFort"].width = 5.6; // 5.5
	this.wallStyles["pers"]["wallFort"].indent = 1.9; // 1.7
	
	this.wallStyles["pers"]["outpost"] = new wallElement("outpost");
	this.wallStyles["pers"]["outpost"].entity = "structures/pers_outpost";
	this.wallStyles["pers"]["outpost"].angle = PI;
	this.wallStyles["pers"]["outpost"].indent = -5;
	
	this.wallStyles["pers"]["house"] = new wallElement("house");
	this.wallStyles["pers"]["house"].entity = "structures/pers_house";
	this.wallStyles["pers"]["house"].angle = PI;
	this.wallStyles["pers"]["house"].indent = 6;
	
	this.wallStyles["pers"]["gate"] = new wallElement("gate");
	this.wallStyles["pers"]["gate"].entity = "structures/pers_wall_gate";
	this.wallStyles["pers"]["gate"].angle = 0*PI;
	this.wallStyles["pers"]["gate"].width = 6;
	
	this.wallStyles["pers"]["entry"] = new wallElement("entry");
	this.wallStyles["pers"]["entry"].entity = "structures/pers_defense_tower";
	this.wallStyles["pers"]["entry"].angle = PI;
	this.wallStyles["pers"]["entry"].width = 6;
	this.wallStyles["pers"]["entry"].indent = -4;
	
	this.wallStyles["pers"]["wideEntry"] = new wallElement("wideEntry");
	this.wallStyles["pers"]["wideEntry"].entity = "structures/pers_fortress";
	this.wallStyles["pers"]["wideEntry"].angle = PI;
	this.wallStyles["pers"]["wideEntry"].width = 12;
	this.wallStyles["pers"]["wideEntry"].indent = 7;
	
	this.wallStyles["pers"]["endRight"] = new wallElement("endRight");
	this.wallStyles["pers"]["endRight"].entity = "structures/pers_wall_tower";
	this.wallStyles["pers"]["endRight"].angle = PI;
	this.wallStyles["pers"]["endRight"].width = 1.7;
	
	this.wallStyles["pers"]["endLeft"] = new wallElement("endLeft");
	this.wallStyles["pers"]["endLeft"].entity = "structures/pers_wall_tower";
	this.wallStyles["pers"]["endLeft"].angle = PI;
	this.wallStyles["pers"]["endLeft"].width = 1.7;
	
	this.wallStyles["pers"]["cornerIn"] = new wallElement("cornerIn");
	this.wallStyles["pers"]["cornerIn"].entity = "structures/pers_wall_tower";
	this.wallStyles["pers"]["cornerIn"].angle = 5*PI/4;
	this.wallStyles["pers"]["cornerIn"].width = 0.2;
	this.wallStyles["pers"]["cornerIn"].indent = 0.5;
	this.wallStyles["pers"]["cornerIn"].bending = PI/2;
	
	this.wallStyles["pers"]["cornerOut"] = new wallElement("cornerOut");
	this.wallStyles["pers"]["cornerOut"].entity = "structures/pers_wall_tower";
	this.wallStyles["pers"]["cornerOut"].angle = 3*PI/4;
	this.wallStyles["pers"]["cornerOut"].width = 0.8;
	this.wallStyles["pers"]["cornerOut"].indent = 0;
	this.wallStyles["pers"]["cornerOut"].bending = -PI/2;
	
	this.wallStyles["pers"]["barracks"] = new wallElement("barracks");
	this.wallStyles["pers"]["barracks"].entity = "structures/pers_barracks";
	this.wallStyles["pers"]["barracks"].angle = PI;
	this.wallStyles["pers"]["barracks"].indent = 7;
	
	// Wall style 'rome'
	this.wallStyles["rome"] = {};
	this.wallStyles["rome"]["wall"] = new wallElement("wall");
	this.wallStyles["rome"]["wall"].entity = "structures/rome_wall";
	this.wallStyles["rome"]["wall"].angle = 0*PI;
	this.wallStyles["rome"]["wall"].width = 5.9;
	
	this.wallStyles["rome"]["tower"] = new wallElement("tower");
	this.wallStyles["rome"]["tower"].entity = "structures/rome_wall_tower";
	this.wallStyles["rome"]["tower"].angle = PI;
	this.wallStyles["rome"]["tower"].width = 2.1;
	
	this.wallStyles["rome"]["wallFort"] = new wallElement("wallFort");
	this.wallStyles["rome"]["wallFort"].entity = "structures/rome_fortress";
	this.wallStyles["rome"]["wallFort"].angle = PI;
	this.wallStyles["rome"]["wallFort"].width = 6.3;
	this.wallStyles["rome"]["wallFort"].indent = 2.1;
	
	this.wallStyles["rome"]["outpost"] = new wallElement("outpost");
	this.wallStyles["rome"]["outpost"].entity = "structures/rome_outpost";
	this.wallStyles["rome"]["outpost"].angle = PI;
	this.wallStyles["rome"]["outpost"].indent = -5;
	
	this.wallStyles["rome"]["house"] = new wallElement("house");
	this.wallStyles["rome"]["house"].entity = "structures/rome_house";
	this.wallStyles["rome"]["house"].angle = PI;
	this.wallStyles["rome"]["house"].indent = 7;
	
	this.wallStyles["rome"]["gate"] = new wallElement("gate");
	this.wallStyles["rome"]["gate"].entity = "structures/rome_wall_gate";
	this.wallStyles["rome"]["gate"].angle = 0*PI;
	this.wallStyles["rome"]["gate"].width = 5.9;
	
	this.wallStyles["rome"]["entry"] = new wallElement("entry");
	this.wallStyles["rome"]["entry"].entity = "structures/rome_defense_tower";
	this.wallStyles["rome"]["entry"].angle = PI;
	this.wallStyles["rome"]["entry"].width = 5.9;
	this.wallStyles["rome"]["entry"].indent = -4;
	
	this.wallStyles["rome"]["wideEntry"] = new wallElement("wideEntry");
	this.wallStyles["rome"]["wideEntry"].entity = "structures/rome_fortress";
	this.wallStyles["rome"]["wideEntry"].angle = PI;
	this.wallStyles["rome"]["wideEntry"].width = 12;
	this.wallStyles["rome"]["wideEntry"].indent = 7;
	
	this.wallStyles["rome"]["endRight"] = new wallElement("endRight");
	this.wallStyles["rome"]["endRight"].entity = "structures/rome_wall_tower";
	this.wallStyles["rome"]["endRight"].angle = PI;
	this.wallStyles["rome"]["endRight"].width = 2.1;
	
	this.wallStyles["rome"]["endLeft"] = new wallElement("endLeft");
	this.wallStyles["rome"]["endLeft"].entity = "structures/rome_wall_tower";
	this.wallStyles["rome"]["endLeft"].angle = PI;
	this.wallStyles["rome"]["endLeft"].width = 2.1;
	
	this.wallStyles["rome"]["cornerIn"] = new wallElement("cornerIn");
	this.wallStyles["rome"]["cornerIn"].entity = "structures/rome_wall_tower";
	this.wallStyles["rome"]["cornerIn"].angle = 5*PI/4;
	this.wallStyles["rome"]["cornerIn"].width = 0;
	this.wallStyles["rome"]["cornerIn"].indent = 0.7;
	this.wallStyles["rome"]["cornerIn"].bending = PI/2;
	
	this.wallStyles["rome"]["cornerOut"] = new wallElement("cornerOut");
	this.wallStyles["rome"]["cornerOut"].entity = "structures/rome_wall_tower";
	this.wallStyles["rome"]["cornerOut"].angle = 3*PI/4;
	this.wallStyles["rome"]["cornerOut"].width = 1.1; // 1.4
	this.wallStyles["rome"]["cornerOut"].indent = 0;
	this.wallStyles["rome"]["cornerOut"].bending = -PI/2;
	
	this.wallStyles["rome"]["barracks"] = new wallElement("barracks");
	this.wallStyles["rome"]["barracks"].entity = "structures/rome_barracks";
	this.wallStyles["rome"]["barracks"].angle = PI;
	this.wallStyles["rome"]["barracks"].indent = 6;
	
	// Setup some default fortress types
	// General default fortress types
	this.fortressTypes["tiny"] = new fortress("tiny");
	var wallPart = ['entry', 'endLeft', 'wall', 'cornerIn', 'wall', 'endRight'];
	var parts = 4;
	for (var part = 0; part < parts; part++)
	{
		for (var wallIndex = 0; wallIndex < wallPart.length; wallIndex++)
			this.fortressTypes["tiny"].wall.push(wallPart[wallIndex])
	};
	
	this.fortressTypes["small"] = new fortress("small");
	var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall',
		'cornerIn', 'wall', 'tower', 'wall', 'endRight'];
	var parts = 4;
	for (var part = 0; part < parts; part++)
	{
		for (var wallIndex = 0; wallIndex < wallPart.length; wallIndex++)
			this.fortressTypes["small"].wall.push(wallPart[wallIndex])
	};
	
	this.fortressTypes["medium"] = new fortress("medium");
	var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall', 'cornerIn', 'wall',
		'cornerOut', 'wall', 'cornerIn', 'wall', 'tower', 'wall', 'endRight'];
	var parts = 4;
	for (var part = 0; part < parts; part++)
	{
		for (var wallIndex = 0; wallIndex < wallPart.length; wallIndex++)
			this.fortressTypes["medium"].wall.push(wallPart[wallIndex])
	};
	
	this.fortressTypes["normal"] = new fortress("normal");
	var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall', 'outpost', 'wall', 'cornerIn', 'wall',
		'cornerOut', 'wall', 'cornerIn', 'wall', 'outpost', 'wall', 'tower', 'wall', 'endRight'];
	var parts = 4;
	for (var part = 0; part < parts; part++)
	{
		for (var wallIndex = 0; wallIndex < wallPart.length; wallIndex++)
			this.fortressTypes["normal"].wall.push(wallPart[wallIndex])
	};
	
	this.fortressTypes["large"] = new fortress("large");
	var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall', 'outpost', 'wall', 'cornerIn', 'wall', 'tower', 'wall',
		'cornerOut', 'wall', 'tower', 'wall', 'cornerIn', 'wall', 'outpost', 'wall', 'tower', 'wall', 'endRight'];
	var parts = 4;
	for (var part = 0; part < parts; part++)
	{
		for (var wallIndex = 0; wallIndex < wallPart.length; wallIndex++)
			this.fortressTypes["large"].wall.push(wallPart[wallIndex])
	};
	
	this.fortressTypes["very large"] = new fortress("very large");
	var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall', 'outpost', 'wall',
		'tower', 'gate', 'tower', 'wall', 'cornerIn', 'wall', 'tower', 'wall',
		'cornerOut', 'wall', 'tower', 'wall', 'cornerIn', 'wall', 'tower', 'gate',
		'tower', 'wall', 'outpost', 'wall', 'tower', 'wall', 'endRight'];
	var parts = 4;
	for (var part = 0; part < parts; part++)
	{
		for (var wallIndex = 0; wallIndex < wallPart.length; wallIndex++)
			this.fortressTypes["very large"].wall.push(wallPart[wallIndex])
	};
	
	this.fortressTypes["giant"] = new fortress("giant");
	var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall', 'outpost', 'wall', 'tower',
		'gate', 'tower', 'wall', 'cornerIn', 'wall', 'outpost', 'wall', 'tower', 'wall',
		'cornerOut', 'wall', 'tower', 'wall', 'outpost', 'wall', 'cornerIn', 'wall', 'tower',
		'gate', 'tower', 'wall', 'outpost', 'wall', 'tower', 'wall', 'endRight'];
	var parts = 4;
	for (var part = 0; part < parts; part++)
	{
		for (var wallIndex = 0; wallIndex < wallPart.length; wallIndex++)
			this.fortressTypes["giant"].wall.push(wallPart[wallIndex])
	};
	
	this.fortressTypes["demo"] = new fortress("demo");
	var wallPart = ['entry', 'endLeft', 'wall', 'tower', 'wall', 'wallFort', 'wall', 'tower', 'gate', 'tower', 'wall', 'outpost',
		'wall', 'cornerIn', 'wall', 'outpost', 'wall', 'endRight', 'wideEntry', 'endLeft', 'wall', 'tower', 'wall',
		'cornerOut', 'wall', 'tower', 'wall', 'endRight', 'wideEntry', 'endLeft', 'wall', 'outpost', 'wall', 'cornerIn',
		'wall', 'outpost', 'wall', 'tower', 'gate', 'tower', 'wall', 'wallFort', 'wall', 'tower', 'wall', 'endRight'];
	var parts = 4;
	for (var part = 0; part < parts; part++)
	{
		for (var wallIndex = 0; wallIndex < wallPart.length; wallIndex++)
			this.fortressTypes["demo"].wall.push(wallPart[wallIndex])
	};
	
	// Default civ dependent fortresses for Spahbod
	this.fortressTypes["cartSB"] = new fortress("cartSB");
	this.fortressTypes["cartSB"].wall = ['entry', 'wall', 'wall',
		'cornerIn', 'wall', 'barracks', 'wall', 'gate', 'wall', 'wall',
		'cornerIn', 'wall', 'house', 'wall', 'entry', 'wall', 'wall',
		'cornerIn', 'wall', 'house', 'wall', 'gate', 'wall', 'wall',
		'cornerIn', 'wall', 'house', 'wall'];
	this.fortressTypes["celtSB"] = new fortress("celtSB");
	this.fortressTypes["celtSB"].wall = ['entry', 'wall', 'wall',
		'cornerIn', 'wall', 'barracks', 'wall', 'gate', 'wall', 'house', 'wall',
		'cornerIn', 'wall', 'house', 'wall', 'entry', 'wall', 'house', 'wall',
		'cornerIn', 'wall', 'house', 'wall', 'gate', 'wall', 'house', 'wall',
		'cornerIn', 'wall', 'house', 'wall'];
	this.fortressTypes["iberSB"] = this.fortressTypes["celtSB"];
	this.fortressTypes["heleSB"] = this.fortressTypes["cartSB"];
	this.fortressTypes["persSB"] = this.fortressTypes["cartSB"];
	this.fortressTypes["romeSB"] = this.fortressTypes["cartSB"];
	
	// Setup style
	this.setStyle(this.style);
	
	// Setup fortress type if preasent
	if (this.fortress !== false && this.fortress !== undefined)
		this.setFortressType(this.fortress);
};

wallTool.prototype.setStyle = function(style)
{
	if (style in this.wallStyles)
	{
		this.style = style;
		this.wallTypes = this.wallStyles[style];
	}
	else
	{
		warn("Wall style '" + style + "' not found, falling back to 'palisades'...");
		this.style = 'palisades';
		this.wallTypes = this.wallStyles['palisades'];
	};
};

wallTool.prototype.getAlignment = function(startX, startZ, orientation)
{
	if (startX == undefined)
		startX = 0;
	if (startZ == undefined)
		startZ = 0;
	if (orientation == undefined)
		orientation = 0;
	var wallAlignment = [];
	var outside = orientation;
	var wallX = startX - this.center[1]*sin(orientation) - this.center[0]*cos(orientation);
	var wallZ = startZ - this.center[1]*cos(orientation) + this.center[0]*sin(orientation);
	for (var iWall = 0; iWall < this.wall.length; iWall++)
	{
		var element = this.wallTypes[this.wall[iWall]];
		var x = wallX - element.indent*sin(outside);
		var z = wallZ - element.indent*cos(outside);
		wallAlignment.push([x, z, element.entity, outside + element.angle]);
		// Presets for the next element
		if (iWall + 1 < this.wall.length)
		{
			outside += element.bending;
			var nextElement = this.wallTypes[this.wall[iWall + 1]];
			var distance = (element.width + nextElement.width)/2;
			// Indent corrections for corners
			if (element.bending !== 0 && element.indent !== 0)
			{
				// Indent correction of distance, not sure for non-right angles...
				distance += element.indent*sin(element.bending);
				// Indent correction of next normalized indentation
				wallX += element.indent * sin(outside);
				wallZ += element.indent * cos(outside);
			};
			wallX += distance * sin(outside + PI/2);
			wallZ += distance * cos(outside + PI/2);
		};
	};
	return wallAlignment
};

wallTool.prototype.getCenter = function(wallAlignment)
{
	var x = 0;
	var z = 0;
	for (var iWall = 0; iWall < wallAlignment.length; iWall++)
	{
		x += wallAlignment[iWall][0]/wallAlignment.length;
		z += wallAlignment[iWall][1]/wallAlignment.length;
	};
	var output = [x, z];
	return output;
};

wallTool.prototype.setFortress = function(fortress)
{
	this.fortress = fortress.type;
	this.wall = fortress.wall;
	if (this.calculateCenter == true)
	{
		this.center = [0, 0];
		var AM = this.getAlignment(0, 0, 0*PI);
		var center = this.getCenter(AM);
		this.center = center;
	}
	else
	{
		this.center = this.fortress.center
	};
};

wallTool.prototype.setFortressType = function(type)
{
	if (type in this.fortressTypes)
	{
		this.setFortress(this.fortressTypes[type]);
	}
	else
	{
		warn("Fortress type '" + type + "' not found, falling back to 'medium'");
		this.setFortress(this.fortressTypes["medium"]);
	};
};

wallTool.prototype.place = function(startX, startZ, playerId, orientation)
{
	var AM = this.getAlignment(startX, startZ, orientation);
	for (var iWall = 0; iWall < this.wall.length; iWall++)
	{
		if (AM[iWall][2] !== undefined)
			placeObject(AM[iWall][0], AM[iWall][1], AM[iWall][2], playerId, AM[iWall][3]);
	};
};
