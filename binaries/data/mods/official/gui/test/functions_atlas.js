// Main Atlas Scenario Editor JS Script file
// Contains functions and code for the game's integrated Scenario Editor.

// ====================================================================

function initAtlas()
{
	// Initialise coordinate set for this page.
	atlasCoord = new Array();
	atlasCoord_Last = 0;

	// =============================================   GLOBALS =================================================

	ATLAS_COUNTER_BOX = new Object();
	ATLAS_COUNTER_BOX.width = 9;
	ATLAS_COUNTER_BOX.height = 5;

	// ============================================= MAIN SCREEN ===============================================

	// Top-left corner piece of main editor frame.
	ATLAS_MAINBORDER_LT_CORNER 		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		42,
		42
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		0,
		0
	);

	// Top-right corner piece of main editor frame ("Info Selection Box").
	ATLAS_MAINBORDER_RT_CORNER 		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		203,
		54
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		0,
		atlasCoord[ATLAS_MAINBORDER_LT_CORNER].y
	);

	// Info window in top-right corner.
	ATLAS_INFO_WINDOW	 		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_RT_CORNER].width-30-6,
		atlasCoord[ATLAS_MAINBORDER_RT_CORNER].height-9
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		3,
		atlasCoord[ATLAS_MAINBORDER_RT_CORNER].y+3
	);

	// Top menu bar.
	ATLAS_MAINBORDER_MENU_BKG 		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_RT_CORNER].width,
		19
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_LT_CORNER].x+atlasCoord[ATLAS_MAINBORDER_LT_CORNER].width,
		atlasCoord[ATLAS_MAINBORDER_LT_CORNER].y
	);

	// Menu buttons.	
	ATLAS_MAINBORDER_MENU_BUTTON = new Object();
	ATLAS_MAINBORDER_MENU_BUTTON.span = 0;
	ATLAS_MAINBORDER_MENU_BUTTON.max = 12;
	for (ATLAS_MAINBORDER_MENU_BUTTON.last = 1; ATLAS_MAINBORDER_MENU_BUTTON.last <= ATLAS_MAINBORDER_MENU_BUTTON.max; ATLAS_MAINBORDER_MENU_BUTTON.last++)
	{
		ATLAS_MAINBORDER_MENU_BUTTON[ATLAS_MAINBORDER_MENU_BUTTON.last] = atlasCoord_Last;
		atlasCoord[atlasCoord_Last] = new Object();
		atlasCoord[atlasCoord_Last].width = 60;
		atlasCoord[atlasCoord_Last].height = atlasCoord[ATLAS_MAINBORDER_MENU_BKG].height;

		if (ATLAS_MAINBORDER_MENU_BUTTON.last == 1)
			atlasCoord[atlasCoord_Last].x = atlasCoord[ATLAS_MAINBORDER_MENU_BKG].x;
		else
			atlasCoord[atlasCoord_Last].x = atlasCoord[ATLAS_MAINBORDER_MENU_BUTTON[ATLAS_MAINBORDER_MENU_BUTTON.last]-1].x+atlasCoord[atlasCoord_Last].width+ATLAS_MAINBORDER_MENU_BUTTON.span;

		atlasCoord[atlasCoord_Last].y = atlasCoord[ATLAS_MAINBORDER_MENU_BKG].y;

		atlasCoord_Last++;
	}

	// Top tool bar (1 row).
	ATLAS_MAINBORDER_TOOLBAR_BKG 		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_RT_CORNER].width-atlasCoord[ATLAS_MAINBORDER_LT_CORNER].width,
		20
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_LT_CORNER].x,
		atlasCoord[ATLAS_MAINBORDER_MENU_BKG].y+atlasCoord[ATLAS_MAINBORDER_MENU_BKG].height
	);

	// Top tool bar (max: two rows).
	ATLAS_MAINBORDER_TOOLBAR_BKG_MAX	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].width,
		35
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].x,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].y
	);

	// Fully Minimise arrow on tool bar.
	ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		8,
		4
	); atlasCoord_Last 				= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_RT_CORNER].x+atlasCoord[ATLAS_MAINBORDER_RT_CORNER].width+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].width-11,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].y+3
	);

	// Minimise arrow on tool bar.
	ATLAS_MAINBORDER_TOOLBAR_MINIMISE_ARROW	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].width,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].x,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].height-atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].height-1
	);

	// Fully Maximise arrow on tool bar.
	ATLAS_MAINBORDER_TOOLBAR_FULLY_MAXIMISE_ARROW	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].width,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].x,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].height-atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MAXIMISE_ARROW].height-atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MAXIMISE_ARROW].height-1
	);

	// Maximise arrow on tool bar.
	ATLAS_MAINBORDER_TOOLBAR_MAXIMISE_ARROW	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].width,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_FULLY_MINIMISE_ARROW].x,
		atlasCoord[ATLAS_MAINBORDER_MENU_BKG].y+atlasCoord[ATLAS_MAINBORDER_MENU_BKG].height-atlasCoord[ATLAS_MAINBORDER_TOOLBAR_MAXIMISE_ARROW].height-atlasCoord[ATLAS_MAINBORDER_TOOLBAR_MAXIMISE_ARROW].height-1
	);

	// Toolbar buttons.	
	ATLAS_MAINBORDER_TOOLBAR_BUTTON = new Object();
	ATLAS_MAINBORDER_TOOLBAR_BUTTON.span = 4;
	ATLAS_MAINBORDER_TOOLBAR_BUTTON.max = 80;
	for (ATLAS_MAINBORDER_TOOLBAR_BUTTON.last = 1; ATLAS_MAINBORDER_TOOLBAR_BUTTON.last <= ATLAS_MAINBORDER_TOOLBAR_BUTTON.max; ATLAS_MAINBORDER_TOOLBAR_BUTTON.last++)
	{
		ATLAS_MAINBORDER_TOOLBAR_BUTTON[ATLAS_MAINBORDER_TOOLBAR_BUTTON.last] = atlasCoord_Last;
		atlasCoord[atlasCoord_Last] = new Object();
		atlasCoord[atlasCoord_Last].width = 15;
		atlasCoord[atlasCoord_Last].height = 15;

		if (ATLAS_MAINBORDER_TOOLBAR_BUTTON.last == 1)
			atlasCoord[atlasCoord_Last].x = atlasCoord[ATLAS_MAINBORDER_MENU_BKG].x+5+ATLAS_MAINBORDER_TOOLBAR_BUTTON.span;
		else
			atlasCoord[atlasCoord_Last].x = atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BUTTON[ATLAS_MAINBORDER_TOOLBAR_BUTTON.last]-1].x+atlasCoord[atlasCoord_Last].width+ATLAS_MAINBORDER_TOOLBAR_BUTTON.span;

		if (ATLAS_MAINBORDER_TOOLBAR_BUTTON.last == 41)
			atlasCoord[atlasCoord_Last].x = atlasCoord[ATLAS_MAINBORDER_MENU_BKG].x+5+ATLAS_MAINBORDER_TOOLBAR_BUTTON.span;

		if (ATLAS_MAINBORDER_TOOLBAR_BUTTON.last >= 41)
			atlasCoord[atlasCoord_Last].y = atlasCoord[ATLAS_MAINBORDER_MENU_BKG].y+atlasCoord[ATLAS_MAINBORDER_MENU_BKG].height+1+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BUTTON[ATLAS_MAINBORDER_TOOLBAR_BUTTON.last]-1].height;
		else
			atlasCoord[atlasCoord_Last].y = atlasCoord[ATLAS_MAINBORDER_MENU_BKG].y+atlasCoord[ATLAS_MAINBORDER_MENU_BKG].height+1;

		atlasCoord_Last++;
	}

	// Left-hand selection pane.
	ATLAS_LEFT_PANE_BKG	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		187,
		0
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MAINBORDER_LT_CORNER].x,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].height
	);

	// Bottom-right Mini Map Background.
	ATLAS_MINIMAP_BKG	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		182,
		182
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		0,
		0
	);

	// Bottom-right Mini Map.
	ATLAS_MINIMAP		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		140,
		140
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		20,
		20
	);

	// Bottom selection pane.
	ATLAS_BOTTOM_PANE_BKG	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].x-atlasCoord[ATLAS_LEFT_PANE_BKG].width-atlasCoord[ATLAS_MINIMAP_BKG].x-atlasCoord[ATLAS_MINIMAP_BKG].width+6,
		148
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].x+atlasCoord[ATLAS_LEFT_PANE_BKG].width-3,
		atlasCoord[ATLAS_LEFT_PANE_BKG].height
	);

	// Left-Bottom selection pane corner.
	ATLAS_LB_CORNER		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		20,
		20
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_BOTTOM_PANE_BKG].x,
		atlasCoord[ATLAS_BOTTOM_PANE_BKG].height-4
	);

	// Right-Bottom selection pane corner.
	ATLAS_RB_CORNER		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MINIMAP_BKG].height-atlasCoord[ATLAS_BOTTOM_PANE_BKG].height+3,
		atlasCoord[ATLAS_MINIMAP_BKG].height-atlasCoord[ATLAS_BOTTOM_PANE_BKG].height+3
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MINIMAP_BKG].width-2,
		atlasCoord[ATLAS_LB_CORNER].y
	);

	// Atlas tooltip window.
	ATLAS_TOOLTIP		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MINIMAP_BKG].width-20,
		82
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_MINIMAP_BKG].x+10,
		atlasCoord[ATLAS_MINIMAP_BKG].y+atlasCoord[ATLAS_MINIMAP_BKG].height+4
	);

	// ============================================= GENERIC LEFT SECTION MENU ===============================================

	// Second heading of section menu.
	ATLAS_LEFT_PANE_SECTION_HEADING_2		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].width-2,
		21+2
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].x,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].height+29
	);

	// Topmost heading of section menu.
	ATLAS_LEFT_PANE_SECTION_HEADING_1		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].width,
		31
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].x,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].height
	);

	// Third heading of section menu.
	ATLAS_LEFT_PANE_SECTION_HEADING_3		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].x,
		0
	);

	// Setup margins for Left Section Menu.
	ATLAS_LEFT_PANE_SECTION = new Object();
	ATLAS_LEFT_PANE_SECTION.LMARGIN = 7;
	ATLAS_LEFT_PANE_SECTION.RMARGIN = 10;
	ATLAS_LEFT_PANE_SECTION.TMARGIN = 10;
	ATLAS_LEFT_PANE_SECTION.BMARGIN = 12;

	// ============================================= MAP CREATOR SECTION MENU ===============================================

	// ============================================= MAP CREATOR: MAP TYPE ===============================================

	// ============================================= MAP CREATOR: MAP TYPE: MAP SIZE ===============================================

	// Height input box for map size.
	ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		58,
		14
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].width-ATLAS_LEFT_PANE_SECTION.RMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].height+ATLAS_LEFT_PANE_SECTION.TMARGIN+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// "X" between map size input boxes.
	ATLAS_LEFT_PANE_SECTION_MAP_TILE_X				= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		20,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_X].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y
	);

	// Width input box for map size.
	ATLAS_LEFT_PANE_SECTION_MAP_TILE_WIDTH_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_X].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y
	);

	// "Size:" label.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		32,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_WIDTH_INPUT_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y
	);

	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN = 3;

	// "Huge" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		40,
		20
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width-ATLAS_LEFT_PANE_SECTION.RMARGIN+2,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_TILE_HEIGHT_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// "Large" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_LARGE	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width-ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y
	);

	// "Medium" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_MEDIUM	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_LARGE].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_LARGE].width-ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y
	);

	// "Small" map size button.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SMALL	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_MEDIUM].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_MEDIUM].width-ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y
	);

	// Horizontal rule at end of Map Size settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SIZE_HR		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN,
		2
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].x+ATLAS_LEFT_PANE_SECTION.LMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SIZE_BUTTON_HUGE].height+ATLAS_LEFT_PANE_SECTION.BMARGIN
	);

	// ============================================= MAP CREATOR: MAP TYPE: RANDOM MAP ===============================================

	// ============================================= MAP CREATOR: MAP SETTINGS ===============================================

	// "Players:" label for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		45,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].x+ATLAS_LEFT_PANE_SECTION.LMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].height+ATLAS_LEFT_PANE_SECTION.TMARGIN+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Players "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		30,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].y
	);

	// Players up button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		(atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].height/2)+3
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].y-3
	);

	// Players down button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_DN	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].height-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX_DN].height
	);

	// Settlements "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].width-ATLAS_LEFT_PANE_SECTION.RMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_INPUT_BOX].y
	);

	// Settlements up button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		(atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].height/2)+3
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].y-3
	);

	// Settlements down button for "counter" input box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_DN	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].height-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX_DN].height
	);

	// "Settlements" label for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		62,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_SETTLEMENT_INPUT_BOX].y
	);

	// "Resources:" label for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		60,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height+ATLAS_LEFT_PANE_SECTION.TMARGIN+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Resources "drop-down" box for Map Settings.
	ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_COMBO_BOX	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		55,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_PLAYER_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_SETTINGS_RESOURCES_LABEL].y
	);

	// ============================================= MAP CREATOR: MAP SETTINGS: TERRITORIES ===============================================

	// ============================================= MAP CREATOR: GENERATE ===============================================

	// Terrain Map input box.
	ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN-ATLAS_LEFT_PANE_SECTION.LMARGIN,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_3].x+ATLAS_LEFT_PANE_SECTION.LMARGIN+ATLAS_LEFT_PANE_SECTION.LMARGIN,
		ATLAS_LEFT_PANE_SECTION.BMARGIN
	);

	// "Terrain Map:" label.
	ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width,
		20
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].height
	);

	// Height Map input box.
	ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL].height
	);

	// "Height Map:" label.
	ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_LABEL].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].height
	);

	// "Generate!" border.
	ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER				= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width,
		40
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_HEIGHT_MAP_INPUT_BOX].height+atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].height+ATLAS_LEFT_PANE_SECTION.BMARGIN
	);

	ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN = 7;

	// "Generate!" button.
	ATLAS_LEFT_PANE_SECTION_GENERATE_BUTTON				= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_TERRAIN_MAP_INPUT_BOX].width-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].height-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN-ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].x+ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_GENERATE_BORDER].y+ATLAS_LEFT_PANE_SECTION_GENERATE_SPAN
	);

	// ============================================= TERRAIN EDITOR SECTION MENU ===============================================

	// ============================================= TERRAIN EDITOR: EDIT ELEVATION ===============================================

	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_BUTTON_SPAN = 10;

	// Modify button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		50,
		15
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].x+ATLAS_LEFT_PANE_SECTION.LMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_HEADING_2].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Smooth button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].width+ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_BUTTON_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].y
	);

	// Sample button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].width+ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_BUTTON_SPAN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].y
	);

	// Intensity slider bar.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		110,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SMOOTH_BUTTON].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Intensity slider marker.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_MARKER	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		8,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].x+40,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].y+(atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].height/3)
	);

	// Intensity label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		50,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_SLIDER_BAR].y
	);

	// Roughen button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INTENSITY_LABEL].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_MODIFY_BUTTON].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Roughen style combo box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_COMBO_BOX		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		75,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_COMBO_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].y+4
	);

	// Roughen style label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		40,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_COMBO_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_STYLE_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].y
	);

	// Power input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		40,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].y+4+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Up button for power input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX_UP	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		(atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].height/2)+3
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].y-3
	);

	// Down button for power input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX_DN	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX_UP].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].height-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX_DN].height
	);

	// Power label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		40,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].y
	);

	// Scale label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		40,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].y
	);

	// Scale input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		40,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_LABEL].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].y
	);

	// Up button for scale input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX_UP	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		(atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].height/2)+3
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].y-3
	);

	// Down button for scale input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX_DN	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX_UP].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].height-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX_DN].height
	);

	// Increment button.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INCREMENT_BUTTON		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SCALE_INPUT_BOX].height+ATLAS_LEFT_PANE_SECTION.TMARGIN
	);

	// Amount input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		40,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_SAMPLE_BUTTON].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INCREMENT_BUTTON].y
	);

	// Up button for amount input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX_UP	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		(atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX].height/2)+3
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_POWER_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INCREMENT_BUTTON].y-3
	);

	// Down button for amount input box.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX_DN	= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		ATLAS_COUNTER_BOX.width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX_UP].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX].x+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX].width-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX_UP].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX].height-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX_DN].height
	);

	// Amount label.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_LABEL		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		45,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_ROUGHEN_BUTTON].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX].x-atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_LABEL].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_AMOUNT_INPUT_BOX].y
	);

	// Horizontal rule at end of Edit Elevation.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].width-ATLAS_LEFT_PANE_SECTION.LMARGIN-ATLAS_LEFT_PANE_SECTION.RMARGIN,
		2
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].x+ATLAS_LEFT_PANE_SECTION.LMARGIN,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INCREMENT_BUTTON].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_INCREMENT_BUTTON].height+ATLAS_LEFT_PANE_SECTION.BMARGIN
	);

	// List of cliff portraits.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		75,
		162
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].x+3,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].x+3
	);

	// Horizontal rule at end of Paint Cliff.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_ELEVATION_HR].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].height+ATLAS_LEFT_PANE_SECTION.BMARGIN
	);

	// List of water portraits.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_LIST		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_LIST].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].x+3
	);

	// Horizontal rule at end of Paint Water.
	ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_HR		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].width,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].height
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_CLIFF_HR].x,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_LIST].y+atlasCoord[ATLAS_LEFT_PANE_SECTION_TERRAIN_WATER_LIST].height+ATLAS_LEFT_PANE_SECTION.BMARGIN
	);
}

