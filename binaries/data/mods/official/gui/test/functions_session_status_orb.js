function initStatusOrb()
{
	// Status Orb background.
	crd_status_orb_bkg_x = 0;
	crd_status_orb_bkg_y = 0;
	crd_status_orb_bkg_width = 255;
	crd_status_orb_bkg_height = 170;

	// Heading.
	crd_status_orb_heading_width = crd_status_orb_bkg_width;
	crd_status_orb_heading_height = 14;
	crd_status_orb_heading_x = crd_status_orb_bkg_x+2;
	crd_status_orb_heading_y = crd_status_orb_bkg_y+3;

	// Status Orb large portrait.
	crd_status_orb_portrait_x = crd_status_orb_heading_x+5;
	crd_status_orb_portrait_y = crd_status_orb_heading_y+crd_status_orb_heading_height+7;
	crd_status_orb_portrait_width = crd_portrait_lrg_width;
	crd_status_orb_portrait_height = crd_portrait_lrg_height;

	// Status Orb rank icon.
	crd_status_orb_rank_width = crd_mini_icon_width;
	crd_status_orb_rank_height = crd_mini_icon_width;
	crd_status_orb_rank_x = crd_status_orb_portrait_x+crd_status_orb_portrait_width-crd_status_orb_rank_width;
	crd_status_orb_rank_y = crd_status_orb_portrait_y;

	// Name1.
	crd_status_orb_name1_width = crd_status_orb_bkg_width-crd_status_orb_portrait_width-10;
	crd_status_orb_name1_height = crd_status_orb_portrait_height;
	crd_status_orb_name1_x = crd_status_orb_portrait_x+crd_status_orb_portrait_width+2;
	crd_status_orb_name1_y = crd_status_orb_portrait_y;

	// Status Orb health bar.
	crd_status_orb_hpbar_span = 2;
	crd_status_orb_hpbar_x = crd_status_orb_portrait_x;
	crd_status_orb_hpbar_y = crd_status_orb_portrait_y+crd_status_orb_portrait_height+crd_status_orb_hpbar_span;
	crd_status_orb_hpbar_width = crd_status_orb_portrait_width;
	crd_status_orb_hpbar_height = 6;

	// Status Orb health text.
	crd_status_orb_hpbar_text_span_x = 4;
	crd_status_orb_hpbar_text_span_y = 0;
	crd_status_orb_hpbar_text_x = crd_status_orb_name1_x;
	crd_status_orb_hpbar_text_y = crd_status_orb_hpbar_y+crd_status_orb_hpbar_text_span_y;
	crd_status_orb_hpbar_text_width = 55;
	crd_status_orb_hpbar_text_height = crd_status_orb_hpbar_height;

	// Status Orb xp bar.
	crd_status_orb_xpbar_x = crd_status_orb_hpbar_x;
	crd_status_orb_xpbar_y = crd_status_orb_hpbar_y+crd_status_orb_hpbar_height+crd_status_orb_hpbar_span+1;
	crd_status_orb_xpbar_width = crd_status_orb_hpbar_width;
	crd_status_orb_xpbar_height = crd_status_orb_hpbar_height;

	// Status Orb xp text.
	crd_status_orb_xpbar_text_x = crd_status_orb_hpbar_text_x;
	crd_status_orb_xpbar_text_y = crd_status_orb_xpbar_y+crd_status_orb_hpbar_text_span_y;
	crd_status_orb_xpbar_text_width = crd_status_orb_hpbar_text_width;
	crd_status_orb_xpbar_text_height = crd_status_orb_hpbar_text_height;

	// Garrison/Supply counter.
	crd_status_orb_stat1_span_x = 5;
	crd_status_orb_stat1_span_y = 2;
	crd_status_orb_stat1_width = 126;
	crd_status_orb_stat1_height = 30;
	crd_status_orb_stat1_x = crd_status_orb_hpbar_text_x+crd_status_orb_hpbar_text_width;
	crd_status_orb_stat1_y = crd_status_orb_hpbar_y-crd_mini_icon_width+1;

	// Stats.

	crd_status_orb_stat1_1_x = crd_status_orb_portrait_x;
	crd_status_orb_stat1_1_y = crd_status_orb_xpbar_y+crd_status_orb_xpbar_height+2;
	crd_status_orb_stat1_1_width = (crd_status_orb_bkg_width-14)/6;
	crd_status_orb_stat1_1_height = (crd_status_orb_bkg_y+crd_status_orb_bkg_height-8-crd_status_orb_stat1_1_y)/2;

	crd_status_orb_stat2_1_width = crd_status_orb_stat1_1_width;
	crd_status_orb_stat2_1_height = crd_status_orb_stat1_1_height;
	crd_status_orb_stat2_1_x = crd_status_orb_stat1_1_x+crd_status_orb_stat1_1_width;
	crd_status_orb_stat2_1_y = crd_status_orb_stat1_1_y;

	crd_status_orb_stat3_1_width = crd_status_orb_stat1_1_width;
	crd_status_orb_stat3_1_height = crd_status_orb_stat1_1_height;
	crd_status_orb_stat3_1_x = crd_status_orb_stat2_1_x+crd_status_orb_stat2_1_width;
	crd_status_orb_stat3_1_y = crd_status_orb_stat1_1_y;

	crd_status_orb_stat4_1_width = crd_status_orb_stat1_1_width;
	crd_status_orb_stat4_1_height = crd_status_orb_stat1_1_height;
	crd_status_orb_stat4_1_x = crd_status_orb_stat3_1_x+crd_status_orb_stat3_1_width;
	crd_status_orb_stat4_1_y = crd_status_orb_stat1_1_y;

	crd_status_orb_stat5_1_width = crd_status_orb_stat1_1_width;
	crd_status_orb_stat5_1_height = crd_status_orb_stat1_1_height;
	crd_status_orb_stat5_1_x = crd_status_orb_stat4_1_x+crd_status_orb_stat4_1_width;
	crd_status_orb_stat5_1_y = crd_status_orb_stat1_1_y;

	crd_status_orb_stat6_1_width = crd_status_orb_stat1_1_width;
	crd_status_orb_stat6_1_height = crd_status_orb_stat1_1_height;
	crd_status_orb_stat6_1_x = crd_status_orb_stat5_1_x+crd_status_orb_stat5_1_width;
	crd_status_orb_stat6_1_y = crd_status_orb_stat1_1_y;

	crd_status_orb_stat1_2_width = crd_status_orb_stat1_1_width;
	crd_status_orb_stat1_2_height = crd_status_orb_stat1_1_height;
	crd_status_orb_stat1_2_x = crd_status_orb_stat1_1_x;
	crd_status_orb_stat1_2_y = crd_status_orb_stat1_1_y+crd_status_orb_stat1_1_height;

	crd_status_orb_stat2_2_width = crd_status_orb_stat1_2_width;
	crd_status_orb_stat2_2_height = crd_status_orb_stat1_2_height;
	crd_status_orb_stat2_2_x = crd_status_orb_stat1_2_x+crd_status_orb_stat1_2_width;
	crd_status_orb_stat2_2_y = crd_status_orb_stat1_2_y;

	crd_status_orb_stat3_2_width = crd_status_orb_stat1_2_width;
	crd_status_orb_stat3_2_height = crd_status_orb_stat1_2_height;
	crd_status_orb_stat3_2_x = crd_status_orb_stat2_2_x+crd_status_orb_stat2_2_width;
	crd_status_orb_stat3_2_y = crd_status_orb_stat1_2_y;

	crd_status_orb_stat4_2_width = crd_status_orb_stat1_2_width;
	crd_status_orb_stat4_2_height = crd_status_orb_stat1_2_height;
	crd_status_orb_stat4_2_x = crd_status_orb_stat3_2_x+crd_status_orb_stat3_2_width;
	crd_status_orb_stat4_2_y = crd_status_orb_stat1_2_y;

	crd_status_orb_stat5_2_width = crd_status_orb_stat1_2_width;
	crd_status_orb_stat5_2_height = crd_status_orb_stat1_2_height;
	crd_status_orb_stat5_2_x = crd_status_orb_stat4_2_x+crd_status_orb_stat4_2_width;
	crd_status_orb_stat5_2_y = crd_status_orb_stat1_2_y;

	crd_status_orb_stat6_2_width = crd_status_orb_stat1_2_width;
	crd_status_orb_stat6_2_height = crd_status_orb_stat1_2_height;
	crd_status_orb_stat6_2_x = crd_status_orb_stat5_2_x+crd_status_orb_stat5_2_width;
	crd_status_orb_stat6_2_y = crd_status_orb_stat1_2_y;

	// Command Button 1.
	command_sub_max = 12;	// Maximum number of entries in a command button list.
	command_list_max = 5;	// The maximum number of command button lists.
	command_max = 13;	// Maximum number of command buttons.
	crd_status_orb_command_1_width = crd_portrait_sml_width;
	crd_status_orb_command_1_height = crd_portrait_sml_height;
	crd_status_orb_command_1_x = 0;
	crd_status_orb_command_1_y = crd_status_orb_bkg_height+1;
	crd_status_orb_command_span = 2;

	// Command Button 1_1.
	crd_status_orb_command_1_1_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_1_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_1_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_1_y = crd_status_orb_command_1_y+crd_status_orb_command_1_height+crd_status_orb_command_span;

	// Command Button 1_2.
	crd_status_orb_command_1_2_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_2_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_2_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_2_y = crd_status_orb_command_1_1_y+crd_status_orb_command_1_1_height+crd_status_orb_command_span;

	// Command Button 1_3.
	crd_status_orb_command_1_3_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_3_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_3_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_3_y = crd_status_orb_command_1_2_y+crd_status_orb_command_1_2_height+crd_status_orb_command_span;

	// Command Button 1_4.
	crd_status_orb_command_1_4_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_4_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_4_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_4_y = crd_status_orb_command_1_3_y+crd_status_orb_command_1_3_height+crd_status_orb_command_span;

	// Command Button 1_5.
	crd_status_orb_command_1_5_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_5_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_5_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_5_y = crd_status_orb_command_1_4_y+crd_status_orb_command_1_4_height+crd_status_orb_command_span;

	// Command Button 1_6.
	crd_status_orb_command_1_6_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_6_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_6_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_6_y = crd_status_orb_command_1_5_y+crd_status_orb_command_1_5_height+crd_status_orb_command_span;

	// Command Button 1_7.
	crd_status_orb_command_1_7_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_7_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_7_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_7_y = crd_status_orb_command_1_6_y+crd_status_orb_command_1_6_height+crd_status_orb_command_span;

	// Command Button 1_8.
	crd_status_orb_command_1_8_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_8_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_8_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_8_y = crd_status_orb_command_1_7_y+crd_status_orb_command_1_7_height+crd_status_orb_command_span;

	// Command Button 1_9.
	crd_status_orb_command_1_9_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_9_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_9_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_9_y = crd_status_orb_command_1_8_y+crd_status_orb_command_1_8_height+crd_status_orb_command_span;

	// Command Button 1_10.
	crd_status_orb_command_1_10_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_10_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_10_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_10_y = crd_status_orb_command_1_9_y+crd_status_orb_command_1_9_height+crd_status_orb_command_span;

	// Command Button 1_11.
	crd_status_orb_command_1_11_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_11_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_11_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_11_y = crd_status_orb_command_1_10_y+crd_status_orb_command_1_10_height+crd_status_orb_command_span;

	// Command Button 1_12.
	crd_status_orb_command_1_12_width = crd_status_orb_command_1_width;
	crd_status_orb_command_1_12_height = crd_status_orb_command_1_height;
	crd_status_orb_command_1_12_x = crd_status_orb_command_1_x;
	crd_status_orb_command_1_12_y = crd_status_orb_command_1_11_y+crd_status_orb_command_1_11_height+crd_status_orb_command_span;

	// Command Button 2.
	crd_status_orb_command_2_width = crd_status_orb_command_1_width;
	crd_status_orb_command_2_height = crd_status_orb_command_1_height;
	crd_status_orb_command_2_x = crd_status_orb_command_1_x+crd_status_orb_command_1_width+crd_status_orb_command_span;
	crd_status_orb_command_2_y = crd_status_orb_command_1_y;

	// Command Button 2_1.
	crd_status_orb_command_2_1_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_1_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_1_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_1_y = crd_status_orb_command_2_y+crd_status_orb_command_2_height+crd_status_orb_command_span;

	// Command Button 2_2.
	crd_status_orb_command_2_2_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_2_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_2_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_2_y = crd_status_orb_command_2_1_y+crd_status_orb_command_2_1_height+crd_status_orb_command_span;

	// Command Button 2_3.
	crd_status_orb_command_2_3_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_3_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_3_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_3_y = crd_status_orb_command_2_2_y+crd_status_orb_command_2_2_height+crd_status_orb_command_span;

	// Command Button 2_4.
	crd_status_orb_command_2_4_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_4_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_4_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_4_y = crd_status_orb_command_2_3_y+crd_status_orb_command_2_3_height+crd_status_orb_command_span;

	// Command Button 2_5.
	crd_status_orb_command_2_5_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_5_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_5_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_5_y = crd_status_orb_command_2_4_y+crd_status_orb_command_2_4_height+crd_status_orb_command_span;

	// Command Button 2_6.
	crd_status_orb_command_2_6_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_6_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_6_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_6_y = crd_status_orb_command_2_5_y+crd_status_orb_command_2_5_height+crd_status_orb_command_span;

	// Command Button 2_7.
	crd_status_orb_command_2_7_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_7_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_7_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_7_y = crd_status_orb_command_2_6_y+crd_status_orb_command_2_6_height+crd_status_orb_command_span;

	// Command Button 2_8.
	crd_status_orb_command_2_8_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_8_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_8_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_8_y = crd_status_orb_command_2_7_y+crd_status_orb_command_2_7_height+crd_status_orb_command_span;

	// Command Button 2_9.
	crd_status_orb_command_2_9_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_9_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_9_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_9_y = crd_status_orb_command_2_8_y+crd_status_orb_command_2_8_height+crd_status_orb_command_span;

	// Command Button 2_10.
	crd_status_orb_command_2_10_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_10_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_10_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_10_y = crd_status_orb_command_2_9_y+crd_status_orb_command_2_9_height+crd_status_orb_command_span;

	// Command Button 2_11.
	crd_status_orb_command_2_11_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_11_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_11_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_11_y = crd_status_orb_command_2_10_y+crd_status_orb_command_2_10_height+crd_status_orb_command_span;

	// Command Button 2_12.
	crd_status_orb_command_2_12_width = crd_status_orb_command_2_width;
	crd_status_orb_command_2_12_height = crd_status_orb_command_2_height;
	crd_status_orb_command_2_12_x = crd_status_orb_command_2_x;
	crd_status_orb_command_2_12_y = crd_status_orb_command_2_11_y+crd_status_orb_command_2_11_height+crd_status_orb_command_span;

	// Command Button 3.
	crd_status_orb_command_3_width = crd_status_orb_command_1_width;
	crd_status_orb_command_3_height = crd_status_orb_command_1_height;
	crd_status_orb_command_3_x = crd_status_orb_command_2_x+crd_status_orb_command_2_width+crd_status_orb_command_span;
	crd_status_orb_command_3_y = crd_status_orb_command_2_y;

	// Command Button 3_1.
	crd_status_orb_command_3_1_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_1_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_1_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_1_y = crd_status_orb_command_3_y+crd_status_orb_command_3_height+crd_status_orb_command_span;

	// Command Button 3_2.
	crd_status_orb_command_3_2_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_2_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_2_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_2_y = crd_status_orb_command_3_1_y+crd_status_orb_command_3_1_height+crd_status_orb_command_span;

	// Command Button 3_3.
	crd_status_orb_command_3_3_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_3_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_3_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_3_y = crd_status_orb_command_3_2_y+crd_status_orb_command_3_2_height+crd_status_orb_command_span;

	// Command Button 3_4.
	crd_status_orb_command_3_4_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_4_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_4_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_4_y = crd_status_orb_command_3_3_y+crd_status_orb_command_3_3_height+crd_status_orb_command_span;

	// Command Button 3_5.
	crd_status_orb_command_3_5_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_5_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_5_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_5_y = crd_status_orb_command_3_4_y+crd_status_orb_command_3_4_height+crd_status_orb_command_span;

	// Command Button 3_6.
	crd_status_orb_command_3_6_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_6_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_6_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_6_y = crd_status_orb_command_3_5_y+crd_status_orb_command_3_5_height+crd_status_orb_command_span;

	// Command Button 3_7.
	crd_status_orb_command_3_7_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_7_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_7_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_7_y = crd_status_orb_command_3_6_y+crd_status_orb_command_3_6_height+crd_status_orb_command_span;

	// Command Button 3_8.
	crd_status_orb_command_3_8_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_8_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_8_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_8_y = crd_status_orb_command_3_7_y+crd_status_orb_command_3_7_height+crd_status_orb_command_span;

	// Command Button 3_9.
	crd_status_orb_command_3_9_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_9_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_9_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_9_y = crd_status_orb_command_3_8_y+crd_status_orb_command_3_8_height+crd_status_orb_command_span;

	// Command Button 3_10.
	crd_status_orb_command_3_10_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_10_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_10_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_10_y = crd_status_orb_command_3_9_y+crd_status_orb_command_3_9_height+crd_status_orb_command_span;

	// Command Button 3_11.
	crd_status_orb_command_3_11_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_11_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_11_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_11_y = crd_status_orb_command_3_10_y+crd_status_orb_command_3_10_height+crd_status_orb_command_span;

	// Command Button 3_12.
	crd_status_orb_command_3_12_width = crd_status_orb_command_3_width;
	crd_status_orb_command_3_12_height = crd_status_orb_command_3_height;
	crd_status_orb_command_3_12_x = crd_status_orb_command_3_x;
	crd_status_orb_command_3_12_y = crd_status_orb_command_3_11_y+crd_status_orb_command_3_11_height+crd_status_orb_command_span;

	// Command Button 4.
	crd_status_orb_command_4_width = crd_status_orb_command_1_width;
	crd_status_orb_command_4_height = crd_status_orb_command_1_height;
	crd_status_orb_command_4_x = crd_status_orb_command_3_x+crd_status_orb_command_3_width+crd_status_orb_command_span;
	crd_status_orb_command_4_y = crd_status_orb_command_3_y;

	// Command Button 4_1.
	crd_status_orb_command_4_1_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_1_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_1_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_1_y = crd_status_orb_command_4_y+crd_status_orb_command_4_height+crd_status_orb_command_span;

	// Command Button 4_2.
	crd_status_orb_command_4_2_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_2_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_2_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_2_y = crd_status_orb_command_4_1_y+crd_status_orb_command_4_1_height+crd_status_orb_command_span;

	// Command Button 4_3.
	crd_status_orb_command_4_3_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_3_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_3_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_3_y = crd_status_orb_command_4_2_y+crd_status_orb_command_4_2_height+crd_status_orb_command_span;

	// Command Button 4_4.
	crd_status_orb_command_4_4_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_4_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_4_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_4_y = crd_status_orb_command_4_3_y+crd_status_orb_command_4_3_height+crd_status_orb_command_span;

	// Command Button 4_5.
	crd_status_orb_command_4_5_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_5_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_5_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_5_y = crd_status_orb_command_4_4_y+crd_status_orb_command_4_4_height+crd_status_orb_command_span;

	// Command Button 4_6.
	crd_status_orb_command_4_6_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_6_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_6_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_6_y = crd_status_orb_command_4_5_y+crd_status_orb_command_4_5_height+crd_status_orb_command_span;

	// Command Button 4_7.
	crd_status_orb_command_4_7_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_7_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_7_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_7_y = crd_status_orb_command_4_6_y+crd_status_orb_command_4_6_height+crd_status_orb_command_span;

	// Command Button 4_8.
	crd_status_orb_command_4_8_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_8_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_8_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_8_y = crd_status_orb_command_4_7_y+crd_status_orb_command_4_7_height+crd_status_orb_command_span;

	// Command Button 4_9.
	crd_status_orb_command_4_9_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_9_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_9_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_9_y = crd_status_orb_command_4_8_y+crd_status_orb_command_4_8_height+crd_status_orb_command_span;

	// Command Button 4_10.
	crd_status_orb_command_4_10_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_10_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_10_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_10_y = crd_status_orb_command_4_9_y+crd_status_orb_command_4_9_height+crd_status_orb_command_span;

	// Command Button 4_11.
	crd_status_orb_command_4_11_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_11_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_11_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_11_y = crd_status_orb_command_4_10_y+crd_status_orb_command_4_10_height+crd_status_orb_command_span;

	// Command Button 4_12.
	crd_status_orb_command_4_12_width = crd_status_orb_command_4_width;
	crd_status_orb_command_4_12_height = crd_status_orb_command_4_height;
	crd_status_orb_command_4_12_x = crd_status_orb_command_4_x;
	crd_status_orb_command_4_12_y = crd_status_orb_command_4_11_y+crd_status_orb_command_4_11_height+crd_status_orb_command_span;

	// Command Button 5.
	crd_status_orb_command_5_width = crd_status_orb_command_1_width;
	crd_status_orb_command_5_height = crd_status_orb_command_1_height;
	crd_status_orb_command_5_x = crd_status_orb_command_4_x+crd_status_orb_command_4_width+crd_status_orb_command_span;
	crd_status_orb_command_5_y = crd_status_orb_command_4_y;

	// Command Button 5_1.
	crd_status_orb_command_5_1_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_1_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_1_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_1_y = crd_status_orb_command_5_y+crd_status_orb_command_5_height+crd_status_orb_command_span;

	// Command Button 5_2.
	crd_status_orb_command_5_2_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_2_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_2_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_2_y = crd_status_orb_command_5_1_y+crd_status_orb_command_5_1_height+crd_status_orb_command_span;

	// Command Button 5_3.
	crd_status_orb_command_5_3_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_3_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_3_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_3_y = crd_status_orb_command_5_2_y+crd_status_orb_command_5_2_height+crd_status_orb_command_span;

	// Command Button 5_4.
	crd_status_orb_command_5_4_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_4_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_4_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_4_y = crd_status_orb_command_5_3_y+crd_status_orb_command_5_3_height+crd_status_orb_command_span;

	// Command Button 5_5.
	crd_status_orb_command_5_5_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_5_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_5_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_5_y = crd_status_orb_command_5_4_y+crd_status_orb_command_5_4_height+crd_status_orb_command_span;

	// Command Button 5_6.
	crd_status_orb_command_5_6_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_6_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_6_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_6_y = crd_status_orb_command_5_5_y+crd_status_orb_command_5_5_height+crd_status_orb_command_span;

	// Command Button 5_7.
	crd_status_orb_command_5_7_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_7_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_7_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_7_y = crd_status_orb_command_5_6_y+crd_status_orb_command_5_6_height+crd_status_orb_command_span;

	// Command Button 5_8.
	crd_status_orb_command_5_8_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_8_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_8_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_8_y = crd_status_orb_command_5_7_y+crd_status_orb_command_5_7_height+crd_status_orb_command_span;

	// Command Button 5_9.
	crd_status_orb_command_5_9_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_9_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_9_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_9_y = crd_status_orb_command_5_8_y+crd_status_orb_command_5_8_height+crd_status_orb_command_span;

	// Command Button 5_10.
	crd_status_orb_command_5_10_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_10_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_10_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_10_y = crd_status_orb_command_5_9_y+crd_status_orb_command_5_9_height+crd_status_orb_command_span;

	// Command Button 5_11.
	crd_status_orb_command_5_11_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_11_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_11_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_11_y = crd_status_orb_command_5_10_y+crd_status_orb_command_5_10_height+crd_status_orb_command_span;

	// Command Button 5_12.
	crd_status_orb_command_5_12_width = crd_status_orb_command_5_width;
	crd_status_orb_command_5_12_height = crd_status_orb_command_5_height;
	crd_status_orb_command_5_12_x = crd_status_orb_command_5_x;
	crd_status_orb_command_5_12_y = crd_status_orb_command_5_11_y+crd_status_orb_command_5_11_height+crd_status_orb_command_span;

	// Command Button 6.
	crd_status_orb_command_6_width = crd_status_orb_command_1_width;
	crd_status_orb_command_6_height = crd_status_orb_command_1_height;
	crd_status_orb_command_6_x = crd_status_orb_command_5_x+crd_status_orb_command_5_width+crd_status_orb_command_span;
	crd_status_orb_command_6_y = crd_status_orb_command_5_y;

	// Command Button 7.
	crd_status_orb_command_7_width = crd_status_orb_command_1_width;
	crd_status_orb_command_7_height = crd_status_orb_command_1_height;
	crd_status_orb_command_7_x = crd_status_orb_command_6_x+crd_status_orb_command_6_width+crd_status_orb_command_span;
	crd_status_orb_command_7_y = crd_status_orb_command_6_y;

	// Command Button 8.
	crd_status_orb_command_8_width = crd_status_orb_command_1_width;
	crd_status_orb_command_8_height = crd_status_orb_command_1_height;
	crd_status_orb_command_8_x = crd_status_orb_command_7_x+crd_status_orb_command_7_width+crd_status_orb_command_span;
	crd_status_orb_command_8_y = crd_status_orb_command_7_y-crd_status_orb_command_span-8;

	// Command Button 9.
	crd_status_orb_command_9_width = crd_status_orb_command_1_width;
	crd_status_orb_command_9_height = crd_status_orb_command_1_height;
	crd_status_orb_command_9_x = crd_status_orb_command_8_x+(crd_status_orb_command_8_width/2)+crd_status_orb_command_span;
	crd_status_orb_command_9_y = crd_status_orb_command_8_y+4-crd_status_orb_command_8_height-crd_status_orb_command_span;

	// Command Button 10.
	crd_status_orb_command_10_width = crd_status_orb_command_1_width;
	crd_status_orb_command_10_height = crd_status_orb_command_1_height;
	crd_status_orb_command_10_x = crd_status_orb_command_9_x;
	crd_status_orb_command_10_y = crd_status_orb_command_9_y-crd_status_orb_command_9_height-(crd_status_orb_command_span/2);

	// Command Button 11.
	crd_status_orb_command_11_width = crd_status_orb_command_1_width;
	crd_status_orb_command_11_height = crd_status_orb_command_1_height;
	crd_status_orb_command_11_x = crd_status_orb_command_10_x;
	crd_status_orb_command_11_y = crd_status_orb_command_10_y-crd_status_orb_command_10_height-(crd_status_orb_command_span/2);

	// Command Button 12.
	crd_status_orb_command_12_width = crd_status_orb_command_1_width;
	crd_status_orb_command_12_height = crd_status_orb_command_1_height;
	crd_status_orb_command_12_x = crd_status_orb_command_11_x;
	crd_status_orb_command_12_y = crd_status_orb_command_11_y-crd_status_orb_command_11_height-(crd_status_orb_command_span/2);

	// Command Button 13.
	crd_status_orb_command_13_width = crd_status_orb_command_1_width;
	crd_status_orb_command_13_height = crd_status_orb_command_1_height;
	crd_status_orb_command_13_x = crd_status_orb_command_12_x;
	crd_status_orb_command_13_y = crd_status_orb_command_12_y-crd_status_orb_command_12_height-(crd_status_orb_command_span/2);
}

