function initTeamTray()
{
	// Team Tray Button 1.
	crd_team_tray_1_width = crd_portrait_sml_width;
	crd_team_tray_1_height = crd_portrait_sml_height;
	crd_team_tray_1_x = -(crd_team_tray_1_width/1.5);
	crd_team_tray_1_y = crd_map_orb_minimap_height+(crd_team_tray_1_height/2);

	// Team Tray Button 2.
	crd_team_tray_2_width = crd_team_tray_1_width;
	crd_team_tray_2_height = crd_team_tray_1_height;
	crd_team_tray_2_x = crd_team_tray_1_x;
	crd_team_tray_2_y = crd_team_tray_1_y+crd_team_tray_2_height;

	// Team Tray Button 3.
	crd_team_tray_3_width = crd_team_tray_1_width;
	crd_team_tray_3_height = crd_team_tray_1_height;
	crd_team_tray_3_x = crd_team_tray_1_x;
	crd_team_tray_3_y = crd_team_tray_2_y+crd_team_tray_3_height;

	// Team Tray Button 4.
	crd_team_tray_4_width = crd_team_tray_1_width;
	crd_team_tray_4_height = crd_team_tray_1_height;
	crd_team_tray_4_x = crd_team_tray_1_x;
	crd_team_tray_4_y = crd_team_tray_3_y+crd_team_tray_4_height;

	// Team Tray Button 5.
	crd_team_tray_5_width = crd_team_tray_1_width;
	crd_team_tray_5_height = crd_team_tray_1_height;
	crd_team_tray_5_x = crd_team_tray_1_x;
	crd_team_tray_5_y = crd_team_tray_4_y+crd_team_tray_5_height;

	// Team Tray Button 6.
	crd_team_tray_6_width = crd_team_tray_1_width;
	crd_team_tray_6_height = crd_team_tray_1_height;
	crd_team_tray_6_x = crd_team_tray_1_x;
	crd_team_tray_6_y = crd_team_tray_5_y+crd_team_tray_6_height;

	// Team Tray Button 7.
	crd_team_tray_7_width = crd_team_tray_1_width;
	crd_team_tray_7_height = crd_team_tray_1_height;
	crd_team_tray_7_x = crd_team_tray_1_x;
	crd_team_tray_7_y = crd_team_tray_6_y+crd_team_tray_7_height;

	// Team Tray Button 8.
	crd_team_tray_8_width = crd_team_tray_1_width;
	crd_team_tray_8_height = crd_team_tray_1_height;
	crd_team_tray_8_x = crd_team_tray_1_x;
	crd_team_tray_8_y = crd_team_tray_7_y+crd_team_tray_8_height;

	// Team Tray Button 9.
	crd_team_tray_9_width = crd_team_tray_1_width;
	crd_team_tray_9_height = crd_team_tray_1_height;
	crd_team_tray_9_x = crd_team_tray_1_x;
	crd_team_tray_9_y = crd_team_tray_8_y+crd_team_tray_9_height;
}

// ====================================================================

function UpdateTeamTray()
{
	// Enable a Team Tray icon if its group has been created.
	if (groups[1].length > 0) getGUIObjectByName("session_team_tray_1").hidden = false;
	else getGUIObjectByName("session_team_tray_1").hidden = true;
	if (groups[2].length > 0) getGUIObjectByName("session_team_tray_2").hidden = false;
	else getGUIObjectByName("session_team_tray_2").hidden = true;
	if (groups[3].length > 0) getGUIObjectByName("session_team_tray_3").hidden = false;
	else getGUIObjectByName("session_team_tray_3").hidden = true;
	if (groups[4].length > 0) getGUIObjectByName("session_team_tray_4").hidden = false;
	else getGUIObjectByName("session_team_tray_4").hidden = true;
	if (groups[5].length > 0) getGUIObjectByName("session_team_tray_5").hidden = false;
	else getGUIObjectByName("session_team_tray_5").hidden = true;
	if (groups[6].length > 0) getGUIObjectByName("session_team_tray_6").hidden = false;
	else getGUIObjectByName("session_team_tray_6").hidden = true;
	if (groups[7].length > 0) getGUIObjectByName("session_team_tray_7").hidden = false;
	else getGUIObjectByName("session_team_tray_7").hidden = true;
	if (groups[8].length > 0) getGUIObjectByName("session_team_tray_8").hidden = false;
	else getGUIObjectByName("session_team_tray_8").hidden = true;
	if (groups[9].length > 0) getGUIObjectByName("session_team_tray_9").hidden = false;
	else getGUIObjectByName("session_team_tray_9").hidden = true;
}

// ====================================================================