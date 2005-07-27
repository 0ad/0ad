// Main Atlas Scenario Editor JS Script file
// Contains functions and code for the game's integrated Scenario Editor.

// ====================================================================

function initAtlas()
{
	// ============================================ CONSTANTS =================================================

	ATLAS_COUNTER_BOX = new Object();
	ATLAS_COUNTER_BOX.width = 9;
	ATLAS_COUNTER_BOX.height = 5;

	// ============================================= GLOBALS ==================================================

	initAtlasMainScreen();
	initAtlasSectionMapCreator();
	initAtlasSectionTerrainEditor();
	initAtlasSectionObjectEditor();
}

// ====================================================================

function initAtlasMainScreen()
{
	// Top-left corner piece of main editor frame.
	ATLAS_MAINBORDER_LT_CORNER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 42; 
	Crd[Crd.last-1].height	= 42; 
	Crd[Crd.last-1].x	= 0; 
	Crd[Crd.last-1].y	= 0; 

	// Top-right corner piece of main editor frame ("Info Selection Box").
	ATLAS_MAINBORDER_RT_CORNER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 203; 
	Crd[Crd.last-1].height	= 54; 
	Crd[Crd.last-1].x	= 0; 
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_LT_CORNER].y;

	// Info window in top-right corner.
	ATLAS_INFO_WINDOW = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_MAINBORDER_RT_CORNER].width-30-6;
	Crd[Crd.last-1].height	= Crd[ATLAS_MAINBORDER_RT_CORNER].height-9;
	Crd[Crd.last-1].x	= 3;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_RT_CORNER].y+3;

	// Top menu bar.
	ATLAS_MAINBORDER_MENU_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_MAINBORDER_RT_CORNER].width;
	Crd[Crd.last-1].height	= 19;
	Crd[Crd.last-1].x	= Crd[ATLAS_MAINBORDER_LT_CORNER].x+Crd[ATLAS_MAINBORDER_LT_CORNER].width;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_LT_CORNER].y;

	// Menu buttons.	
	ATLAS_MAINBORDER_MENU_BUTTON = new Object();
	ATLAS_MAINBORDER_MENU_BUTTON.span = 0;
	ATLAS_MAINBORDER_MENU_BUTTON.max = 12;

	for (ATLAS_MAINBORDER_MENU_BUTTON.last = 1; ATLAS_MAINBORDER_MENU_BUTTON.last <= ATLAS_MAINBORDER_MENU_BUTTON.max; ATLAS_MAINBORDER_MENU_BUTTON.last++)
	{
		ATLAS_MAINBORDER_MENU_BUTTON[ATLAS_MAINBORDER_MENU_BUTTON.last] = Crd.last;
		Crd[Crd.last] = new Object();
		Crd[Crd.last].width = 60;
		Crd[Crd.last].height = Crd[ATLAS_MAINBORDER_MENU_BKG].height;
		Crd[Crd.last].rleft = left_screen;
		Crd[Crd.last].rtop = top_screen;
		Crd[Crd.last].rright = left_screen;
		Crd[Crd.last].rbottom = top_screen;

		if (ATLAS_MAINBORDER_MENU_BUTTON.last == 1)
			Crd[Crd.last].x = Crd[ATLAS_MAINBORDER_MENU_BKG].x;
		else
			Crd[Crd.last].x = Crd[ATLAS_MAINBORDER_MENU_BUTTON[ATLAS_MAINBORDER_MENU_BUTTON.last]-1].x+Crd[Crd.last].width+ATLAS_MAINBORDER_MENU_BUTTON.span;

		Crd[Crd.last].y = Crd[ATLAS_MAINBORDER_MENU_BKG].y;

		Crd.last++;
	}

	// Top tool bar (1 row).
	ATLAS_MAINBORDER_TOOLBAR_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_MAINBORDER_RT_CORNER].width-Crd[ATLAS_MAINBORDER_LT_CORNER].width;
	Crd[Crd.last-1].height	= 20;
	Crd[Crd.last-1].x	= Crd[ATLAS_MAINBORDER_LT_CORNER].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_MENU_BKG].y+Crd[ATLAS_MAINBORDER_MENU_BKG].height;

	// Top tool bar (max: two rows).
	ATLAS_MAINBORDER_TOOLBAR_BKG_MAX = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].width;
	Crd[Crd.last-1].height	= 35;
	Crd[Crd.last-1].x	= Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].y;

	// Fully Minimise arrow on tool bar.
	ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 12;
	Crd[Crd.last-1].height	= 4;
	Crd[Crd.last-1].x	= Crd[ATLAS_MAINBORDER_RT_CORNER].x+Crd[ATLAS_MAINBORDER_RT_CORNER].width+Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].width-15;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].y+3;

	// Minimise arrow on tool bar.
	ATLAS_MAINBORDER_TOOLBAR_MINIMISE_ARROW = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].width;
	Crd[Crd.last-1].height	= Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].height;
	Crd[Crd.last-1].x	= Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].height-Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].height-1;

	// Fully Maximise arrow on tool bar.
	ATLAS_MAINBORDER_TOOLBAR_FULLY_MAXIMISE_ARROW = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].width;
	Crd[Crd.last-1].height	= Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].height;
	Crd[Crd.last-1].x	= Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].y+Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].height-Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MAXIMISE_ARROW].height-Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MAXIMISE_ARROW].height-1;

	// Maximise arrow on tool bar.
	ATLAS_MAINBORDER_TOOLBAR_MAXIMISE_ARROW = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].width;
	Crd[Crd.last-1].height	= Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].height;
	Crd[Crd.last-1].x	= Crd[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_MENU_BKG].y+Crd[ATLAS_MAINBORDER_MENU_BKG].height-Crd[ATLAS_MAINBORDER_TOOLBAR_MAXIMISE_ARROW].height-Crd[ATLAS_MAINBORDER_TOOLBAR_MAXIMISE_ARROW].height-1;

	// Toolbar buttons.	
	ATLAS_MAINBORDER_TOOLBAR_BUTTON = new Object();
	ATLAS_MAINBORDER_TOOLBAR_BUTTON.span = 4;
	ATLAS_MAINBORDER_TOOLBAR_BUTTON.max = 80;
	for (ATLAS_MAINBORDER_TOOLBAR_BUTTON.last = 1; ATLAS_MAINBORDER_TOOLBAR_BUTTON.last <= ATLAS_MAINBORDER_TOOLBAR_BUTTON.max; ATLAS_MAINBORDER_TOOLBAR_BUTTON.last++)
	{
		ATLAS_MAINBORDER_TOOLBAR_BUTTON[ATLAS_MAINBORDER_TOOLBAR_BUTTON.last] = Crd.last;
		Crd[Crd.last] = new Object();
		Crd[Crd.last].width = 15;
		Crd[Crd.last].height = 15;
		Crd[Crd.last].rleft = left_screen;
		Crd[Crd.last].rtop = top_screen;
		Crd[Crd.last].rright = left_screen;
		Crd[Crd.last].rbottom = top_screen;

		if (ATLAS_MAINBORDER_TOOLBAR_BUTTON.last == 1)
			Crd[Crd.last].x = Crd[ATLAS_MAINBORDER_MENU_BKG].x+5+ATLAS_MAINBORDER_TOOLBAR_BUTTON.span;
		else
			Crd[Crd.last].x = Crd[ATLAS_MAINBORDER_TOOLBAR_BUTTON[ATLAS_MAINBORDER_TOOLBAR_BUTTON.last]-1].x+Crd[Crd.last].width+ATLAS_MAINBORDER_TOOLBAR_BUTTON.span;

		if (ATLAS_MAINBORDER_TOOLBAR_BUTTON.last == 41)
			Crd[Crd.last].x = Crd[ATLAS_MAINBORDER_MENU_BKG].x+5+ATLAS_MAINBORDER_TOOLBAR_BUTTON.span;

		if (ATLAS_MAINBORDER_TOOLBAR_BUTTON.last >= 41)
			Crd[Crd.last].y = Crd[ATLAS_MAINBORDER_MENU_BKG].y+Crd[ATLAS_MAINBORDER_MENU_BKG].height+1+Crd[ATLAS_MAINBORDER_TOOLBAR_BUTTON[ATLAS_MAINBORDER_TOOLBAR_BUTTON.last]-1].height;
		else
			Crd[Crd.last].y = Crd[ATLAS_MAINBORDER_MENU_BKG].y+Crd[ATLAS_MAINBORDER_MENU_BKG].height+1;

		Crd.last++;
	}

	// Left-hand selection pane.
	ATLAS_LEFT_PANE_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 187;
	Crd[Crd.last-1].height	= 0;
	Crd[Crd.last-1].x	= Crd[ATLAS_MAINBORDER_LT_CORNER].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].y+Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].height;

	// ============================================= GENERIC BOTTOM SECTION MENU ===============================================

	// Bottom-right Mini Map Background.
	ATLAS_MINIMAP_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 182;
	Crd[Crd.last-1].height	= 182;
	Crd[Crd.last-1].x	= 0;
	Crd[Crd.last-1].y	= 0;

	// Bottom-right Mini Map.
	ATLAS_MINIMAP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 140;
	Crd[Crd.last-1].height	= 140;
	Crd[Crd.last-1].x	= 20;
	Crd[Crd.last-1].y	= 20;

	// Bottom selection pane.
	ATLAS_BOTTOM_PANE_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_BKG].x-Crd[ATLAS_LEFT_PANE_BKG].width-Crd[ATLAS_MINIMAP_BKG].x-Crd[ATLAS_MINIMAP_BKG].width+6;
	Crd[Crd.last-1].height	= 148;
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_BKG].x+Crd[ATLAS_LEFT_PANE_BKG].width-3;
	Crd[Crd.last-1].y	= Crd[ATLAS_LEFT_PANE_BKG].height;

	// Left-Bottom selection pane corner.
	ATLAS_LB_CORNER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 20;
	Crd[Crd.last-1].height	= 20;
	Crd[Crd.last-1].x	= Crd[ATLAS_BOTTOM_PANE_BKG].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_BOTTOM_PANE_BKG].height-4;

	// Right-Bottom selection pane corner.
	ATLAS_RB_CORNER = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_MINIMAP_BKG].height-Crd[ATLAS_BOTTOM_PANE_BKG].height+3;
	Crd[Crd.last-1].height	= Crd[ATLAS_MINIMAP_BKG].height-Crd[ATLAS_BOTTOM_PANE_BKG].height+3;
	Crd[Crd.last-1].x	= Crd[ATLAS_MINIMAP_BKG].width-2;
	Crd[Crd.last-1].y	= Crd[ATLAS_LB_CORNER].y;

	// Atlas tooltip window.
	ATLAS_TOOLTIP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_MINIMAP_BKG].width-20;
	Crd[Crd.last-1].height	= 82;
	Crd[Crd.last-1].x	= Crd[ATLAS_MINIMAP_BKG].x+10;
	Crd[Crd.last-1].y	= Crd[ATLAS_MINIMAP_BKG].y+Crd[ATLAS_MINIMAP_BKG].height+4;

	// Setup margins for Bottom Section Menu.
	ATLAS_BOTTOM_PANE_SECTION = new Object();
	ATLAS_BOTTOM_PANE_SECTION.LMARGIN = 5;
	ATLAS_BOTTOM_PANE_SECTION.RMARGIN = 5;
	ATLAS_BOTTOM_PANE_SECTION.TMARGIN = 5;
	ATLAS_BOTTOM_PANE_SECTION.BMARGIN = 5;

	// ============================================= GENERIC LEFT SECTION MENU ===============================================

	// Second heading of section menu.
	ATLAS_LEFT_PANE_SECTION_HEADING_2 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_BKG].width-2;
	Crd[Crd.last-1].height	= 21+2;
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_BKG].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+Crd[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].height+29;

	// Topmost heading of section menu.
	ATLAS_LEFT_PANE_SECTION_HEADING_1 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].width;
	Crd[Crd.last-1].height	= 31;
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].x;
	Crd[Crd.last-1].y	= Crd[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+Crd[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].height;

	// Third heading of section menu.
	ATLAS_LEFT_PANE_SECTION_HEADING_3 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].width;
	Crd[Crd.last-1].height	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].height;
	Crd[Crd.last-1].x	= Crd[ATLAS_LEFT_PANE_SECTION_HEADING_2].x;
	Crd[Crd.last-1].y	= 0;

	// Setup margins for Left Section Menu.
	ATLAS_LEFT_PANE_SECTION = new Object();
	ATLAS_LEFT_PANE_SECTION.LMARGIN = 7;
	ATLAS_LEFT_PANE_SECTION.RMARGIN = 10;
	ATLAS_LEFT_PANE_SECTION.TMARGIN = 10;
	ATLAS_LEFT_PANE_SECTION.BMARGIN = 12;
}

