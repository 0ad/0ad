function initSubWindows()
{
	// In-Game Menu background.
	crd_ingame_menu_bkg_x = -100;
	crd_ingame_menu_bkg_y = -150;
	crd_ingame_menu_bkg_width = (crd_ingame_menu_bkg_x * -1) * 2;
	crd_ingame_menu_bkg_height = (crd_ingame_menu_bkg_y * -1) * 2;

	// Return button.
	crd_ingame_menu_return_button_width = crd_ingame_menu_bkg_width;
	crd_ingame_menu_return_button_height = 34;
	crd_ingame_menu_return_button_x = crd_ingame_menu_bkg_x;
	crd_ingame_menu_return_button_y = crd_ingame_menu_bkg_y+crd_ingame_menu_bkg_height-crd_ingame_menu_return_button_height;
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
}

// ====================================================================