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
	crd_status_orb_portrait_height = crd_portrait_lrg_height;

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

	// Attack.
	crd_status_orb_attack_width = crd_status_orb_bkg_width;
	crd_status_orb_attack_height = ((crd_status_orb_name1_y)-(crd_status_orb_xpbar_y+crd_status_orb_xpbar_height))/2;
	crd_status_orb_attack_x = crd_status_orb_name3_x;
	crd_status_orb_attack_y = crd_status_orb_xpbar_y+crd_status_orb_xpbar_height-1;

	// Armour.
	crd_status_orb_armour_width = crd_status_orb_attack_width;
	crd_status_orb_armour_height = crd_status_orb_attack_height;
	crd_status_orb_armour_x = crd_status_orb_attack_x;
	crd_status_orb_armour_y = crd_status_orb_attack_y+crd_status_orb_attack_height;

	// Command Button 1.
	crd_status_orb_command_1_width = crd_portrait_sml_width;
	crd_status_orb_command_1_height = crd_portrait_sml_height;
	crd_status_orb_command_1_x = 0;
	crd_status_orb_command_1_y = crd_status_orb_bkg_height;

	// Command Button 2.
	crd_status_orb_command_2_width = crd_status_orb_command_1_width;
	crd_status_orb_command_2_height = crd_status_orb_command_1_height;
	crd_status_orb_command_2_x = crd_status_orb_command_1_x+crd_status_orb_command_1_width;
	crd_status_orb_command_2_y = crd_status_orb_command_1_y;

	// Command Button 3.
	crd_status_orb_command_3_width = crd_status_orb_command_1_width;
	crd_status_orb_command_3_height = crd_status_orb_command_1_height;
	crd_status_orb_command_3_x = crd_status_orb_command_2_x+crd_status_orb_command_2_width;
	crd_status_orb_command_3_y = crd_status_orb_command_2_y;

	// Command Button 4.
	crd_status_orb_command_4_width = crd_status_orb_command_1_width;
	crd_status_orb_command_4_height = crd_status_orb_command_1_height;
	crd_status_orb_command_4_x = crd_status_orb_command_3_x+crd_status_orb_command_3_width;
	crd_status_orb_command_4_y = crd_status_orb_command_3_y-3;

	// Command Button 5.
	crd_status_orb_command_5_width = crd_status_orb_command_1_width;
	crd_status_orb_command_5_height = crd_status_orb_command_1_height;
	crd_status_orb_command_5_x = crd_status_orb_command_4_x+crd_status_orb_command_4_width-1;
	crd_status_orb_command_5_y = crd_status_orb_command_4_y-10;

	// Command Button 6.
	crd_status_orb_command_6_width = crd_status_orb_command_1_width;
	crd_status_orb_command_6_height = crd_status_orb_command_1_height;
	crd_status_orb_command_6_x = crd_status_orb_command_5_x+crd_status_orb_command_5_width-7;
	crd_status_orb_command_6_y = crd_status_orb_command_5_y-20;

	// Command Button 7.
	crd_status_orb_command_7_width = crd_status_orb_command_1_width;
	crd_status_orb_command_7_height = crd_status_orb_command_1_height;
	crd_status_orb_command_7_x = crd_status_orb_command_6_x+crd_status_orb_command_6_width-12;
	crd_status_orb_command_7_y = crd_status_orb_command_6_y-25;

	// Command Button 8.
	crd_status_orb_command_8_width = crd_status_orb_command_1_width;
	crd_status_orb_command_8_height = crd_status_orb_command_1_height;
	crd_status_orb_command_8_x = crd_status_orb_command_7_x+crd_status_orb_command_7_width-21;
	crd_status_orb_command_8_y = crd_status_orb_command_7_y-31;

	// Command Button 9.
	crd_status_orb_command_9_width = crd_status_orb_command_1_width;
	crd_status_orb_command_9_height = crd_status_orb_command_1_height;
	crd_status_orb_command_9_x = crd_status_orb_command_8_x+crd_status_orb_command_8_width-32;
	crd_status_orb_command_9_y = crd_status_orb_command_8_y-33;

	// Command Button 10.
	crd_status_orb_command_10_width = crd_status_orb_command_1_width;
	crd_status_orb_command_10_height = crd_status_orb_command_1_height;
	crd_status_orb_command_10_x = crd_status_orb_command_9_x+crd_status_orb_command_9_width-41;
	crd_status_orb_command_10_y = crd_status_orb_command_9_y-32;

	// Command Button 11.
	crd_status_orb_command_11_width = crd_status_orb_command_1_width;
	crd_status_orb_command_11_height = crd_status_orb_command_1_height;
	crd_status_orb_command_11_x = crd_status_orb_command_10_x+crd_status_orb_command_10_width-52;
	crd_status_orb_command_11_y = crd_status_orb_command_10_y-28;
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
	if (selection[0].traits.up.rank > 1)
	{
		getGUIObjectByName("session_panel_status_icon_rank").sprite = "statistic_rank" + (selection[0].traits.up.rank-1);
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
	if (selection[0].traits.up && selection[0].traits.up.curr && selection[0].traits.up.req)
	{
		getGUIObjectByName("session_panel_status_icon_xp_text").caption = Math.round(selection[0].traits.up.curr) + "/" + Math.round(selection[0].traits.up.req);
		getGUIObjectByName("session_panel_status_icon_xp_text").hidden = false;
		getGUIObjectByName("session_panel_status_icon_xp_bar").caption = ((Math.round(selection[0].traits.up.curr) * 100 ) / Math.round(selection[0].traits.up.req));
		getGUIObjectByName("session_panel_status_icon_xp_bar").hidden = false;
	}
	else
	{
		getGUIObjectByName("session_panel_status_icon_xp_text").hidden = true;
		getGUIObjectByName("session_panel_status_icon_xp_bar").hidden = true;
	}

	// Update stats
	if (selection[0].actions.attack && selection[0].actions.attack.damage && selection[0].actions.attack.damage > 0 && selection[0].actions.attack.crush && selection[0].actions.attack.hack && selection[0].actions.attack.pierce)
	{
		getGUIObjectByName("session_panel_status_attack").caption = "[icon=icon_statistic_attack]" + selection[0].actions.attack.damage + " [icon=icon_statistic_hack]" + Math.round(selection[0].actions.attack.hack*100) + "[icon=icon_statistic_pierce]" + Math.round(selection[0].actions.attack.pierce*100) + "[icon=icon_statistic_crush]" + Math.round(selection[0].actions.attack.crush*100) + "";
	}
	else
		getGUIObjectByName("session_panel_status_attack").caption = "";

	if (selection[0].traits.armour && selection[0].traits.armour.value && selection[0].traits.armour.value > 0 && selection[0].traits.armour.crush && selection[0].traits.armour.hack && selection[0].traits.armour.pierce)
	{
		getGUIObjectByName("session_panel_status_armour").caption = "[icon=icon_statistic_armour]" + selection[0].traits.armour.value + " [icon=icon_statistic_hack]" + Math.round(selection[0].traits.armour.hack*100) + "[icon=icon_statistic_pierce]" + Math.round(selection[0].traits.armour.pierce*100) + "[icon=icon_statistic_crush]" + Math.round(selection[0].traits.armour.crush*100) + "";
	}
	else
		getGUIObjectByName("session_panel_status_armour").caption = "";

	// Reveal Status Orb
	getGUIObjectByName("session_status_orb").hidden = false;
}