// ====================================================================

function atlasUpdateInfoWindow()
{
	// Refresh content of Info Window.

	getGUIObjectByName("ATLAS_INFO_WINDOW").caption = "File: something.map\nWorkspace: MyWorkspace\nFPS: " + getFPS();
}

// ====================================================================

function atlasFullyMinimiseToolbar()
{
	// Hide toolbar.

	guiHide("ATLAS_MAINBORDER_TOOLBAR");
	guiUnHide("ATLAS_MAINBORDER_TOOLBAR_MAXIMISE_ARROW");
	guiHide("ATLAS_MAINBORDER_TOOLBAR_BUTTON_ROW_1");
	guiHide("ATLAS_MAINBORDER_TOOLBAR_BUTTON_ROW_2");

	// Set toolbar height.
	Crd[ATLAS_LEFT_PANE_BKG].y = Crd[ATLAS_MAINBORDER_MENU_BKG].y+Crd[ATLAS_MAINBORDER_MENU_BKG].height;
	setSizeArray("ATLAS_LEFT_PANE_BKG", Crd[ATLAS_LEFT_PANE_BKG]);
}

// ====================================================================

function atlasMinimiseToolbar()
{
	// Reduce toolbar to one row.

	guiHide("ATLAS_MAINBORDER_TOOLBAR_MAX");
	guiUnHide("ATLAS_MAINBORDER_TOOLBAR");
	guiUnHide("ATLAS_MAINBORDER_TOOLBAR_BUTTON_ROW_1");
	guiHide("ATLAS_MAINBORDER_TOOLBAR_BUTTON_ROW_2");

	// Set toolbar height.
	Crd[ATLAS_LEFT_PANE_BKG].y = Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].y+Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].height;
	setSizeArray("ATLAS_LEFT_PANE_BKG", Crd[ATLAS_LEFT_PANE_BKG]);
}

