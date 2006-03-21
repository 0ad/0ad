/*
	DESCRIPTION	: Functions that support the coordinate system (ability to maintain and swap between multiple
			  sizes for an object).
	NOTES		: 
*/

// ====================================================================

function initCoord()
{
	// Initialise coordinate set for this page.
	Crd = new Array();
	Crd.last = new Object(-1);

	// Set corner constants.
	rb	= 0;
	lb	= 1;
	lt	= 2;
	rt	= 3;
}

// ====================================================================

// Add a new size coordinate to the array.
function addCrd (name, group, rx, ry, x, y, width, height, rx2, ry2)
{
	// If no group is specified, default to the first one.
	if (!group)
		group = rb;

	// Create new coordinate if necessary (it doesn't exist, or we have a new name).
	if (group == rb || !Crd[Crd.last])
	{
		Crd.last++;
		Crd[Crd.last]				= new Object();
		Crd[Crd.last].name			= new Object(name);
		Crd[Crd.last].size			= new Object();
		Crd[Crd.last].coord			= new Object();
	}
	
	// Get coordinates from first element if not defined.
	if (x == undefined) 	
		x 	= Crd[Crd.last].coord[rb].x;
	if (y == undefined) 		
		y 	= Crd[Crd.last].coord[rb].y;
	if (width == undefined)		
		width 	= Crd[Crd.last].coord[rb].width;
	if (height == undefined) 	
		height 	= Crd[Crd.last].coord[rb].height;
		
	// Generate and save coordinates.
	Crd[Crd.last].size[group]		= calcCrdArray (rx, ry, x, y, width, height, rx2, ry2);
	Crd[Crd.last].coord[group]		= new Array();
	Crd[Crd.last].coord[group].rx		= rx;
	Crd[Crd.last].coord[group].ry		= ry;
	Crd[Crd.last].coord[group].x		= x;
	Crd[Crd.last].coord[group].y		= y;
	Crd[Crd.last].coord[group].width	= width;
	Crd[Crd.last].coord[group].height	= height;
}

// ====================================================================

function addCrds (name, rx, ry, x, y, width, height, rx2, ry2)
{
	// This is a shortcut function that saves you having to call addCrd() for each coordinate group. Just do this once for the whole set.
	// It creates the first, then assumes all the remaining coordinates are flipped to the opposite screen edge.
	// (True in 90% of cases.)
	
	addCrd (name, rb, rx, ry, x, y, width, height, rx2, ry2);
	
	// Determine flip relatives.
	switch (rx)
	{
		case 0:
			var rx1 = 100;
			var rx2 = 100;			
			var rx3 = 0;			
		break;
		case 50:
			var rx1 = 50;
			var rx2 = 50;
			var rx3 = 50;
		break;
		case 100:
			var rx1 = 0;
			var rx2 = 0;			
			var rx3 = 100;		
		break;
	}
	switch (ry)
	{
		case 0:
			var ry1 = 0;
			var ry2 = 100;
			var ry3 = 100;
		break;
		case 50:
			var ry1 = 50;
			var ry2 = 50;
			var ry3 = 50;
		break;		
		case 100:
			var ry1 = 100;
			var ry2 = 0;
			var ry3 = 0;			
		break;
	}	
	
	// Define the rest of the coordinate set.
	addCrd (name, lb, rx1, ry1);
	addCrd (name, lt, rx2, ry2);
	addCrd (name, rt, rx3, ry3);
} 

// ====================================================================

// Return coordinate object with a given name.
// Optionally can choose to return the index to the coordinate, rather than the coordinate itself.
function getCrd (name, byIndex)
{
	for (var getCrdLoop = 0; getCrdLoop <= Crd.last; getCrdLoop++)
	{
		if (Crd[getCrdLoop].name == name)
		{	
			// If only index requested, just return index.
			if (byIndex)
				return getCrdLoop;
			else
			// Otherwise return the whole coordinate array for this object.
				return Crd[getCrdLoop];
		}
	}

	console.write ("Coordinate " + name + " not found in call to getCrd().");
	// Return -1 to indicate failure.
	return -1;
}

// ====================================================================

