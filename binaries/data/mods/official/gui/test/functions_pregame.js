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
	// IP Host Window background.
	crd_pregame_iphost_bkg_x = -350;
	crd_pregame_iphost_bkg_y = -200;
	crd_pregame_iphost_bkg_width = (crd_pregame_iphost_bkg_x * -1) * 2;
	crd_pregame_iphost_bkg_height = (crd_pregame_iphost_bkg_y * -1) * 2;

	// IP Host Window exit button.
	crd_pregame_iphost_exit_button_width = 16;
	crd_pregame_iphost_exit_button_height = crd_pregame_iphost_exit_button_width;
	crd_pregame_iphost_exit_button_x = crd_pregame_iphost_bkg_x+crd_pregame_iphost_bkg_width+10;
	crd_pregame_iphost_exit_button_y = crd_pregame_iphost_bkg_y-25;

	// IP Host Window titlebar.
	crd_pregame_iphost_titlebar_width = crd_pregame_iphost_bkg_width;
	crd_pregame_iphost_titlebar_height = 16;
	crd_pregame_iphost_titlebar_x = crd_pregame_iphost_bkg_x;
	crd_pregame_iphost_titlebar_y = crd_pregame_iphost_bkg_y-25;
}

// ====================================================================