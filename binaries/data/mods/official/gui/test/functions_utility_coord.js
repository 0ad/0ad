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

function setSizeArray(objectName, objectArrayElement, rleft, rtop, rright, rbottom)
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
	}
	switch (setSizeContainer.rtop)
	{
		case top_screen:
			setSizeContainer.y1 = objectArrayElement.y;
		break;
		case bottom_screen:
			setSizeContainer.y1 = -objectArrayElement.y-objectArrayElement.height;
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
	}

	// Set appropriate size for dimensions.
	getGUIObjectByName(objectName).size = new GUISize(setSizeContainer.x1, setSizeContainer.y1, setSizeContainer.x2, setSizeContainer.y2, setSizeContainer.rleft, setSizeContainer.rtop, setSizeContainer.rright, setSizeContainer.rbottom);
} 

// ====================================================================

function FlipGUI(NewGUIType)
{
	// Sets the GUI coordinates and graphics so that the panel is either at the top or bottom of the screen.

	switch (NewGUIType)
	{
		// Set which GUI to use.
		case "top":
		case "bottom":
		case "none":
			GUIType=NewGUIType;
		break;
		default:
			// If no type specified, toggle.
			if (GUIType == "top")
				GUIType = "bottom";
			else
			if (GUIType == "bottom")
				GUIType = "none";
			else
				GUIType = "top";
		break;
	}

	if (GUIType != "none")
	{
		GUIObjectUnhide("SESSION_GUI");
		GUIObjectUnhide("always_on");

		// Seek through all sizes created.
		for (FlipGUILoop = 0; FlipGUILoop <= SizeCoord.last-1; FlipGUILoop++)
		{
			// Set each object to the other size.
			switch (GUIType)
			{
				case "top":
					setSize(SizeCoord[FlipGUILoop].name, SizeCoord[FlipGUILoop].size1);
					switch (SizeCoord[FlipGUILoop].name){
						case "session_panel_minimap_segbottom1":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = SizeCoord[FlipGUILoop].name;
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = SizeCoord[FlipGUILoop].name + "_lit";
						break;
						case "session_panel_minimap_segbottom2":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = SizeCoord[FlipGUILoop].name;
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = SizeCoord[FlipGUILoop].name + "_lit";
						break;
						case "session_panel_minimap_segbottom3":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = SizeCoord[FlipGUILoop].name;
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = SizeCoord[FlipGUILoop].name + "_lit";
						break;
						case "session_panel_minimap_segbottom4":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = SizeCoord[FlipGUILoop].name;
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = SizeCoord[FlipGUILoop].name + "_lit";
						break;
						case "session_panel_status_bg":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_status_bg_top";
						break;
						default:
						break;
					}
				break;
				case "bottom":
					setSize(SizeCoord[FlipGUILoop].name, SizeCoord[FlipGUILoop].size2);
					switch (SizeCoord[FlipGUILoop].name){
						case "session_panel_minimap_segbottom1":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop1";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segtop1_lit";
						break;
						case "session_panel_minimap_segbottom2":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop2";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segtop2_lit";
						break;
						case "session_panel_minimap_segbottom3":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop3";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segtop3_lit";
						break;
						case "session_panel_minimap_segbottom4":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_minimap_segtop4";
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite_over = "session_panel_minimap_segtop4_lit";
						break;
						case "session_panel_status_bg":
							getGUIObjectByName(SizeCoord[FlipGUILoop].name).sprite = "session_panel_status_bg_bottom";
						break;
						default:
						break;
					}
				break;
			}			
		}

		UpdateGroupPane();
	}
	else
	{
		GUIObjectHide("SESSION_GUI");
		GUIObjectHide("always_on");
	}

	writeConsole("GUI flipped to " + GUIType + ".");

}
