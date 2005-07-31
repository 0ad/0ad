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
function addCrd (name, group, rx, ry, x, y, width, height)
{
	// Create new coordinate if necessary (it doesn't exist, or we have a new name).
	if (!Crd[Crd.last])
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
	Crd[Crd.last].size[group]		= calcCrdArray (rx, ry, x, y, width, height);
	Crd[Crd.last].coord[group]		= new Array();
	Crd[Crd.last].coord[group].rx		= rx;
	Crd[Crd.last].coord[group].ry		= ry;
	Crd[Crd.last].coord[group].x		= x;
	Crd[Crd.last].coord[group].y		= y;
	Crd[Crd.last].coord[group].width	= width;
	Crd[Crd.last].coord[group].height	= height;
}

// ====================================================================

// Calculates and returns "size" string from a given x/y/width/height and relative x and y. 
function calcCrdArray (rx, ry, x, y, width, height)
{
	setSizeContainer = new Object();

	// Set relatives.
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

// ====================================================================