// ====================================================================

function atlasFullyMaximiseToolbar()
{
	// Extend toolbar to two rows.

	guiHide("ATLAS_MAINBORDER_TOOLBAR");
	guiUnHide("ATLAS_MAINBORDER_TOOLBAR_MAX");
	guiUnHide("ATLAS_MAINBORDER_TOOLBAR_BUTTON_ROW_1");
	guiUnHide("ATLAS_MAINBORDER_TOOLBAR_BUTTON_ROW_2");

	// Set toolbar height.
	Crd[ATLAS_LEFT_PANE_BKG].y = Crd[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+Crd[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].height;
	setSizeArray("ATLAS_LEFT_PANE_BKG", Crd[ATLAS_LEFT_PANE_BKG]);
}

// ====================================================================

function atlasMaximiseToolbar()
{
	// Extend toolbar to one row.

	guiUnHide("ATLAS_MAINBORDER_TOOLBAR");
	guiHide("ATLAS_MAINBORDER_TOOLBAR_MAXIMISE_ARROW");
	guiUnHide("ATLAS_MAINBORDER_TOOLBAR_BUTTON_ROW_1");
	guiHide("ATLAS_MAINBORDER_TOOLBAR_BUTTON_ROW_2");

	// Set toolbar height.
	Crd[ATLAS_LEFT_PANE_BKG].y = Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].y+Crd[ATLAS_MAINBORDER_TOOLBAR_BKG].height;
	setSizeArray("ATLAS_LEFT_PANE_BKG", Crd[ATLAS_LEFT_PANE_BKG]);
}

