function initTeamTray()
{
	SN_TEAM_TRAY = new Array();
	SN_TEAM_TRAY.last = 0;
	SN_TEAM_TRAY.max  = 9;
	for (SN_TEAM_TRAY.curr = 1; SN_TEAM_TRAY.curr <= SN_TEAM_TRAY.max; SN_TEAM_TRAY.curr++)
	{
		SN_TEAM_TRAY[SN_TEAM_TRAY.curr] = addArrayElement(Crd, Crd.last); 
		Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
		Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
		Crd[Crd.last-1].width	= crd_portrait_sml_width; 
		Crd[Crd.last-1].height	= crd_portrait_sml_height; 
		Crd[Crd.last-1].x	= -(Crd[Crd.last-1].width/2);

		if (SN_TEAM_TRAY.curr == 1)
			Crd[Crd.last-1].y	= Crd[SN_MINIMAP].height+(Crd[Crd.last-1].height/2); 
		else
			Crd[Crd.last-1].y	= Crd[Crd.last-2].y+Crd[Crd.last-2].width;
	}
	SN_TEAM_TRAY.last = SN_TEAM_TRAY.curr;
}

// ====================================================================

function UpdateTeamTray()
{
	// Enable a Team Tray icon if its group has been created.
	for (SN_TEAM_TRAY.curr = 1; SN_TEAM_TRAY.curr < SN_TEAM_TRAY.last; SN_TEAM_TRAY.curr++)
	{					
		if (groups[SN_TEAM_TRAY.curr].length > 0)
			getGUIObjectByName("SN_TEAM_TRAY_" + SN_TEAM_TRAY.curr).hidden = false;
		else
			getGUIObjectByName("SN_TEAM_TRAY_" + SN_TEAM_TRAY.curr).hidden = true;
	}
}

// ====================================================================