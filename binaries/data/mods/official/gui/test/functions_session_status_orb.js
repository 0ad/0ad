function initStatusOrb()
{
	// Status Orb background.
	crd_status_orb_bkg_x = 0;
	crd_status_orb_bkg_y = 0;
	crd_status_orb_bkg_width = 184;
	crd_status_orb_bkg_height = crd_status_orb_bkg_width;

	// Status Orb large portrait.
	crd_status_orb_portrait_x = 7;
	crd_status_orb_portrait_y = 6;
	crd_status_orb_portrait_width = crd_portrait_lrg_width;
	crd_status_orb_portrait_height = crd_status_orb_portrait_width;

	// Status Orb rank icon.
	crd_status_orb_rank_width = crd_mini_icon_width;
	crd_status_orb_rank_height = crd_mini_icon_width;
	crd_status_orb_rank_x = crd_status_orb_portrait_x+crd_status_orb_portrait_width-crd_status_orb_rank_width;
	crd_status_orb_rank_y = crd_status_orb_portrait_y;

	// Status Orb health bar.
	crd_status_orb_hpbar_span = 2;
	crd_status_orb_hpbar_x = crd_status_orb_portrait_x;
	crd_status_orb_hpbar_y = crd_status_orb_portrait_y+crd_status_orb_portrait_height+crd_status_orb_hpbar_span;
	crd_status_orb_hpbar_width = crd_status_orb_portrait_width;
	crd_status_orb_hpbar_height = 6;

	// Status Orb health text.
	crd_status_orb_hpbar_text_span_x = 4;
	crd_status_orb_hpbar_text_span_y = 0;
	crd_status_orb_hpbar_text_x = crd_status_orb_hpbar_x+crd_status_orb_hpbar_width+crd_status_orb_hpbar_text_span_x;
	crd_status_orb_hpbar_text_y = crd_status_orb_hpbar_y+crd_status_orb_hpbar_text_span_y;
	crd_status_orb_hpbar_text_width = 100;
	crd_status_orb_hpbar_text_height = crd_status_orb_hpbar_height;

	// Status Orb xp bar.
	crd_status_orb_xpbar_x = crd_status_orb_hpbar_x;
	crd_status_orb_xpbar_y = crd_status_orb_hpbar_y+crd_status_orb_hpbar_height+crd_status_orb_hpbar_span+1;
	crd_status_orb_xpbar_width = crd_status_orb_hpbar_width;
	crd_status_orb_xpbar_height = crd_status_orb_hpbar_height;

	// Status Orb xp text.
	crd_status_orb_xpbar_text_x = crd_status_orb_xpbar_x+crd_status_orb_xpbar_width+crd_status_orb_hpbar_text_span_x;
	crd_status_orb_xpbar_text_y = crd_status_orb_xpbar_y+crd_status_orb_hpbar_text_span_y;
	crd_status_orb_xpbar_text_width = crd_status_orb_hpbar_text_width;
	crd_status_orb_xpbar_text_height = crd_status_orb_hpbar_text_height;

	// Garrison counter.
	crd_status_orb_garrison_span_x = 5;
	crd_status_orb_garrison_span_y = 2;
	crd_status_orb_garrison_x = crd_status_orb_portrait_x+crd_status_orb_portrait_width+crd_status_orb_garrison_span_x;
	crd_status_orb_garrison_y = crd_status_orb_portrait_y+crd_status_orb_garrison_span_y;
	crd_status_orb_garrison_width = 65;
	crd_status_orb_garrison_height = 30;

	// Supply counter.
	crd_status_orb_supply_x = crd_status_orb_garrison_x;
	crd_status_orb_supply_y = crd_status_orb_garrison_y+crd_status_orb_garrison_height-2;
	crd_status_orb_supply_width = crd_status_orb_garrison_width+20;
	crd_status_orb_supply_height = crd_status_orb_garrison_height;

	// Name3.
	crd_status_orb_name3_width = 252;
	crd_status_orb_name3_height = 14;
	crd_status_orb_name3_x = crd_status_orb_bkg_x+2;
	crd_status_orb_name3_y = crd_status_orb_bkg_y+crd_status_orb_bkg_height-crd_status_orb_name3_height-8;

	// Name2.
	crd_status_orb_name2_width = crd_status_orb_name3_width;
	crd_status_orb_name2_height = crd_status_orb_name3_height+1;
	crd_status_orb_name2_x = crd_status_orb_name3_x;
	crd_status_orb_name2_y = crd_status_orb_name3_y-crd_status_orb_name2_height;

	// Name1.
	crd_status_orb_name1_width = crd_status_orb_name2_width;
	crd_status_orb_name1_height = crd_status_orb_name2_height;
	crd_status_orb_name1_x = crd_status_orb_name2_x;
	crd_status_orb_name1_y = crd_status_orb_name2_y-crd_status_orb_name1_height+1;

	// Armour.
	crd_status_orb_armour_width = crd_status_orb_bkg_width;
	crd_status_orb_armour_height = (crd_status_orb_name1_y)-(crd_status_orb_xpbar_y+crd_status_orb_xpbar_height)-10;
	crd_status_orb_armour_x = crd_status_orb_name3_x;
	crd_status_orb_armour_y = crd_status_orb_xpbar_y+crd_status_orb_xpbar_height+10;
}

// ====================================================================

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

	// Update rank.
	if (selection[0].traits.id.rank > 1)
	{
		getGUIObjectByName("session_panel_status_icon_rank").sprite = "statistic_rank" + (selection[0].traits.id.rank-1);
	}
	else
		getGUIObjectByName("session_panel_status_icon_rank").sprite = "";

	// Update hitpoints
	if (selection[0].traits.health.curr && selection[0].traits.health.hitpoints)
	{
		getGUIObjectByName("session_panel_status_icon_hp_text").caption = Math.round(selection[0].traits.health.curr) + "/" + Math.round(selection[0].traits.health.hitpoints);
		getGUIObjectByName("session_panel_status_icon_hp_text").hidden = false;
		getGUIObjectByName("session_panel_status_icon_hp_bar").caption = ((Math.round(selection[0].traits.health.curr) * 100 ) / Math.round(selection[0].traits.health.hitpoints));
		getGUIObjectByName("session_panel_status_icon_hp_bar").hidden = false;
	}
	else
	{
		getGUIObjectByName("session_panel_status_icon_hp_text").hidden = true;
		getGUIObjectByName("session_panel_status_icon_hp_bar").hidden = true;
	}

	// Update upgrade points
	if (selection[0].traits.transform && selection[0].traits.transform.upcurr && selection[0].traits.transform.upreq)
	{
		getGUIObjectByName("session_panel_status_icon_xp_text").caption = Math.round(selection[0].traits.transform.upcurr) + "/" + Math.round(selection[0].traits.transform.upreq);
		getGUIObjectByName("session_panel_status_icon_xp_text").hidden = false;
		getGUIObjectByName("session_panel_status_icon_xp_bar").caption = ((Math.round(selection[0].traits.transform.upcurr) * 100 ) / Math.round(selection[0].traits.transform.upreq));
		getGUIObjectByName("session_panel_status_icon_xp_bar").hidden = false;
	}
	else
	{
		getGUIObjectByName("session_panel_status_icon_xp_text").hidden = true;
		getGUIObjectByName("session_panel_status_icon_xp_bar").hidden = true;
	}

	// Reveal Status Orb
	getGUIObjectByName("session_status_orb").hidden = false;
}