// ====================================================================

function atlasOpenSectionMenu(atlasMenuName)
{
	// Open the specified Section Menu; only one is open at a time.

	// Clear all section menus to begin.
	guiHide("ATLAS_LEFT_PANE_SECTION_MAP");
	guiHide("ATLAS_LEFT_PANE_SECTION_TERRAIN");
	guiHide("ATLAS_BOTTOM_PANE_SECTION_TERRAIN");
	guiHide("ATLAS_LEFT_PANE_SECTION_OBJECT");
	guiHide("ATLAS_BOTTOM_PANE_SECTION_OBJECT");

	switch (atlasMenuName)
	{
		case "none":
			// Hide backgrounds.
			guiHide("ATLAS_LEFT_PANE");
			guiHide("ATLAS_BOTTOM_PANE");
			// Hide headings.
			guiHide("ATLAS_LEFT_PANE_SECTION_HEADING_1");
			guiHide("ATLAS_LEFT_PANE_SECTION_HEADING_2");
			guiHide("ATLAS_LEFT_PANE_SECTION_HEADING_3");
		break;
		case "ATLAS_LEFT_PANE_SECTION_MAP":
			// Toggle backgrounds.
			guiUnHide("ATLAS_LEFT_PANE");
			guiHide("ATLAS_BOTTOM_PANE");
			// Reveal headings.
			guiRenameAndReveal("ATLAS_LEFT_PANE_SECTION_HEADING_1", "Map Creator");
			guiRenameAndReveal("ATLAS_LEFT_PANE_SECTION_HEADING_2", "Map Type");
			guiRenameAndReveal("ATLAS_LEFT_PANE_SECTION_HEADING_3", "Map Settings");
		break;
		case "ATLAS_LEFT_PANE_SECTION_TERRAIN":
			// Toggle backgrounds.
			guiUnHide("ATLAS_LEFT_PANE");
			guiUnHide("ATLAS_BOTTOM_PANE");
			guiUnHide("ATLAS_BOTTOM_PANE_SECTION_TERRAIN");
			// Reveal headings.
			guiRenameAndReveal("ATLAS_LEFT_PANE_SECTION_HEADING_1", "Terrain Editor");
			guiRenameAndReveal("ATLAS_LEFT_PANE_SECTION_HEADING_2", "Edit Elevation");
			guiHide("ATLAS_LEFT_PANE_SECTION_HEADING_3");
		break;
		case "ATLAS_LEFT_PANE_SECTION_OBJECT":
			// Toggle backgrounds.
			guiUnHide("ATLAS_LEFT_PANE");
			guiUnHide("ATLAS_BOTTOM_PANE");
			guiUnHide("ATLAS_BOTTOM_PANE_SECTION_OBJECT");
			// Reveal headings.
			guiRenameAndReveal("ATLAS_LEFT_PANE_SECTION_HEADING_1", "Object Editor");
			guiRenameAndReveal("ATLAS_LEFT_PANE_SECTION_HEADING_2", "Object List");
			guiHide("ATLAS_LEFT_PANE_SECTION_HEADING_3");
		break;
	}

	// Reveal Section Menu content.
	if (atlasMenuName != "none")
		guiUnHide(atlasMenuName);
}

// ====================================================================
