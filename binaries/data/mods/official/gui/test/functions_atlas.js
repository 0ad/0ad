// Main Atlas Scenario Editor JS Script file
// Contains functions and code for the game's integrated Scenario Editor.

// ====================================================================

function initAtlas()
{
	// Top-left corner piece of main editor frame.
	crd_atlas_mainborder_lt_corner_width = 42;
	crd_atlas_mainborder_lt_corner_height = 42;
	crd_atlas_mainborder_lt_corner_x = 0;
	crd_atlas_mainborder_lt_corner_y = 0;

	// Top-right corner piece of main editor frame ("Info Selection Box").
	crd_atlas_mainborder_rt_corner_width = 203;
	crd_atlas_mainborder_rt_corner_height = 54;
	crd_atlas_mainborder_rt_corner_x = 0;
	crd_atlas_mainborder_rt_corner_y = crd_atlas_mainborder_lt_corner_y;

	// Info window in top-right corner.
	crd_atlas_info_window_width = crd_atlas_mainborder_rt_corner_width-30-6;
	crd_atlas_info_window_height = crd_atlas_mainborder_rt_corner_height-9;
	crd_atlas_info_window_x = 3;
	crd_atlas_info_window_y = crd_atlas_mainborder_rt_corner_y+3;

	// Top menu bar.
	crd_atlas_mainborder_menu_bkg_width = crd_atlas_mainborder_rt_corner_width;
	crd_atlas_mainborder_menu_bkg_height = 19;
	crd_atlas_mainborder_menu_bkg_x = crd_atlas_mainborder_lt_corner_x+crd_atlas_mainborder_lt_corner_width;
	crd_atlas_mainborder_menu_bkg_y = crd_atlas_mainborder_lt_corner_y;

	// Top tool bar (1 row).
	crd_atlas_mainborder_toolbar_bkg_width = crd_atlas_mainborder_rt_corner_width-crd_atlas_mainborder_lt_corner_width;
	crd_atlas_mainborder_toolbar_bkg_height = 20;
	crd_atlas_mainborder_toolbar_bkg_x = crd_atlas_mainborder_lt_corner_x;
	crd_atlas_mainborder_toolbar_bkg_y = crd_atlas_mainborder_menu_bkg_y+crd_atlas_mainborder_menu_bkg_height;

	// Top tool bar (max: two rows).
	crd_atlas_mainborder_toolbar_bkg_max_width = crd_atlas_mainborder_toolbar_bkg_width;
	crd_atlas_mainborder_toolbar_bkg_max_height = 35;
	crd_atlas_mainborder_toolbar_bkg_max_x = crd_atlas_mainborder_toolbar_bkg_x;
	crd_atlas_mainborder_toolbar_bkg_max_y = crd_atlas_mainborder_toolbar_bkg_y;

	// Fully Minimise arrow on tool bar.
	crd_atlas_mainborder_toolbar_fully_minimise_arrow_width = 8;
	crd_atlas_mainborder_toolbar_fully_minimise_arrow_height = 4;
	crd_atlas_mainborder_toolbar_fully_minimise_arrow_x = crd_atlas_mainborder_rt_corner_x+crd_atlas_mainborder_rt_corner_width+crd_atlas_mainborder_toolbar_fully_minimise_arrow_width-11;
	crd_atlas_mainborder_toolbar_fully_minimise_arrow_y = crd_atlas_mainborder_toolbar_bkg_y+3;

	// Minimise arrow on tool bar.
	crd_atlas_mainborder_toolbar_minimise_arrow_width = crd_atlas_mainborder_toolbar_fully_minimise_arrow_width;
	crd_atlas_mainborder_toolbar_minimise_arrow_height = crd_atlas_mainborder_toolbar_fully_minimise_arrow_height;
	crd_atlas_mainborder_toolbar_minimise_arrow_x = crd_atlas_mainborder_toolbar_fully_minimise_arrow_x;
	crd_atlas_mainborder_toolbar_minimise_arrow_y = crd_atlas_mainborder_toolbar_bkg_max_y+crd_atlas_mainborder_toolbar_bkg_height-crd_atlas_mainborder_toolbar_minimise_arrow_height-1;

	// Fully Maximise arrow on tool bar.
	crd_atlas_mainborder_toolbar_fully_maximise_arrow_width = crd_atlas_mainborder_toolbar_fully_minimise_arrow_width;
	crd_atlas_mainborder_toolbar_fully_maximise_arrow_height = crd_atlas_mainborder_toolbar_fully_minimise_arrow_height;
	crd_atlas_mainborder_toolbar_fully_maximise_arrow_x = crd_atlas_mainborder_toolbar_fully_minimise_arrow_x;
	crd_atlas_mainborder_toolbar_fully_maximise_arrow_y = crd_atlas_mainborder_toolbar_bkg_y+crd_atlas_mainborder_toolbar_bkg_height-crd_atlas_mainborder_toolbar_fully_maximise_arrow_height-crd_atlas_mainborder_toolbar_fully_maximise_arrow_height-1;

	// Maximise arrow on tool bar.
	crd_atlas_mainborder_toolbar_maximise_arrow_width = crd_atlas_mainborder_toolbar_fully_minimise_arrow_width;
	crd_atlas_mainborder_toolbar_maximise_arrow_height = crd_atlas_mainborder_toolbar_fully_minimise_arrow_height;
	crd_atlas_mainborder_toolbar_maximise_arrow_x = crd_atlas_mainborder_toolbar_fully_minimise_arrow_x;
	crd_atlas_mainborder_toolbar_maximise_arrow_y = crd_atlas_mainborder_menu_bkg_y+crd_atlas_mainborder_menu_bkg_height-crd_atlas_mainborder_toolbar_maximise_arrow_height-crd_atlas_mainborder_toolbar_maximise_arrow_height-1;

	// Toolbar button.
	crd_atlas_mainborder_toolbar_button_span = 4;
	crd_atlas_mainborder_toolbar_button_1_width = 15;
	crd_atlas_mainborder_toolbar_button_1_height = 15;
	crd_atlas_mainborder_toolbar_button_1_x = crd_atlas_mainborder_menu_bkg_x+5;
	crd_atlas_mainborder_toolbar_button_1_y = crd_atlas_mainborder_menu_bkg_y+crd_atlas_mainborder_menu_bkg_height+1;

	crd_atlas_mainborder_toolbar_button_2_width = crd_atlas_mainborder_toolbar_button_1_width;
	crd_atlas_mainborder_toolbar_button_2_height = crd_atlas_mainborder_toolbar_button_1_height;
	crd_atlas_mainborder_toolbar_button_2_x = crd_atlas_mainborder_toolbar_button_1_x+crd_atlas_mainborder_toolbar_button_1_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_2_y = crd_atlas_mainborder_toolbar_button_1_y;

	crd_atlas_mainborder_toolbar_button_3_width = crd_atlas_mainborder_toolbar_button_2_width;
	crd_atlas_mainborder_toolbar_button_3_height = crd_atlas_mainborder_toolbar_button_2_height;
	crd_atlas_mainborder_toolbar_button_3_x = crd_atlas_mainborder_toolbar_button_2_x+crd_atlas_mainborder_toolbar_button_2_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_3_y = crd_atlas_mainborder_toolbar_button_2_y;

	crd_atlas_mainborder_toolbar_button_4_width = crd_atlas_mainborder_toolbar_button_3_width;
	crd_atlas_mainborder_toolbar_button_4_height = crd_atlas_mainborder_toolbar_button_3_height;
	crd_atlas_mainborder_toolbar_button_4_x = crd_atlas_mainborder_toolbar_button_3_x+crd_atlas_mainborder_toolbar_button_3_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_4_y = crd_atlas_mainborder_toolbar_button_3_y;

	crd_atlas_mainborder_toolbar_button_5_width = crd_atlas_mainborder_toolbar_button_4_width;
	crd_atlas_mainborder_toolbar_button_5_height = crd_atlas_mainborder_toolbar_button_4_height;
	crd_atlas_mainborder_toolbar_button_5_x = crd_atlas_mainborder_toolbar_button_4_x+crd_atlas_mainborder_toolbar_button_4_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_5_y = crd_atlas_mainborder_toolbar_button_4_y;

	crd_atlas_mainborder_toolbar_button_6_width = crd_atlas_mainborder_toolbar_button_5_width;
	crd_atlas_mainborder_toolbar_button_6_height = crd_atlas_mainborder_toolbar_button_5_height;
	crd_atlas_mainborder_toolbar_button_6_x = crd_atlas_mainborder_toolbar_button_5_x+crd_atlas_mainborder_toolbar_button_5_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_6_y = crd_atlas_mainborder_toolbar_button_5_y;

	crd_atlas_mainborder_toolbar_button_7_width = crd_atlas_mainborder_toolbar_button_6_width;
	crd_atlas_mainborder_toolbar_button_7_height = crd_atlas_mainborder_toolbar_button_6_height;
	crd_atlas_mainborder_toolbar_button_7_x = crd_atlas_mainborder_toolbar_button_6_x+crd_atlas_mainborder_toolbar_button_6_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_7_y = crd_atlas_mainborder_toolbar_button_6_y;

	crd_atlas_mainborder_toolbar_button_8_width = crd_atlas_mainborder_toolbar_button_7_width;
	crd_atlas_mainborder_toolbar_button_8_height = crd_atlas_mainborder_toolbar_button_7_height;
	crd_atlas_mainborder_toolbar_button_8_x = crd_atlas_mainborder_toolbar_button_7_x+crd_atlas_mainborder_toolbar_button_7_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_8_y = crd_atlas_mainborder_toolbar_button_7_y;

	crd_atlas_mainborder_toolbar_button_9_width = crd_atlas_mainborder_toolbar_button_8_width;
	crd_atlas_mainborder_toolbar_button_9_height = crd_atlas_mainborder_toolbar_button_8_height;
	crd_atlas_mainborder_toolbar_button_9_x = crd_atlas_mainborder_toolbar_button_8_x+crd_atlas_mainborder_toolbar_button_8_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_9_y = crd_atlas_mainborder_toolbar_button_8_y;

	crd_atlas_mainborder_toolbar_button_10_width = crd_atlas_mainborder_toolbar_button_9_width;
	crd_atlas_mainborder_toolbar_button_10_height = crd_atlas_mainborder_toolbar_button_9_height;
	crd_atlas_mainborder_toolbar_button_10_x = crd_atlas_mainborder_toolbar_button_9_x+crd_atlas_mainborder_toolbar_button_9_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_10_y = crd_atlas_mainborder_toolbar_button_9_y;

	crd_atlas_mainborder_toolbar_button_11_width = crd_atlas_mainborder_toolbar_button_10_width;
	crd_atlas_mainborder_toolbar_button_11_height = crd_atlas_mainborder_toolbar_button_10_height;
	crd_atlas_mainborder_toolbar_button_11_x = crd_atlas_mainborder_toolbar_button_10_x+crd_atlas_mainborder_toolbar_button_10_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_11_y = crd_atlas_mainborder_toolbar_button_10_y;

	crd_atlas_mainborder_toolbar_button_12_width = crd_atlas_mainborder_toolbar_button_11_width;
	crd_atlas_mainborder_toolbar_button_12_height = crd_atlas_mainborder_toolbar_button_11_height;
	crd_atlas_mainborder_toolbar_button_12_x = crd_atlas_mainborder_toolbar_button_11_x+crd_atlas_mainborder_toolbar_button_11_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_12_y = crd_atlas_mainborder_toolbar_button_11_y;

	crd_atlas_mainborder_toolbar_button_13_width = crd_atlas_mainborder_toolbar_button_12_width;
	crd_atlas_mainborder_toolbar_button_13_height = crd_atlas_mainborder_toolbar_button_12_height;
	crd_atlas_mainborder_toolbar_button_13_x = crd_atlas_mainborder_toolbar_button_12_x+crd_atlas_mainborder_toolbar_button_12_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_13_y = crd_atlas_mainborder_toolbar_button_12_y;

	crd_atlas_mainborder_toolbar_button_14_width = crd_atlas_mainborder_toolbar_button_13_width;
	crd_atlas_mainborder_toolbar_button_14_height = crd_atlas_mainborder_toolbar_button_13_height;
	crd_atlas_mainborder_toolbar_button_14_x = crd_atlas_mainborder_toolbar_button_13_x+crd_atlas_mainborder_toolbar_button_13_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_14_y = crd_atlas_mainborder_toolbar_button_13_y;

	crd_atlas_mainborder_toolbar_button_15_width = crd_atlas_mainborder_toolbar_button_14_width;
	crd_atlas_mainborder_toolbar_button_15_height = crd_atlas_mainborder_toolbar_button_14_height;
	crd_atlas_mainborder_toolbar_button_15_x = crd_atlas_mainborder_toolbar_button_14_x+crd_atlas_mainborder_toolbar_button_14_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_15_y = crd_atlas_mainborder_toolbar_button_14_y;

	crd_atlas_mainborder_toolbar_button_16_width = crd_atlas_mainborder_toolbar_button_15_width;
	crd_atlas_mainborder_toolbar_button_16_height = crd_atlas_mainborder_toolbar_button_15_height;
	crd_atlas_mainborder_toolbar_button_16_x = crd_atlas_mainborder_toolbar_button_15_x+crd_atlas_mainborder_toolbar_button_15_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_16_y = crd_atlas_mainborder_toolbar_button_15_y;

	crd_atlas_mainborder_toolbar_button_17_width = crd_atlas_mainborder_toolbar_button_16_width;
	crd_atlas_mainborder_toolbar_button_17_height = crd_atlas_mainborder_toolbar_button_16_height;
	crd_atlas_mainborder_toolbar_button_17_x = crd_atlas_mainborder_toolbar_button_16_x+crd_atlas_mainborder_toolbar_button_16_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_17_y = crd_atlas_mainborder_toolbar_button_16_y;

	crd_atlas_mainborder_toolbar_button_18_width = crd_atlas_mainborder_toolbar_button_17_width;
	crd_atlas_mainborder_toolbar_button_18_height = crd_atlas_mainborder_toolbar_button_17_height;
	crd_atlas_mainborder_toolbar_button_18_x = crd_atlas_mainborder_toolbar_button_17_x+crd_atlas_mainborder_toolbar_button_17_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_18_y = crd_atlas_mainborder_toolbar_button_17_y;

	crd_atlas_mainborder_toolbar_button_19_width = crd_atlas_mainborder_toolbar_button_18_width;
	crd_atlas_mainborder_toolbar_button_19_height = crd_atlas_mainborder_toolbar_button_18_height;
	crd_atlas_mainborder_toolbar_button_19_x = crd_atlas_mainborder_toolbar_button_18_x+crd_atlas_mainborder_toolbar_button_18_width+crd_atlas_mainborder_toolbar_button_span;
	crd_atlas_mainborder_toolbar_button_19_y = crd_atlas_mainborder_toolbar_button_18_y;

	// Toolbar button.
	crd_atlas_mainborder_toolbar_button_41_width = crd_atlas_mainborder_toolbar_button_1_width;
	crd_atlas_mainborder_toolbar_button_41_height = crd_atlas_mainborder_toolbar_button_1_height;
	crd_atlas_mainborder_toolbar_button_41_x = crd_atlas_mainborder_toolbar_button_1_x;
	crd_atlas_mainborder_toolbar_button_41_y = crd_atlas_mainborder_toolbar_button_1_y+crd_atlas_mainborder_toolbar_button_1_height;

	// Left-hand selection pane.
	crd_atlas_left_pane_bkg_width = 187;
	crd_atlas_left_pane_bkg_height = 0;
	crd_atlas_left_pane_bkg_x = crd_atlas_mainborder_lt_corner_x;
	crd_atlas_left_pane_bkg_y = crd_atlas_mainborder_toolbar_bkg_y+crd_atlas_mainborder_toolbar_bkg_height;

	// Bottom-right Mini Map Background.
	crd_atlas_minimap_bkg_width = 182;
	crd_atlas_minimap_bkg_height = 182;
	crd_atlas_minimap_bkg_x = 0;
	crd_atlas_minimap_bkg_y = 0;

	// Bottom-right Mini Map.
	crd_atlas_minimap_width = 140;
	crd_atlas_minimap_height = 140;
	crd_atlas_minimap_x = 20;
	crd_atlas_minimap_y = 20;

	// Bottom selection pane.
	crd_atlas_bottom_pane_bkg_width = crd_atlas_left_pane_bkg_x-crd_atlas_left_pane_bkg_width-crd_atlas_minimap_bkg_x-crd_atlas_minimap_bkg_width+6;
	crd_atlas_bottom_pane_bkg_height = 148;
	crd_atlas_bottom_pane_bkg_x = crd_atlas_left_pane_bkg_x+crd_atlas_left_pane_bkg_width-3;
	crd_atlas_bottom_pane_bkg_y = crd_atlas_left_pane_bkg_height;

	// Left-Bottom selection pane corner.
	crd_atlas_lb_corner_width = 20;
	crd_atlas_lb_corner_height = 20;
	crd_atlas_lb_corner_x = crd_atlas_left_pane_bkg_x+crd_atlas_left_pane_bkg_width-3;
	crd_atlas_lb_corner_y = crd_atlas_bottom_pane_bkg_height-4;

	// Right-Bottom selection pane corner.
	crd_atlas_rb_corner_width = (crd_atlas_minimap_bkg_height-crd_atlas_bottom_pane_bkg_height)+3;
	crd_atlas_rb_corner_height = crd_atlas_rb_corner_width;
	crd_atlas_rb_corner_x = crd_atlas_minimap_bkg_width-2;
	crd_atlas_rb_corner_y = crd_atlas_bottom_pane_bkg_height-4;

	// Atlas tooltip window.
	crd_atlas_tooltip_width = crd_atlas_minimap_bkg_width-20;
	crd_atlas_tooltip_height = 82;
	crd_atlas_tooltip_x = crd_atlas_minimap_bkg_x+10;
	crd_atlas_tooltip_y = crd_atlas_minimap_bkg_y+crd_atlas_minimap_bkg_height+4;
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
	crd_atlas_left_pane_bkg_y = crd_atlas_mainborder_menu_bkg_y+crd_atlas_mainborder_menu_bkg_height;
	setSizeCoord("atlas_left_pane_bkg", crd_atlas_left_pane_bkg_x, crd_atlas_left_pane_bkg_y, crd_atlas_left_pane_bkg_x+crd_atlas_left_pane_bkg_width, 0, left_screen, top_screen, left_screen, bottom_screen);
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
	crd_atlas_left_pane_bkg_y = crd_atlas_mainborder_toolbar_bkg_y+crd_atlas_mainborder_toolbar_bkg_height;
	setSizeCoord("atlas_left_pane_bkg", crd_atlas_left_pane_bkg_x, crd_atlas_left_pane_bkg_y, crd_atlas_left_pane_bkg_x+crd_atlas_left_pane_bkg_width, 0, left_screen, top_screen, left_screen, bottom_screen);
}

