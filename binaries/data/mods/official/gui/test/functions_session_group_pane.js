function initGroupPane()
{
	// Populate the table of Group Pane portrait coordinates.

	// Group Pane table.
	crd_grppane_prt_col = new Array();
	crd_grppane_prt_col.last = 0; 	// Last size added.
	crd_grppane_prt_col.style = 2; 	// Style length.
	crd_grppane_prt_row = new Array();
	crd_grppane_prt_row_bar = new Array();
	crd_grppane_prt_row.last = 0; 	// Last size added.
	crd_grppane_prt_row.style = 2; 	// Style length.
	crd_grppane_prt_last = 0;	// Total group portraits.

	// Build table of group pane coordinates.
	initGroupPaneTable();
}

// ====================================================================

function initGroupPaneTable()
{
	AddGroupPaneRow(0, 32, 		100, -70);	// Row 0
	AddGroupPaneRow(0, 78, 		100, -110);	// Row 1
	AddGroupPaneRow(0, 124,		100, -156);	// Row 2
//	AddGroupPaneRow(0, 170,		100, -202);	// Row 3
	AddGroupPaneCol(50, -16, 	50, -16);	// Col 0
	AddGroupPaneCol(50, -52, 	50, -52);	// Col 1
	AddGroupPaneCol(50, 20, 	50, 20);	// Col 2
	AddGroupPaneCol(50, -88, 	50, -88);	// Col 3
	AddGroupPaneCol(50, 56, 	50, 56);	// Col 4
	AddGroupPaneCol(50, -124, 	50, -124);	// Col 5
	AddGroupPaneCol(50, 92, 	50, 92);	// Col 6
	AddGroupPaneCol(50, -160, 	50, -160);	// Col 7
	AddGroupPaneCol(50, 128, 	50, 128);	// Col 8
	AddGroupPaneCol(50, -196, 	50, -196);	// Col 9
	AddGroupPaneCol(50, 164, 	50, 164);	// Col 10
	AddGroupPaneCol(50, -232, 	50, -232);	// Col 11
	AddGroupPaneCol(50, 200, 	50, 200);	// Col 12
	AddGroupPaneCol(50, -268, 	50, -268);	// Col 13
	AddGroupPaneCol(50, 236, 	50, 236);	// Col 14
}

// ====================================================================

function AddGroupPaneCol(rleft1, left1, rleft2, left2)
{
	// Add a set of column coordinates to the Group Pane table.

	crd_grppane_prt_col[crd_grppane_prt_col.last] = new Array();

	style=0;
	crd_grppane_prt_col[crd_grppane_prt_col.last][style] = new Array(); 
	crd_grppane_prt_col[crd_grppane_prt_col.last][style].size = new GUISize(left1, 0, left1 + crd_portrait_sml_width, 0, rleft1, 0, rleft1, 0);

	style=1;
	crd_grppane_prt_col[crd_grppane_prt_col.last][style] = new Array(); 
	crd_grppane_prt_col[crd_grppane_prt_col.last][style].size = new GUISize(left2, 0, left2 + crd_portrait_sml_width, 0, rleft2, 0, rleft2, 0);

	crd_grppane_prt_col.last++; // Increment array.
}

// ====================================================================

function AddGroupPaneRow(rtop1, top1, rtop2, top2)
{
	// Add a set of row coordinates to the Group Pane table.

	crd_grppane_prt_row[crd_grppane_prt_row.last] = new Array();

	style=0;
	crd_grppane_prt_row[crd_grppane_prt_row.last][style] = new Array();
	crd_grppane_prt_row[crd_grppane_prt_row.last][style].size = new GUISize(0, top1, 0, top1 + crd_portrait_sml_height, 0, rtop1, 0, rtop1);

	style=1;
	crd_grppane_prt_row[crd_grppane_prt_row.last][style] = new Array();
	crd_grppane_prt_row[crd_grppane_prt_row.last][style].size = new GUISize(0, top2, 0, top2 + crd_portrait_sml_height, 0, rtop2, 0, rtop2);

	crd_grppane_prt_row.last++; // Increment array.
}

// ====================================================================

