// Main Pregame JS Script file
// Contains functions and code for Main Menu, Session Creation etc.

// ====================================================================

function initPreGame()
{
	initIPHost();
}

// ====================================================================

function initIPHost()
{
	// In-Game Menu background.
	crd_pregame_iphost_bkg_x = -150;
	crd_pregame_iphost_bkg_y = -200;
	crd_pregame_iphost_bkg_width = (crd_pregame_iphost_bkg_x * -1) * 2;
	crd_pregame_iphost_bkg_height = (crd_pregame_iphost_bkg_y * -1) * 2;

	// Border corner lt.
	crd_pregame_iphost_border_corner_lt_x = crd_pregame_iphost_bkg_x;
	crd_pregame_iphost_border_corner_lt_y = crd_pregame_iphost_bkg_y;
	crd_pregame_iphost_border_corner_lt_width = 15;
	crd_pregame_iphost_border_corner_lt_height = crd_pregame_iphost_border_corner_lt_width;

	// Border corner top.
	crd_pregame_iphost_border_top_x = crd_pregame_iphost_border_corner_lt_x + crd_pregame_iphost_border_corner_lt_width;
	crd_pregame_iphost_border_top_y = crd_pregame_iphost_border_corner_lt_y;
	crd_pregame_iphost_border_top_width = crd_pregame_iphost_bkg_width-crd_pregame_iphost_border_corner_lt_width-crd_pregame_iphost_border_corner_lt_width;
	crd_pregame_iphost_border_top_height = 32;

	// Border corner rt.
	crd_pregame_iphost_border_corner_rt_x = crd_pregame_iphost_border_top_x+crd_pregame_iphost_border_top_width;
	crd_pregame_iphost_border_corner_rt_y = crd_pregame_iphost_border_corner_lt_y;
	crd_pregame_iphost_border_corner_rt_width = crd_pregame_iphost_border_corner_lt_width;
	crd_pregame_iphost_border_corner_rt_height = crd_pregame_iphost_border_corner_lt_height;

	// Border corner left.
	crd_pregame_iphost_border_left_x = crd_pregame_iphost_border_corner_lt_x;
	crd_pregame_iphost_border_left_y = crd_pregame_iphost_border_corner_lt_y+crd_pregame_iphost_border_corner_lt_height;
	crd_pregame_iphost_border_left_width = 20;
	crd_pregame_iphost_border_left_height = (crd_pregame_iphost_bkg_y+crd_pregame_iphost_bkg_height-crd_pregame_iphost_border_corner_lt_height)*2;

	// Border corner right.
	crd_pregame_iphost_border_right_width = crd_pregame_iphost_border_left_width*2;
	crd_pregame_iphost_border_right_height = crd_pregame_iphost_border_left_height;
	crd_pregame_iphost_border_right_x = crd_pregame_iphost_bkg_x+crd_pregame_iphost_bkg_width-crd_pregame_iphost_border_top_height;
	crd_pregame_iphost_border_right_y = crd_pregame_iphost_border_left_y;

	// Border corner lb.
	crd_pregame_iphost_border_corner_lb_x = crd_pregame_iphost_border_corner_lt_x;
	crd_pregame_iphost_border_corner_lb_y = crd_pregame_iphost_bkg_y+crd_pregame_iphost_bkg_height-crd_pregame_iphost_border_corner_lt_height;
	crd_pregame_iphost_border_corner_lb_width = crd_pregame_iphost_border_corner_lt_width;
	crd_pregame_iphost_border_corner_lb_height = crd_pregame_iphost_border_corner_lb_width;

	// Border corner bottom.
	crd_pregame_iphost_border_bottom_width = crd_pregame_iphost_border_top_width;
	crd_pregame_iphost_border_bottom_height = crd_pregame_iphost_border_top_height;
	crd_pregame_iphost_border_bottom_x = crd_pregame_iphost_border_top_x;
	crd_pregame_iphost_border_bottom_y = crd_pregame_iphost_bkg_y+crd_pregame_iphost_bkg_height-crd_pregame_iphost_border_bottom_height;

	// Border corner rb.
	crd_pregame_iphost_border_corner_rb_x = crd_pregame_iphost_border_corner_rt_x;
	crd_pregame_iphost_border_corner_rb_y = crd_pregame_iphost_border_corner_lb_y;
	crd_pregame_iphost_border_corner_rb_width = crd_pregame_iphost_border_corner_rt_width;
	crd_pregame_iphost_border_corner_rb_height = crd_pregame_iphost_border_corner_rt_height;
}

// ====================================================================