// Calculates and returns "size" string from a given x/y/width/height and relative x and y. 
function calcCrdArray (rx, ry, x, y, width, height, rx2, ry2)
{
	if (!rx2 && !ry2)
	{
		// If two sets of relatives are not specified at the end of the parameter list, use the "shortcut" format:
		//	"Left Corner" (Relative X)
		//	"Top Corner" (Relative Y)
		//	X Position
		//	Y Position
		//	Width from X
		//	Height from Y	
	
		var setSizeContainer = new Object();

		setSizeContainer.rleft = rx;
		setSizeContainer.rtop = ry;
		setSizeContainer.rright = rx;
		setSizeContainer.rbottom = ry;
			
		// Define size dimensions.
		switch (setSizeContainer.rleft)
		{
			case 0:
			case 50:
				setSizeContainer.x1 = x;
			break;
			case 100:
				setSizeContainer.x1 = -x-width;
			break;
		}
		switch (setSizeContainer.rtop)
		{
			case 0:
			case 50:
				setSizeContainer.y1 = y;
			break;
			case 100:
				setSizeContainer.y1 = -y-height;
			break;
		}
		switch (setSizeContainer.rright)
		{
			case 0:
			case 50:
				setSizeContainer.x2 = x+width;
			break;
			case 100:
				setSizeContainer.x2 = -x;
			break;
		}
		switch (setSizeContainer.rbottom)
		{
			case 0:
			case 50:
				setSizeContainer.y2 = y+height;
			break;
			case 100:
				setSizeContainer.y2 = -y;
			break;
		}

		return	new GUISize (setSizeContainer.x1, setSizeContainer.y1, setSizeContainer.x2, setSizeContainer.y2,
				setSizeContainer.rleft, setSizeContainer.rtop, setSizeContainer.rright, setSizeContainer.rbottom);
	}
	else
	{
		// Use the standard "size" structure:
		//	RX
		//	RY
		//	X1
		//	X2
		//	Y1
		//	Y2
		//	RX2
		//	RY2
	
		return 	new GUISize (x, y, width, height, rx, ry, rx2, ry2);	
		
	}
	
	
	
}

// ====================================================================

// Set an existing coord of a given name to a new value.
function setCrd (name, newCrd)
{
	// Get the index to the given coordinate.
	var crdResult = getCrd (name, true);
	if (crdResult != -1)
	{
		// Set new value of this coordinate.
		Crd[crdResult] = newCrd;
	}
	else
	{
		console.write ("Failed to setCrd() + " + name);
		return -1;
	}
}

// ====================================================================

function setSkin (skinName)
{
//	(Unfortunately, this idea just isn't going to work until the GUI Engine allows us to set the value of a control's style (and therefore refresh its properties
//	with those of the new style. As it stands, the style property is empty at runtime because styles are all set when the stuff is loaded. And since we're wrapping
//	sprite info for skins in the styles, it won't be adequate to simply change the sprite properties. So commenting this out for now so we can adapt it later.)

/*
	// Sets the sprites of all registered controls with a skin (eg *<skinname>*SpriteName) to the specified skin name.
	// Obviously such sprite names must exist, or it all will go horribly wrong.
	
	// Seek through all registered coordinates.
	for (var i = 0; i <= Crd.last; i++)
	{
		var objectName = getGUIObjectByName (Crd[i].name);
		
		// We need to deliberately ignore the minimap object as it has no sprite (and checking for the existence of the property or type doesn't work).
		if (objectName.name != "snMiniMapDisplay")
		{
			// Get first asterisk position.
			var startIndex = objectName.style.indexOf ("*", 1);
			console.write (startIndex);
			// If the coordinate begins with a "skin*" (and therefore qualifies for replacement),
			if (startIndex != -1)
			{
				// Look for and return the ending asterisk.
				var endIndex = objectName.style.indexOf ("*", 5);
				console.write (endIndex);				
				if (endIndex != -1)
				{
					// Set new skin name for sprite.
					objectName.style = "skin*" + skinName + "*" + objectName.style.substring (endIndex);
				}
			}
			
		}
	}	
*/
}

// ====================================================================