// ====================================================================

function atlasUpdateInfoWindow()
{
	// Refresh content of Info Window.

	getGUIObjectByName("atlas_info_window").caption = "File: something.map\nWorkspace: MyWorkspace\nFPS: " + getFPS();
}

// ====================================================================

function atlasFullyMinimiseToolbar()
{
	// Hide toolbar.

	GUIObjectHide("atlas_mainborder_toolbar");
	GUIObjectUnhide("atlas_mainborder_toolbar_maximise_arrow");
	GUIObjectHide("atlas_mainborder_toolbar_button_row_1");
	GUIObjectHide("atlas_mainborder_toolbar_button_row_2");

	// Set toolbar height.
	atlasCoord[ATLAS_LEFT_PANE_BKG].y = atlasCoord[ATLAS_MAINBORDER_MENU_BKG].y+atlasCoord[ATLAS_MAINBORDER_MENU_BKG].height;
	setSizeArray("atlas_left_pane_bkg", atlasCoord[ATLAS_LEFT_PANE_BKG], left_screen, top_screen, left_screen, bottom_screen);
}

// ====================================================================

function atlasMinimiseToolbar()
{
	// Reduce toolbar to one row.

	GUIObjectHide("atlas_mainborder_toolbar_max");
	GUIObjectUnhide("atlas_mainborder_toolbar");
	GUIObjectUnhide("atlas_mainborder_toolbar_button_row_1");
	GUIObjectHide("atlas_mainborder_toolbar_button_row_2");

	// Set toolbar height.
	atlasCoord[ATLAS_LEFT_PANE_BKG].y = atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].height;
	setSizeArray("atlas_left_pane_bkg", atlasCoord[ATLAS_LEFT_PANE_BKG], left_screen, top_screen, left_screen, bottom_screen);}