// ====================================================================

function UpdateList(listIcon, listCol)
{
	// Populates a given column of command icons with appropriate build portraits for the selected object.
	// Returns an array of this selection.

	// Build unit list.
	if (selection[0].actions.create && selection[0].actions.create.list)
	{
		listName = "";
		switch (listIcon)
		{
			case action_tab_train:
				if (selection[0].actions.create.list.unit)
					listName = selection[0].actions.create.list.unit.toString();
			break;
			case action_tab_buildciv:
				if (selection[0].actions.create.list.structciv)
					listName = selection[0].actions.create.list.structciv.toString();
			break;
			case action_tab_buildmil:
				if (selection[0].actions.create.list.structmil)
					listName = selection[0].actions.create.list.structmil.toString();
			break;
			case action_tab_research:
				if (selection[0].actions.create.list.tech)
					listName = selection[0].actions.create.list.tech.toString();
			break;
			default:
				return 0;
			break;
		}

		if (listName != "")
		{
			// Enable tab portrait.
			setPortrait("session_panel_status_command_pri_" + listCol, "sheet_action", listIcon);
			GUIObjectUnhide("session_panel_status_command_pri_" + listCol);

			// Extract entity list into an array.
			listArray = parseDelimiterString(listName, ";", listName.length);

			// Populate appropriate command buttons.
			for (createLoop = 1; createLoop <= command_sub_max; createLoop++)
			{
				if (createLoop < listArray.length+1)
				{
					if (getEntityTemplate(listArray[createLoop-1]).traits.id.icon_cell && getEntityTemplate(listArray[createLoop-1]).traits.id.icon_cell != "")
						setPortrait("session_panel_status_command_pri_" + listCol + "_" + createLoop, getEntityTemplate(listArray[createLoop-1]).traits.id.icon, getEntityTemplate(listArray[createLoop-1]).traits.id.icon_cell);
					else
						setPortrait("session_panel_status_command_pri_" + listCol + "_" + createLoop, getEntityTemplate(listArray[createLoop-1]).traits.id.icon);
					GUIObjectUnhide("session_panel_status_command_pri_" + listCol + "_" + createLoop);
				}
				else
					GUIObjectHide("session_panel_status_command_pri_" + listCol + "_" + createLoop);
			}

			return listArray;
		}
	}

	return 0;
}

