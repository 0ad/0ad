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

	// Build table of group pane coordinates.
	initGroupPaneTable();
}

// ====================================================================

function initGroupPaneTable()
{
	AddGroupPaneRow(0, 32, 		100, -70);	// Row 0
	AddGroupPaneCol(50, -16, 	50, -16);	// Col 0
	AddGroupPaneCol(50, -52, 	50, -52);	// Col 1
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