// ====================================================================

function atlasFullyMaximiseToolbar()
{
	// Extend toolbar to two rows.

	GUIObjectHide("atlas_mainborder_toolbar");
	GUIObjectUnhide("atlas_mainborder_toolbar_max");
	GUIObjectUnhide("atlas_mainborder_toolbar_button_row_1");
	GUIObjectUnhide("atlas_mainborder_toolbar_button_row_2");

	// Set toolbar height.
	atlasCoord[ATLAS_LEFT_PANE_BKG].y = atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].height;
	setSizeArray("atlas_left_pane_bkg", atlasCoord[ATLAS_LEFT_PANE_BKG], left_screen, top_screen, left_screen, bottom_screen);}

// ====================================================================

function atlasMaximiseToolbar()
{
	// Extend toolbar to one row.

	GUIObjectUnhide("atlas_mainborder_toolbar");
	GUIObjectHide("atlas_mainborder_toolbar_maximise_arrow");
	GUIObjectUnhide("atlas_mainborder_toolbar_button_row_1");
	GUIObjectHide("atlas_mainborder_toolbar_button_row_2");

	// Set toolbar height.
	atlasCoord[ATLAS_LEFT_PANE_BKG].y = atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG].height;
	setSizeArray("atlas_left_pane_bkg", atlasCoord[ATLAS_LEFT_PANE_BKG], left_screen, top_screen, left_screen, bottom_screen);}