// ====================================================================

function UpdateCommand(listIcon, listCol)
{
	// Similar to UpdateList, but without the list.
	// Updates a particular command button with a particular action.

	if (
            (listIcon == action_attack && selection[0].actions.attack)
         || (listIcon == action_patrol && selection[0].actions.patrol)
         || (listIcon == action_repair && selection[0].actions.repair)
         || (listIcon == action_gather_food && selection[0].actions.gather && selection[0].actions.gather.food)
         || (listIcon == action_gather_wood && selection[0].actions.gather && selection[0].actions.gather.wood)
         || (listIcon == action_gather_stone && selection[0].actions.gather && selection[0].actions.gather.stone)
         || (listIcon == action_gather_ore && selection[0].actions.gather && selection[0].actions.gather.ore)
           )
	{	
		setPortrait("session_panel_status_command_pri_" + listCol, "sheet_action", listIcon);
		GUIObjectUnhide("session_panel_status_command_pri_" + listCol);

		return (listCol-1);
	}
	else
		return (listCol);
}

// ====================================================================

function UpdateCommandButtons()
{
	// Update train/research/build lists.
	listCounter	= 1; 
	unitArray 	= UpdateList(action_tab_train, listCounter); 		if (unitArray != 0)	 listCounter++;
	structcivArray 	= UpdateList(action_tab_buildciv, listCounter);		if (structcivArray != 0) listCounter++;
	structmilArray 	= UpdateList(action_tab_buildmil, listCounter);		if (structmilArray != 0) listCounter++;
	techArray 	= UpdateList(action_tab_research, listCounter);		if (techArray != 0)	 listCounter++;
	formationArray 	= UpdateList(action_tab_formation, listCounter);	if (formationArray != 0) listCounter++;
	stanceArray 	= UpdateList(action_tab_stance, listCounter);		if (stanceArray != 0)	 listCounter++;

	// Update commands.
	commandCounter = command_max;
	commandCounter = UpdateCommand(action_attack, commandCounter);
	commandCounter = UpdateCommand(action_patrol, commandCounter);
	commandCounter = UpdateCommand(action_repair, commandCounter);
	commandCounter = UpdateCommand(action_gather_food, commandCounter);
	commandCounter = UpdateCommand(action_gather_wood, commandCounter);
	commandCounter = UpdateCommand(action_gather_stone, commandCounter);
	commandCounter = UpdateCommand(action_gather_ore, commandCounter);

	// Clear remaining buttons between them.
	for (commandClearLoop = listCounter; commandClearLoop <= commandCounter; commandClearLoop++)
	{
		GUIObjectHide("session_panel_status_command_pri_" + commandClearLoop);
		// If this slot could possibly contain a list, hide that too.
		if (commandClearLoop <= command_list_max)
			GUIObjectHide("session_panel_status_command_pri_" + commandClearLoop + "_group");
	}
}