// ====================================================================

function atlasFullyMaximiseToolbar()
{
	// Extend toolbar to two rows.

	GUIObjectHide("atlas_mainborder_toolbar");
	GUIObjectUnhide("atlas_mainborder_toolbar_max");
	GUIObjectUnhide("atlas_mainborder_toolbar_button_row_1");
	GUIObjectUnhide("atlas_mainborder_toolbar_button_row_2");

	// Set toolbar height.
	crd_atlas_left_pane_bkg_y = crd_atlas_mainborder_toolbar_bkg_max_y+crd_atlas_mainborder_toolbar_bkg_max_height;
	setSizeCoord("atlas_left_pane_bkg", crd_atlas_left_pane_bkg_x, crd_atlas_left_pane_bkg_y, crd_atlas_left_pane_bkg_x+crd_atlas_left_pane_bkg_width, 0, left_screen, top_screen, left_screen, bottom_screen);
}

// ====================================================================

function atlasMaximiseToolbar()
{
	// Extend toolbar to one row.

	GUIObjectUnhide("atlas_mainborder_toolbar");
	GUIObjectHide("atlas_mainborder_toolbar_maximise_arrow");
	GUIObjectUnhide("atlas_mainborder_toolbar_button_row_1");
	GUIObjectHide("atlas_mainborder_toolbar_button_row_2");

	// Set toolbar height.
	crd_atlas_left_pane_bkg_y = crd_atlas_mainborder_toolbar_bkg_y+crd_atlas_mainborder_toolbar_bkg_height;
	setSizeCoord("atlas_left_pane_bkg", crd_atlas_left_pane_bkg_x, crd_atlas_left_pane_bkg_y, crd_atlas_left_pane_bkg_x+crd_atlas_left_pane_bkg_width, 0, left_screen, top_screen, left_screen, bottom_screen);
}

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

function atlasToolbarButton_41()
{
	// Perform action associated with this toolbar button.
}

// ====================================================================