function AddSizeGroupPane(objectName, row, col)
{
	// Used to store the two GUI style sizes for an object on creation.
	// This is a special method that uses the group pane table to specify values.
	// Used later by FlipGUI() to switch the objects to a new set of positions.

	SizeCoord[SizeCoord.last] = new Object();
	SizeCoord[SizeCoord.last].name = objectName;

	style = 0; // Set top GUI style.
	size_col = crd_grppane_prt_col[col][style].size;
	size_row = crd_grppane_prt_row[row][style].size;
	SizeCoord[SizeCoord.last].size1 = new GUISize(size_col.left, size_row.top, size_col.right, size_row.bottom, size_col.rleft, size_row.rtop, size_col.rright, size_row.rbottom);

	style = 1; // Set bottom GUI style.
	size_col = crd_grppane_prt_col[col][style].size;
	size_row = crd_grppane_prt_row[row][style].size;
	SizeCoord[SizeCoord.last].size2 = new GUISize(size_col.left, size_row.top, size_col.right, size_row.bottom, size_col.rleft, size_row.rtop, size_col.rright, size_row.rbottom);

	SizeCoord.last++; // Increment counter for next entry.
	crd_grppane_prt_last++; // Increment group pane portrait counter.
}

// ====================================================================

function AddSizeGroupPaneBar(objectName)
{
	// Create a health bar for the last group pane portrait created.

	var size1 = SizeCoord[SizeCoord.last-1].size1;
	var size2 = SizeCoord[SizeCoord.last-1].size2;

	AddSizeCoord(objectName,
		size1.left, size1.bottom+2, size1.right, size1.bottom+6, size1.rleft, size1.rtop, size1.rright, size1.rbottom, 
		size2.left, size2.bottom+2, size2.right, size2.bottom+6, size2.rleft, size2.rtop, size2.rright, size2.rbottom);
}

// ====================================================================

function UpdateGroupPane()
{
	// Reveal Group Pane.
	getGUIObjectByName("session_group_pane").hidden = false;

	// Set size of Group Pane background.
	if (selection.length <= 15)
	{
		switch (GUIType)
		{
			case "top":
				setSize("session_group_pane_bg", "0%+20 0% 100%-20 0%+86");
			break;
			case "bottom":
				setSize("session_group_pane_bg", "0%+20 100%-86 100%-20 100%");
			break;
		}
	}
	else
	if (selection.length > 15 && selection.length <= 30)
	{
		switch (GUIType)
		{
			case "top":
				setSize("session_group_pane_bg", "0%+20 0% 100%-20 0%+127");
			break;
			case "bottom":
				setSize("session_group_pane_bg", "0%+20 100%-127 100%-20 100%");
			break;
		}
	}
	else
	{
		switch (GUIType)
		{
			case "top":
				setSize("session_group_pane_bg", "0%+20 0% 100%-20 0%+168");
			break;
			case "bottom":
				setSize("session_group_pane_bg", "0%+20 100%-168 100%-20 100%");
			break;
		}
	}

	// Display appropriate portraits.						
	for (groupPaneLoop = 1; groupPaneLoop <= crd_grppane_prt_last; groupPaneLoop++)
	{
		groupPanePortrait = getGUIObjectByName("session_group_pane_portrait_" + groupPaneLoop);
		groupPaneBar = getGUIObjectByName("session_group_pane_portrait_" + groupPaneLoop + "_bar");

		// If it's a valid entity,
		if (groupPaneLoop <= selection.length){
			// Reveal and set to display this entity's portrait in the group pane.
			groupPanePortrait.hidden = false;
			groupPaneBar.hidden = false;
			// Set progress bar for hitpoints.
			if (selection[groupPaneLoop-1].traits.health.curr && selection[groupPaneLoop-1].traits.health.hitpoints)
				groupPaneBar.caption = ((Math.round(selection[groupPaneLoop-1].traits.health.curr) * 100 ) / Math.round(selection[groupPaneLoop-1].traits.health.hitpoints));
			if (selection[groupPaneLoop-1].traits.id.icon)
				setPortrait("session_group_pane_portrait_" + groupPaneLoop, selection[groupPaneLoop-1].traits.id.icon);
		}
		// If it's empty, hide its group portrait.
		else
		{
			groupPanePortrait.hidden = true;
			groupPaneBar.hidden = true;
		}
	}
}