// ====================================================================

function UpdateStatusOrb()
{
	// Update heading.
	GUIObject = getGUIObjectByName("session_panel_status_heading");
	GUIObject.caption = "Acumen"; // Player name (placeholder; replace with proper callsign).
	if (selection[0].traits.id.civ)
		GUIObject.caption += " [icon=bullet_icon] " + selection[0].traits.id.civ;

	// Update name text.
	GUIObject = getGUIObjectByName("session_panel_status_name1");
	GUIObject.caption = "";
	// Personal name.
	if (selection[0].traits.id.personal && selection[0].traits.id.personal != "")
	{
		GUIObject.caption += selection[0].traits.id.personal + "\n";
	}
	// Generic name.
	if (selection[0].traits.id.generic)
	{
		GUIObject.caption += selection[0].traits.id.generic + "\n";
	}
	// Specific/ranked name.
	if (selection[0].traits.id.ranked)
	{
		GUIObject = getGUIObjectByName("session_panel_status_name1");
		GUIObject.caption += selection[0].traits.id.ranked + "\n";
	}
	else{
		if (selection[0].traits.id.specific)
		{
			GUIObject.caption += selection[0].traits.id.specific + "\n";
		}
	}

	// Update portrait
	if (selection[0].traits.id.icon)
	{
		if (selection[0].traits.id.icon_cell && selection[0].traits.id.icon_cell != "")
			setPortrait("session_panel_status_portrait", selection[0].traits.id.icon, selection[0].traits.id.icon_cell);
		else
			setPortrait("session_panel_status_portrait", selection[0].traits.id.icon);
	}

	// Update rank.
	GUIObject = getGUIObjectByName("session_panel_status_icon_rank");
	if (selection[0].traits.up.rank > 1)
	{
		GUIObject.sprite = "ui_icon_sheet_statistic";
		GUIObject["icon-id"] = stat_rank1 + (selection[0].traits.up.rank-2);
	}
	else
		GUIObject.sprite = "";

	// Update hitpoints
	if (selection[0].traits.health.curr && selection[0].traits.health.max)
	{
		getGUIObjectByName("session_panel_status_icon_hp_text").caption = Math.round(selection[0].traits.health.curr) + "/" + Math.round(selection[0].traits.health.max);
		getGUIObjectByName("session_panel_status_icon_hp_text").hidden = false;
		getGUIObjectByName("session_panel_status_icon_hp_bar").caption = ((Math.round(selection[0].traits.health.curr) * 100 ) / Math.round(selection[0].traits.health.max));
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

	// Update Supply/Garrison
	GUIObject = getGUIObjectByName("session_panel_status_stat1");
	GUIObject.caption = '';

	if (selection[0].traits.garrison)
	{
		if (selection[0].traits.garrison.curr && selection[0].traits.garrison.max)
		{
			GUIObject.caption += '[icon="icon_statistic_garrison"] [color="100 100 255"]' + selection[0].traits.garrison.curr + '/' + selection[0].traits.garrison.max + '[/color] ';
		}
	}

	if (selection[0].traits.supply)
	{
		if (selection[0].traits.supply.curr && selection[0].traits.supply.max && selection[0].traits.supply.type)
		{
			// Special case for infinity.
			if (selection[0].traits.supply.curr == "0" && selection[0].traits.supply.max == "0")
				GUIObject.caption += '[icon="icon_resource_' + selection[0].traits.supply.type + '"] [color="100 100 255"] [icon="infinity_icon"] [/color] ';
			else
				GUIObject.caption += '[icon="icon_resource_' + selection[0].traits.supply.type + '"] [color="100 100 255"]' + selection[0].traits.supply.curr + '/' + selection[0].traits.supply.max + '[/color] ';
		}
	}

	// Update Attack stats
		if (selection[0].actions.attack && selection[0].actions.attack.damage && selection[0].actions.attack.damage > 0)
			getGUIObjectByName("session_panel_status_stat1_1").caption = '[icon="icon_statistic_attack"]' + selection[0].actions.attack.damage;
		else
			getGUIObjectByName("session_panel_status_stat1_1").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.hack && selection[0].actions.attack.hack > 0)
			getGUIObjectByName("session_panel_status_stat2_1").caption = '[icon="icon_statistic_hack"]' + Math.round(selection[0].actions.attack.hack*100) + '%';
		else
			getGUIObjectByName("session_panel_status_stat2_1").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.pierce && selection[0].actions.attack.pierce > 0)
			getGUIObjectByName("session_panel_status_stat3_1").caption = '[icon="icon_statistic_pierce"]' + Math.round(selection[0].actions.attack.pierce*100) + '%';
		else
			getGUIObjectByName("session_panel_status_stat3_1").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.crush && selection[0].actions.attack.crush > 0)
			getGUIObjectByName("session_panel_status_stat4_1").caption = '[icon="icon_statistic_crush"]' + Math.round(selection[0].actions.attack.crush*100) + '%';
		else
			getGUIObjectByName("session_panel_status_stat4_1").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.range && selection[0].actions.attack.range > 0)
			getGUIObjectByName("session_panel_status_stat5_1").caption = '[icon="icon_statistic_range"]' + selection[0].actions.attack.range;
		else
			getGUIObjectByName("session_panel_status_stat5_1").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.accuracy && selection[0].actions.attack.accuracy > 0)
			getGUIObjectByName("session_panel_status_stat6_1").caption = '[icon="icon_statistic_accuracy"]' + Math.round(selection[0].actions.attack.accuracy*100) + '%';
		else
			getGUIObjectByName("session_panel_status_stat6_1").caption = "";

	// Update Armour & Other stats
		if (selection[0].traits.armour && selection[0].traits.armour.value && selection[0].traits.armour.value > 0)
			getGUIObjectByName("session_panel_status_stat1_2").caption = '[icon="icon_statistic_armour"]' + selection[0].traits.armour.value;
		else getGUIObjectByName("session_panel_status_stat1_2").caption = "";
		if (selection[0].traits.armour && selection[0].traits.armour.hack && selection[0].traits.armour.hack > 0)
			getGUIObjectByName("session_panel_status_stat2_2").caption = '[icon="icon_statistic_hack"]' + Math.round(selection[0].traits.armour.hack*100) + '%';
		else getGUIObjectByName("session_panel_status_stat2_2").caption = "";
		if (selection[0].traits.armour && selection[0].traits.armour.pierce && selection[0].traits.armour.pierce > 0)
			getGUIObjectByName("session_panel_status_stat3_2").caption = '[icon="icon_statistic_pierce"]' + Math.round(selection[0].traits.armour.pierce*100) + '%';
		else getGUIObjectByName("session_panel_status_stat3_2").caption = "";
		if (selection[0].traits.armour && selection[0].traits.armour.crush && selection[0].traits.armour.crush > 0)
			getGUIObjectByName("session_panel_status_stat4_2").caption = '[icon="icon_statistic_crush"]' + Math.round(selection[0].traits.armour.crush*100) + '%';
		else getGUIObjectByName("session_panel_status_stat4_2").caption = "";

	if (selection[0].actions.move && selection[0].actions.move.speed)
		getGUIObjectByName("session_panel_status_stat5_2").caption = '[icon="icon_statistic_speed"]' + selection[0].actions.move.speed;
		else getGUIObjectByName("session_panel_status_stat5_2").caption = "";
	if (selection[0].traits.vision && selection[0].traits.vision.los)
		getGUIObjectByName("session_panel_status_stat6_2").caption = '[icon="icon_statistic_los"]' + selection[0].traits.vision.los;
		else getGUIObjectByName("session_panel_status_stat6_2").caption = "";

	// Reveal Status Orb
	getGUIObjectByName("session_status_orb").hidden = false;

	// Update Command Buttons.
	UpdateCommandButtons();
}

