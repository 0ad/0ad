function initSubWindows()
{
	// In-Game Menu background.
	crd_ingame_menu_bkg_x = -150;
	crd_ingame_menu_bkg_y = -200;
	crd_ingame_menu_bkg_width = (crd_ingame_menu_bkg_x * -1) * 2;
	crd_ingame_menu_bkg_height = (crd_ingame_menu_bkg_y * -1) * 2;

	// Border corner lt.
	crd_ingame_menu_border_corner_lt_x = crd_ingame_menu_bkg_x;
	crd_ingame_menu_border_corner_lt_y = crd_ingame_menu_bkg_y;
	crd_ingame_menu_border_corner_lt_width = 15;
	crd_ingame_menu_border_corner_lt_height = crd_ingame_menu_border_corner_lt_width;

	// Border corner top.
	crd_ingame_menu_border_top_x = crd_ingame_menu_border_corner_lt_x + crd_ingame_menu_border_corner_lt_width;
	crd_ingame_menu_border_top_y = crd_ingame_menu_border_corner_lt_y;
	crd_ingame_menu_border_top_width = crd_ingame_menu_bkg_width-crd_ingame_menu_border_corner_lt_width-crd_ingame_menu_border_corner_lt_width;
	crd_ingame_menu_border_top_height = 32;

	// Border corner rt.
	crd_ingame_menu_border_corner_rt_x = crd_ingame_menu_border_top_x+crd_ingame_menu_border_top_width;
	crd_ingame_menu_border_corner_rt_y = crd_ingame_menu_border_corner_lt_y;
	crd_ingame_menu_border_corner_rt_width = crd_ingame_menu_border_corner_lt_width;
	crd_ingame_menu_border_corner_rt_height = crd_ingame_menu_border_corner_lt_height;

	// Border corner left.
	crd_ingame_menu_border_left_x = crd_ingame_menu_border_corner_lt_x;
	crd_ingame_menu_border_left_y = crd_ingame_menu_border_corner_lt_y+crd_ingame_menu_border_corner_lt_height;
	crd_ingame_menu_border_left_width = 20;
	crd_ingame_menu_border_left_height = (crd_ingame_menu_bkg_y+crd_ingame_menu_bkg_height-crd_ingame_menu_border_corner_lt_height)*2;

	// Border corner right.
	crd_ingame_menu_border_right_width = crd_ingame_menu_border_left_width*2;
	crd_ingame_menu_border_right_height = crd_ingame_menu_border_left_height;
	crd_ingame_menu_border_right_x = crd_ingame_menu_bkg_x+crd_ingame_menu_bkg_width-crd_ingame_menu_border_top_height;
	crd_ingame_menu_border_right_y = crd_ingame_menu_border_left_y;

	// Border corner lb.
	crd_ingame_menu_border_corner_lb_x = crd_ingame_menu_border_corner_lt_x;
	crd_ingame_menu_border_corner_lb_y = crd_ingame_menu_bkg_y+crd_ingame_menu_bkg_height-crd_ingame_menu_border_corner_lt_height;
	crd_ingame_menu_border_corner_lb_width = crd_ingame_menu_border_corner_lt_width;
	crd_ingame_menu_border_corner_lb_height = crd_ingame_menu_border_corner_lb_width;

	// Border corner bottom.
	crd_ingame_menu_border_bottom_width = crd_ingame_menu_border_top_width;
	crd_ingame_menu_border_bottom_height = crd_ingame_menu_border_top_height;
	crd_ingame_menu_border_bottom_x = crd_ingame_menu_border_top_x;
	crd_ingame_menu_border_bottom_y = crd_ingame_menu_bkg_y+crd_ingame_menu_bkg_height-crd_ingame_menu_border_bottom_height;

	// Border corner rb.
	crd_ingame_menu_border_corner_rb_x = crd_ingame_menu_border_corner_rt_x;
	crd_ingame_menu_border_corner_rb_y = crd_ingame_menu_border_corner_lb_y;
	crd_ingame_menu_border_corner_rb_width = crd_ingame_menu_border_corner_rt_width;
	crd_ingame_menu_border_corner_rb_height = crd_ingame_menu_border_corner_rt_height;

	// Return button.
	crd_ingame_menu_return_button_width = crd_ingame_menu_border_right_x-crd_ingame_menu_border_left_x-30;
	crd_ingame_menu_return_button_height = 34;
	crd_ingame_menu_return_button_x = crd_ingame_menu_border_left_x+crd_ingame_menu_border_left_width+10;
	crd_ingame_menu_return_button_y = crd_ingame_menu_border_bottom_y+crd_ingame_menu_border_bottom_height-(crd_ingame_menu_return_button_height*2);
	crd_ingame_menu_button_span = 5;

	// Exit button.
	crd_ingame_menu_exit_button_width = crd_ingame_menu_return_button_width;
	crd_ingame_menu_exit_button_height = crd_ingame_menu_return_button_height;
	crd_ingame_menu_exit_button_x = crd_ingame_menu_return_button_x;
	crd_ingame_menu_exit_button_y = crd_ingame_menu_return_button_y-crd_ingame_menu_return_button_height-crd_ingame_menu_button_span;

	// End Game / Resign button.
	crd_ingame_menu_end_button_width = crd_ingame_menu_exit_button_width;
	crd_ingame_menu_end_button_height = crd_ingame_menu_exit_button_height;
	crd_ingame_menu_end_button_x = crd_ingame_menu_exit_button_x;
	crd_ingame_menu_end_button_y = crd_ingame_menu_exit_button_y-crd_ingame_menu_exit_button_height-crd_ingame_menu_button_span;

	// End Game button.
}

// ====================================================================