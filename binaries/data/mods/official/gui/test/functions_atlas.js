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

	// Top menu bar.
	crd_atlas_mainborder_menu_bkg_width = crd_atlas_mainborder_rt_corner_width;
	crd_atlas_mainborder_menu_bkg_height = 19;
	crd_atlas_mainborder_menu_bkg_x = crd_atlas_mainborder_lt_corner_x+crd_atlas_mainborder_lt_corner_width;
	crd_atlas_mainborder_menu_bkg_y = crd_atlas_mainborder_lt_corner_y;

	// Top tool bar.
	crd_atlas_mainborder_toolbar_bkg_width = crd_atlas_mainborder_rt_corner_width-crd_atlas_mainborder_lt_corner_width;
	crd_atlas_mainborder_toolbar_bkg_height = 22;
	crd_atlas_mainborder_toolbar_bkg_x = crd_atlas_mainborder_lt_corner_x;
	crd_atlas_mainborder_toolbar_bkg_y = crd_atlas_mainborder_menu_bkg_y+crd_atlas_mainborder_menu_bkg_height;

	// Bottom-right Mini Map Background.
	crd_atlas_minimap_bkg_width = 180;
	crd_atlas_minimap_bkg_height = 180;
	crd_atlas_minimap_bkg_x = 0;
	crd_atlas_minimap_bkg_y = 0;

	// Bottom-right Mini Map.
	crd_atlas_minimap_width = 140;
	crd_atlas_minimap_height = 140;
	crd_atlas_minimap_x = 20;
	crd_atlas_minimap_y = 20;
}

// ====================================================================