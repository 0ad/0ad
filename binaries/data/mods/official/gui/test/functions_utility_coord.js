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
// console.write ("Created " + Crd.last + Crd[Crd.last].name);
	}
	
	// Create new size.
	Crd[Crd.last].size[group]		= new Array();
/*
	// Get coordinates from first element if not defined.
	if (!x) 		x 	= Crd[Crd.last].size[rb].size.x1;
	if (!y) 		y 	= Crd[Crd.last].size[rb].size.y1;
	if (!width) 		width 	= Crd[Crd.last].size[rb].size.x2-Crd[Crd.last].size[rb].size.x1;
	if (!height)	 	height 	= Crd[Crd.last].size[rb].size.y2-Crd[Crd.last].size[rb].size.y1;
*/
	Crd[Crd.last].size[group] 	= calcCrdArray (rx, ry, x, y, width, height);
// console.write (Crd[Crd.last].size[group]);
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
			setSizeContainer.x1 = x;
		break;
		case 100:
			setSizeContainer.x1 = -x-width;
		break;
		case 50:
			setSizeContainer.x1 = x;
		break;
	}
	switch (setSizeContainer.rtop)
	{
		case 0:
			setSizeContainer.y1 = y;
		break;
		case 100:
			setSizeContainer.y1 = -y-height;
		break;
		case 50:
			setSizeContainer.y1 = y;
		break;
	}
	switch (setSizeContainer.rright)
	{
		case 0:
			setSizeContainer.x2 = x+width;
		break;
		case 100:
			if (setSizeContainer.rleft == 100)
				setSizeContainer.x2 = -x;
			else
				setSizeContainer.x2 = -width;				
		break;
		case 50:
			setSizeContainer.x2 = x+width;
		break;
	}
	switch (setSizeContainer.rbottom)
	{
		case 0:
			setSizeContainer.y2 = y+height;
		break;
		case 100:
			if (setSizeContainer.rtop == 100)
				setSizeContainer.y2 = -y;
			else
				setSizeContainer.y2 = -height;
		break;
		case 50:
			setSizeContainer.y2 = objectArrayElement.y+objectArrayElement.height;
		break;
	}

	return new GUISize(setSizeContainer.x1, setSizeContainer.y1, setSizeContainer.x2, setSizeContainer.y2, setSizeContainer.rleft, setSizeContainer.rtop, setSizeContainer.rright, setSizeContainer.rbottom);
}

// ====================================================================
