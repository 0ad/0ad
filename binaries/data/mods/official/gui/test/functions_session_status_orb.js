function UpdateStatusOrb()
{
	// Update name text.
	// Personal name.
	if (selection[0].traits.id.personal && selection[0].traits.id.personal != "")
	{
		GUIObject = getGUIObjectByName("session_panel_status_name1");
		GUIObject.caption = selection[0].traits.id.personal + "\n";
	}
	else{
		GUIObject = getGUIObjectByName("session_panel_status_name1");
	}
	// Generic name.
	if (selection[0].traits.id.generic)
	{
		GUIObject = getGUIObjectByName("session_panel_status_name2");
		GUIObject.caption = selection[0].traits.id.generic + "\n";
	}
	else{
		GUIObject = getGUIObjectByName("session_panel_status_name2");
		GUIObject.caption = "";
	}
	// Specific/ranked name.
	if (selection[0].traits.id.ranked)
	{
		GUIObject = getGUIObjectByName("session_panel_status_name3");
		GUIObject.caption = selection[0].traits.id.ranked + "\n";
	}
	else{
		if (selection[0].traits.id.specific)
		{
			GUIObject = getGUIObjectByName("session_panel_status_name3");
			GUIObject.caption = selection[0].traits.id.specific + "\n";
		}
	}

	// Update portrait
	if (selection[0].traits.id.icon)
	{
		if (selection[0].traits.id.icon_cell)
			setPortrait("session_panel_status_portrait", selection[0].traits.id.icon + "_" + selection[0].traits.id.icon_cell);
		else
			setPortrait("session_panel_status_portrait", selection[0].traits.id.icon);
	}

	// Update hitpoints
	if (selection[0].traits.health.curr & selection[0].traits.health.hitpoints)
	{
		getGUIObjectByName("session_panel_status_icon_hp_text").caption = Math.round(selection[0].traits.health.curr) + "/" + Math.round(selection[0].traits.health.hitpoints);
		getGUIObjectByName("session_panel_status_icon_hp_bar").caption = ((Math.round(selection[0].traits.health.curr) * 100 ) / Math.round(selection[0].traits.health.hitpoints));
	}

	// Reveal Status Orb
	getGUIObjectByName("session_status_orb").hidden = false;
}
