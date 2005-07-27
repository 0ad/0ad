/*
	DESCRIPTION	: Functions that support the coordinate system (ability to maintain and swap between multiple
			  sizes for an object).
	NOTES		: 
*/

//// THIS WILL BE REWRITTEN SHORTLY - AGAIN! ////

// ====================================================================

function initCoord()
{
	// Initialise coordinate set for this page.
	Crd = new Array();
	Crd.last = 0;

	// Screen percentages.
	top_screen = 0;
	left_screen = 0;
	mid_screen = 50;
	bottom_screen = 100;
	right_screen = 100;

	// Coord-style size table.
	SizeCoord = new Array();
	SizeCoord.last = 0;
}

// ====================================================================

function AddSizeCoord(objectName, left1, top1, right1, bottom1, rleft1, rtop1, rright1, rbottom1, left2, top2, right2, bottom2, rleft2, rtop2, rright2, rbottom2)
{
	// Used to store the two GUI style sizes for an object on creation (specified as coordinates).
	// Used later by FlipGUI() to switch the objects to a new set of positions.

	SizeCoord[SizeCoord.last] = new Object();
	SizeCoord[SizeCoord.last].name = objectName;
	SizeCoord[SizeCoord.last].size1 = new GUISize(left1, top1, right1, bottom1, rleft1, rtop1, rright1, rbottom1);
	SizeCoord[SizeCoord.last].size2 = new GUISize(left2, top2, right2, bottom2, rleft2, rtop2, rright2, rbottom2);

	SizeCoord.last++; // Increment counter for next entry.
}

// ====================================================================

function AddSizeString(objectName, size1, size2)
{
	// Used to store the two GUI style sizes for an object on creation (specified as strings).
	// Used later by FlipGUI() to switch the objects to a new set of positions.

	SizeCoord[SizeCoord.last] = new Object();
	SizeCoord[SizeCoord.last].name = objectName;
	SizeCoord[SizeCoord.last].size1 = size1;
	SizeCoord[SizeCoord.last].size2 = size2;

	SizeCoord.last++; // Increment counter for next entry.
}

// ====================================================================

function setSize(objectName, sizeString)
{
	// Use this function as a shortcut to change the size of a GUI control, specifying a size string. 

        getGUIObjectByName(objectName).size = sizeString;
}

// ====================================================================

function setSizeCoord(objectName, left, top, right, bottom, rleft, rtop, rright, rbottom)
{
	// Use this function as a shortcut to change the size of a GUI control, specifying individual coordinates. 

	getGUIObjectByName(objectName).size = new GUISize(left, top, right, bottom, rleft, rtop, rright, rbottom);
} 

// ====================================================================

function addSizeArray(objectArray, objectElement, objectX, objectY, objectWidth, objectHeight)
{
	// Adds a x/y/width/height element to a given coordinate array.

	objectArray[objectElement] = new Object();
	objectArray[objectElement].width = objectWidth;
	objectArray[objectElement].height = objectHeight;
	objectArray[objectElement].x = objectX;
	objectArray[objectElement].y = objectY;

	return objectElement;
}

// ====================================================================

function addSizeArrayWH(objectArray, objectElement, objectWidth, objectHeight)
{
	// Creates and adds a width/height element to a given coordinate array.

	objectArray[objectElement] = new Object();
	objectArray[objectElement].width = objectWidth;
	objectArray[objectElement].height = objectHeight;

	return objectElement;
}

// ====================================================================

function addSizeArrayXY(objectArray, objectElement, objectX, objectY)
{
	// Adds a x/y element to a given coordinate array and increments.

	objectArray[objectElement].x = objectX;
	objectArray[objectElement].y = objectY;

	objectElement++;
	return objectElement;
}

// ====================================================================

function calcSizeArray(objectName, objectArrayElement, rleft, rtop, rright, rbottom)
{
	// Use this function as a shortcut to change the size of a GUI control, via an array containing x, y, width, height and 4 relative sizes. 
	// Alternatively, specify 4 relative sizes to override and use these ones instead.

	setSizeContainer = new Object();

	// Override with relative parameter if given.
	if (rleft && rtop && rright && rbottom)
	{
		objectArrayElement.rleft = rleft;
		objectArrayElement.rtop = rtop;
		objectArrayElement.rright = rright;
		objectArrayElement.rbottom = rbottom;
	}

	setSizeContainer.rleft = objectArrayElement.rleft;
	setSizeContainer.rtop = objectArrayElement.rtop;
	setSizeContainer.rright = objectArrayElement.rright;
	setSizeContainer.rbottom = objectArrayElement.rbottom;

	// Define size dimensions.
	switch (setSizeContainer.rleft)
	{
		case left_screen:
			setSizeContainer.x1 = objectArrayElement.x;
		break;
		case right_screen:
			setSizeContainer.x1 = -objectArrayElement.x-objectArrayElement.width;
		break;
		case mid_screen:
			setSizeContainer.x1 = objectArrayElement.x;
		break;
	}
	switch (setSizeContainer.rtop)
	{
		case top_screen:
			setSizeContainer.y1 = objectArrayElement.y;
		break;
		case bottom_screen:
			setSizeContainer.y1 = -objectArrayElement.y-objectArrayElement.height;
		break;
		case mid_screen:
			setSizeContainer.y1 = objectArrayElement.y;
		break;
	}
	switch (setSizeContainer.rright)
	{
		case left_screen:
			setSizeContainer.x2 = objectArrayElement.x+objectArrayElement.width;
		break;
		case right_screen:
			if (setSizeContainer.rleft == right_screen)
				setSizeContainer.x2 = -objectArrayElement.x;
			else
				setSizeContainer.x2 = -objectArrayElement.width;				
		break;
		case mid_screen:
			setSizeContainer.x2 = objectArrayElement.x+objectArrayElement.width;
		break;
	}
	switch (setSizeContainer.rbottom)
	{
		case top_screen:
			setSizeContainer.y2 = objectArrayElement.y+objectArrayElement.height;
		break;
		case bottom_screen:
			if (setSizeContainer.rtop == bottom_screen)
				setSizeContainer.y2 = -objectArrayElement.y;
			else
				setSizeContainer.y2 = -objectArrayElement.height;
		break;
		case mid_screen:
			setSizeContainer.y2 = objectArrayElement.y+objectArrayElement.height;
		break;
	}

	return new GUISize(setSizeContainer.x1, setSizeContainer.y1, setSizeContainer.x2, setSizeContainer.y2, setSizeContainer.rleft, setSizeContainer.rtop, setSizeContainer.rright, setSizeContainer.rbottom);
}

