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
	// Use this function as a shortcut to change the size of a GUI control, via an array containing x, y, width and height. 

	switch (rleft)
	{
		case left_screen:
			size_x1 = objectArrayElement.x;
		break;
		case right_screen:
			size_x1 = -objectArrayElement.x-objectArrayElement.width;
		break;
	}
	switch (rtop)
	{
		case top_screen:
			size_y1 = objectArrayElement.y;
		break;
		case bottom_screen:
			size_y1 = -objectArrayElement.y-objectArrayElement.height;
		break;
	}
	switch (rright)
	{
		case left_screen:
			size_x2 = objectArrayElement.x+objectArrayElement.width;
		break;
		case right_screen:
			if (rleft == right_screen)
				size_x2 = -objectArrayElement.x;
			else
				size_x2 = -objectArrayElement.width;				
		break;
	}
	switch (rbottom)
	{
		case top_screen:
			size_y2 = objectArrayElement.y+objectArrayElement.height;
		break;
		case bottom_screen:
			if (rtop == bottom_screen)
				size_y2 = -objectArrayElement.y;
			else
				size_y2 = -objectArrayElement.height;
		break;
	}

	getGUIObjectByName(objectName).size = new GUISize(size_x1, size_y1, size_x2, size_y2, rleft, rtop, rright, rbottom);

/*
	if (rleft == left_screen && rtop == top_screen && rright == left_screen && rbottom == top_screen)
		// Object is aligned to top left.
		getGUIObjectByName(objectName).size = new GUISize(objectArrayElement.x, objectArrayElement.y, objectArrayElement.x+objectArrayElement.width, objectArrayElement.y+objectArrayElement.height, rleft, rtop, rright, rbottom);
	else
	if (rleft == left_screen && rtop == bottom_screen && rright == left_screen && rbottom == bottom_screen)
		// Object is aligned to bottom left.
		getGUIObjectByName(objectName).size = new GUISize(objectArrayElement.x, -objectArrayElement.y-objectArrayElement.height, objectArrayElement.x+objectArrayElement.width, -objectArrayElement.y, rleft, rtop, rright, rbottom);
	else
	if (rleft == right_screen && rtop == top_screen && rright == right_screen && rbottom == top_screen)
		// Object is aligned to top right.
		getGUIObjectByName(objectName).size = new GUISize(-objectArrayElement.x-objectArrayElement.width, objectArrayElement.y, -objectArrayElement.x, objectArrayElement.y+objectArrayElement.height, rleft, rtop, rright, rbottom);
	else
	if (rleft == right_screen && rtop == bottom_screen && rright == right_screen && rbottom == bottom_screen)
		// Object is aligned to bottom right.
		getGUIObjectByName(objectName).size = new GUISize(-objectArrayElement.x-objectArrayElement.width, -objectArrayElement.y-objectArrayElement.height, -objectArrayElement.x, -objectArrayElement.y, rleft, rtop, rright, rbottom);
	else
	if (rleft == left_screen && rtop == top_screen && rright == right_screen && rbottom == top_screen)
		// Object is aligned between left top and right top.
		getGUIObjectByName(objectName).size = new GUISize(objectArrayElement.x, objectArrayElement.y, -objectArrayElement.width, objectArrayElement.y+objectArrayElement.height, rleft, rtop, rright, rbottom);
	else
	if (rleft == left_screen && rtop == top_screen && rright == right_screen && rbottom == bottom_screen)
		// Object is aligned between left top and right bottom.
		getGUIObjectByName(objectName).size = new GUISize(objectArrayElement.x, objectArrayElement.y, -objectArrayElement.width, -objectArrayElement.height, rleft, rtop, rright, rbottom);
	else
	if (rleft == left_screen && rtop == bottom_screen && rright == right_screen && rbottom == bottom_screen)
		// Object is aligned between left bottom and right bottom.
		getGUIObjectByName(objectName).size = new GUISize(objectArrayElement.x, -objectArrayElement.y-objectArrayElement.height, -objectArrayElement.width, objectArrayElement.y, rleft, rtop, rright, rbottom);
	else
	if (rleft == left_screen && rtop == top_screen && rright == left_screen && rbottom == bottom_screen)
		// Object is aligned between left top and left bottom.
		getGUIObjectByName(objectName).size = new GUISize(objectArrayElement.x, objectArrayElement.y, objectArrayElement.x+objectArrayElement.width, -objectArrayElement.height, rleft, rtop, rright, rbottom);
	else
	if (rleft == left_screen && rtop == mid_screen && rright == left_screen && rbottom == mid_screen)
		// Object is aligned to middle left.
		getGUIObjectByName(objectName).size = new GUISize(objectArrayElement.x, objectArrayElement.y, objectArrayElement.x+objectArrayElement.width, objectArrayElement.y+objectArrayElement.height, rleft, rtop, rright, rbottom);
*/
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
		GUIObjectUnhide("session_gui");
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
		GUIObjectHide("session_gui");
		GUIObjectHide("always_on");
	}

	writeConsole("GUI flipped to " + GUIType + ".");

}