// ====================================================================

function atlasOpenSectionMenu(atlasMenuName)
{
	// Open the specified Section Menu; only one is open at a time.

	// Clear all section menus to begin.
	GUIObjectHide("atlas_left_pane_section_map");
	GUIObjectHide("atlas_left_pane_section_terrain");

	switch (atlasMenuName)
	{
		case "none":
			// Hide backgrounds.
			GUIObjectHide("atlas_left_pane");
			GUIObjectHide("atlas_bottom_pane");
			// Hide headings.
			GUIObjectHide("atlas_left_pane_section_heading_1");
			GUIObjectHide("atlas_left_pane_section_heading_2");
			GUIObjectHide("atlas_left_pane_section_heading_3");
		break;
		case "atlas_left_pane_section_map":
			// Toggle backgrounds.
			GUIObjectUnhide("atlas_left_pane");
			GUIObjectHide("atlas_bottom_pane");
			// Reveal headings.
			GUIObjectRenameandReveal("atlas_left_pane_section_heading_1", "Map Creator");
			GUIObjectRenameandReveal("atlas_left_pane_section_heading_2", "Map Type");
			GUIObjectRenameandReveal("atlas_left_pane_section_heading_3", "Map Settings");
		break;
		case "atlas_left_pane_section_terrain":
			// Toggle backgrounds.
			GUIObjectUnhide("atlas_left_pane");
			GUIObjectUnhide("atlas_bottom_pane");
			// Reveal headings.
			GUIObjectRenameandReveal("atlas_left_pane_section_heading_1", "Terrain Editor");
			GUIObjectRenameandReveal("atlas_left_pane_section_heading_2", "Edit Elevation");
			GUIObjectHide("atlas_left_pane_section_heading_3");
		break;
	}

	// Reveal Section Menu content.
	if (atlasMenuName != "none")
		GUIObjectUnhide(atlasMenuName);
}

// ====================================================================
