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
	crd_atlas_mainborder_rt_corner_x = -crd_atlas_mainborder_rt_corner_width;
	crd_atlas_mainborder_rt_corner_y = crd_atlas_mainborder_lt_corner_y;

	// Info window in top-right corner.
	crd_atlas_info_window_width = crd_atlas_mainborder_rt_corner_width-30-6;
	crd_atlas_info_window_height = crd_atlas_mainborder_rt_corner_height-9;
	crd_atlas_info_window_x = -crd_atlas_info_window_width-3;
	crd_atlas_info_window_y = crd_atlas_mainborder_rt_corner_y+3;

	// Top menu bar.
	crd_atlas_mainborder_menu_bkg_width = crd_atlas_mainborder_rt_corner_width;
	crd_atlas_mainborder_menu_bkg_height = 19;
	crd_atlas_mainborder_menu_bkg_x = crd_atlas_mainborder_lt_corner_x+crd_atlas_mainborder_lt_corner_width;
	crd_atlas_mainborder_menu_bkg_y = crd_atlas_mainborder_lt_corner_y;

	// Top tool bar.
	crd_atlas_mainborder_toolbar_bkg_width = crd_atlas_mainborder_rt_corner_width-crd_atlas_mainborder_lt_corner_width;
	crd_atlas_mainborder_toolbar_bkg_height = 20;
	crd_atlas_mainborder_toolbar_bkg_x = crd_atlas_mainborder_lt_corner_x;
	crd_atlas_mainborder_toolbar_bkg_y = crd_atlas_mainborder_menu_bkg_y+crd_atlas_mainborder_menu_bkg_height;

	// Left-hand selection pane.
	crd_atlas_left_pane_bkg_width = 187;
	crd_atlas_left_pane_bkg_height = crd_atlas_mainborder_toolbar_bkg_y-crd_atlas_mainborder_toolbar_bkg_height;
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
	getGUIObjectByName("atlas_info_window").caption = "File: something.map\nOwner: Someone\nFPS: " + getFPS();
}

// ====================================================================