// Main Atlas Scenario Editor JS Script file
// Contains functions and code for the game's integrated Scenario Editor.

// ====================================================================

function initAtlas()
{
	// Initialise coordinate set for this page.
	atlasCoord = new Array();
	atlasCoord_Last = 0;

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

	// ============================================= CREATE MAP SECTION ===============================================

	// Second heading of Create Map section menu.
	ATLAS_LEFT_PANE_SECTION_MAP_HEADING_2		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].width-2,
		21+2
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_BKG].x,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].height+29
	);

	// Topmost heading of Create Map section menu.
	ATLAS_LEFT_PANE_SECTION_MAP_HEADING_1		= addSizeArrayWH(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_HEADING_2].width,
		31
	); atlasCoord_Last 			= addSizeArrayXY(atlasCoord, atlasCoord_Last,
		atlasCoord[ATLAS_LEFT_PANE_SECTION_MAP_HEADING_2].x,
		atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].y+atlasCoord[ATLAS_MAINBORDER_TOOLBAR_BKG_MAX].height
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

function atlasToolbarButton_1()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_2()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_3()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_4()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_5()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_6()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_7()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_8()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_9()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_10()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_11()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_12()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_13()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_14()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_15()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_16()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_17()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_18()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_19()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_20()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_21()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_22()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_23()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_24()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_25()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_26()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_27()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_28()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_29()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_30()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_31()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_32()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_33()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_34()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_35()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_36()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_37()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_38()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_39()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_40()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_41()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_42()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_43()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_44()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_45()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_46()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_47()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_48()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_49()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_50()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_51()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_52()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_53()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_54()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_55()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_56()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_57()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_58()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_59()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_60()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_61()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_62()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_63()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_64()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_65()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_66()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_67()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_68()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_69()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_70()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_71()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_72()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_73()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_74()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_75()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_76()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_77()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_78()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_79()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================

function atlasToolbarButton_80()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================