// ====================================================================

function setSizeArray(objectName, objectArrayElement, rleft, rtop, rright, rbottom)
{
	// Set appropriate size for dimensions.
	getGUIObjectByName(objectName).size = calcSizeArray(objectName, objectArrayElement, rleft, rtop, rright, rbottom);
} 

// ====================================================================

function addSizeCoordArray(objectName, objectArrayElement, objectArrayElement_Flp)
{
	// Used to store the two GUI style sizes for an object on creation (specified as coordinates).
	// Used later by FlipGUI() to switch the objects to a new set of positions.
	// This uses the revised coordinate system, so it handles the flipping internally.
	// If an optional secondary coordinate object is specified by parameter, it will
	// use the values in this object for the flipped version, rather than automatically
	// generating a top-down flip.

	SizeCoord[SizeCoord.last] = new Object();
	SizeCoord[SizeCoord.last].name = objectName;

	// Determine relatives for the second size.
	addSizeContainer = new Object();
	if (objectArrayElement_Flp) // Specify new flipped coordinates from object.
	{
		addSizeContainer.rleft = objectArrayElement_Flp.rleft;
		addSizeContainer.rtop = objectArrayElement_Flp.rtop;
		addSizeContainer.rright = objectArrayElement_Flp.rright;
		addSizeContainer.rbottom = objectArrayElement_Flp.rbottom;
		addSizeContainer.width = objectArrayElement_Flp.width;
		addSizeContainer.height = objectArrayElement_Flp.height;
		addSizeContainer.x = objectArrayElement_Flp.x;
		addSizeContainer.y = objectArrayElement_Flp.y;
	}
	else // Generate flipped version automatically.
	{
		addSizeContainer.rleft = objectArrayElement.rleft;
		addSizeContainer.rtop = 100-objectArrayElement.rtop;
		addSizeContainer.rright = objectArrayElement.rright;
		addSizeContainer.rbottom = 100-objectArrayElement.rbottom;

		if (addSizeContainer.rtop <= 0) addSizeContainer.rtop = 0;
		if (addSizeContainer.rbottom <= 0) addSizeContainer.rbottom = 0;
		if (addSizeContainer.rtop >= 100) addSizeContainer.rtop = 100;
		if (addSizeContainer.rbottom >= 100) addSizeContainer.rbottom = 100;

		addSizeContainer.x = objectArrayElement.x;
		addSizeContainer.y = objectArrayElement.y;
		addSizeContainer.width = objectArrayElement.width;
		addSizeContainer.height = objectArrayElement.height;

		if (addSizeContainer.rtop == 100 && objectArrayElement.y < 0) addSizeContainer.y = objectArrayElement.y*-1;
		if (addSizeContainer.rbottom == 100 && objectArrayElement.y < 0) addSizeContainer.y = objectArrayElement.y*-1;
	}

	// Assign the two sizes to array for retrieval when needed.
	SizeCoord[SizeCoord.last].size1 = calcSizeArray(objectName, objectArrayElement);
	SizeCoord[SizeCoord.last].size2 = calcSizeArray(objectName, addSizeContainer);

	SizeCoord.last++; // Increment counter for next entry.

}

// ====================================================================

function setCoordInputUpDown(setCoordObjectUp, setCoordObjectDn, parentInput, rleft, rtop, rright, rbottom)
{
	// Set the coordinates for the up/down button for an input box.

	setCoordObjectUp.rleft = 		rleft;
	setCoordObjectUp.rtop = 		rtop;
	setCoordObjectUp.rright = 		rright;
	setCoordObjectUp.rbottom = 		rbottom;

	setCoordObjectDn.rleft = 		rleft;
	setCoordObjectDn.rtop = 		rtop;
	setCoordObjectDn.rright = 		rright;
	setCoordObjectDn.rbottom = 		rbottom;

	setCoordObjectUp.width =	 	(Crd[parentInput].width/5)+3;
	setCoordObjectUp.height =		(Crd[parentInput].height/2)+3;
	setCoordObjectUp.x =			Crd[parentInput].x+Crd[parentInput].width-setCoordObjectUp.width;
	setCoordObjectUp.y = 			Crd[parentInput].y;

	setCoordObjectDn.width =	 	setCoordObjectUp.width;
	setCoordObjectDn.height =		setCoordObjectUp.height;
	setCoordObjectDn.x =			setCoordObjectUp.x;
	setCoordObjectDn.y = 			Crd[parentInput].y+Crd[parentInput].height-setCoordObjectDn.height+2;
}

// ====================================================================