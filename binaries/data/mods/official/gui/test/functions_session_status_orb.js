function initStatusOrb()
{
	SN_STATUS_PANE_BG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 255; 
	Crd[Crd.last-1].height	= 170; 
	Crd[Crd.last-1].x	= 0; 
	Crd[Crd.last-1].y	= 0; 

	SN_STATUS_PANE_HEADING = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[SN_STATUS_PANE_BG].width; 
	Crd[Crd.last-1].height	= 14; 
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_BG].x+2; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_BG].y+Crd[SN_STATUS_PANE_BG].height-Crd[Crd.last-1].height-3;

	SN_STATUS_PANE_HEADING_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[Crd.last-2].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_BG].y+3;

	SN_STATUS_PANE_PORTRAIT = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= crd_portrait_lrg_width; 
	Crd[Crd.last-1].height	= crd_portrait_lrg_height; 
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_HEADING].x+5; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_HEADING].y-Crd[Crd.last-1].height-7;

	SN_STATUS_PANE_PORTRAIT_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[Crd.last-2].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_HEADING_FLP].y+Crd[SN_STATUS_PANE_HEADING_FLP].height+7;

	SN_STATUS_PANE_ICON_RANK = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= crd_mini_icon_width; 
	Crd[Crd.last-1].height	= crd_mini_icon_height; 
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_PORTRAIT].x+Crd[SN_STATUS_PANE_PORTRAIT].width-Crd[Crd.last-1].width; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_PORTRAIT].y;

	SN_STATUS_PANE_ICON_RANK_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[Crd.last-2].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_PORTRAIT_FLP].y;

	SN_STATUS_PANE_NAME1 = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[SN_STATUS_PANE_BG].width-Crd[SN_STATUS_PANE_PORTRAIT].width-10; 
	Crd[Crd.last-1].height	= Crd[SN_STATUS_PANE_PORTRAIT].height; 
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_PORTRAIT].x+Crd[SN_STATUS_PANE_PORTRAIT].width+2; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_PORTRAIT].y;

	SN_STATUS_PANE_NAME1_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[Crd.last-2].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_PORTRAIT_FLP].y;

	SN_STATUS_PANE_ICON_HP_BAR_SPAN = 2;
	SN_STATUS_PANE_ICON_HP_TEXT_X_SPAN = 4;
	SN_STATUS_PANE_ICON_HP_TEXT_Y_SPAN = 0;

	SN_STATUS_PANE_ICON_HP_BAR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[SN_STATUS_PANE_PORTRAIT].width; 
	Crd[Crd.last-1].height	= 6; 
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_PORTRAIT].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_PORTRAIT].y-Crd[Crd.last-1].height-SN_STATUS_PANE_ICON_HP_BAR_SPAN;

	SN_STATUS_PANE_ICON_HP_BAR_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[Crd.last-2].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_PORTRAIT_FLP].y+Crd[SN_STATUS_PANE_PORTRAIT_FLP].height+SN_STATUS_PANE_ICON_HP_BAR_SPAN;

	SN_STATUS_PANE_ICON_HP_TEXT = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 55; 
	Crd[Crd.last-1].height	= Crd[SN_STATUS_PANE_ICON_HP_BAR].height; 
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_NAME1].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_HP_BAR].y+SN_STATUS_PANE_ICON_HP_TEXT_Y_SPAN;

	SN_STATUS_PANE_ICON_HP_TEXT_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[Crd.last-2].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_HP_BAR_FLP].y+SN_STATUS_PANE_ICON_HP_TEXT_Y_SPAN;

	SN_STATUS_PANE_ICON_XP_BAR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[SN_STATUS_PANE_ICON_HP_BAR].width; 
	Crd[Crd.last-1].height	= Crd[SN_STATUS_PANE_ICON_HP_BAR].height;
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_ICON_HP_BAR].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_HP_BAR].y-Crd[SN_STATUS_PANE_ICON_HP_BAR].height-SN_STATUS_PANE_ICON_HP_BAR_SPAN-1;

	SN_STATUS_PANE_ICON_XP_BAR_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[Crd.last-2].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_HP_BAR_FLP].y+Crd[SN_STATUS_PANE_ICON_HP_BAR_FLP].height+SN_STATUS_PANE_ICON_HP_BAR_SPAN+1;

	SN_STATUS_PANE_ICON_XP_TEXT = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= Crd[SN_STATUS_PANE_ICON_HP_TEXT].width; 
	Crd[Crd.last-1].height	= Crd[SN_STATUS_PANE_ICON_HP_TEXT].height; 
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_ICON_HP_TEXT].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_XP_BAR].y+SN_STATUS_PANE_ICON_HP_TEXT_Y_SPAN;

	SN_STATUS_PANE_ICON_XP_TEXT_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[Crd.last-2].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_XP_BAR_FLP].y+SN_STATUS_PANE_ICON_HP_TEXT_Y_SPAN;

	SN_STATUS_PANE_2STAT = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 126; 
	Crd[Crd.last-1].height	= 30; 
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_ICON_HP_TEXT].x+Crd[SN_STATUS_PANE_ICON_HP_TEXT].width; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_HP_BAR].y+crd_mini_icon_width+1;

	SN_STATUS_PANE_2STAT_FLP = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
	Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 
	Crd[Crd.last-1].x	= Crd[Crd.last-2].x; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_HP_BAR_FLP].y+crd_mini_icon_width+1;

	SN_STATUS_PANE_STAT = new Array();
	SN_STATUS_PANE_STAT_FLP = new Array();
	SN_STATUS_PANE_STAT.last = 0;
	SN_STATUS_PANE_STAT.row_curr  = 1;
	SN_STATUS_PANE_STAT.col_curr  = 1;
	SN_STATUS_PANE_STAT.row  = 2;
	SN_STATUS_PANE_STAT.col  = 6;
	SN_STATUS_PANE_STAT.max	 = 12;
	for (SN_STATUS_PANE_STAT.curr = 1; SN_STATUS_PANE_STAT.curr <= SN_STATUS_PANE_STAT.max; SN_STATUS_PANE_STAT.curr++)
	{
		SN_STATUS_PANE_STAT[SN_STATUS_PANE_STAT.curr] = addArrayElement(Crd, Crd.last); 
		Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
		Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
		Crd[Crd.last-1].width	= (Crd[SN_STATUS_PANE_BG].width-14)/6; 
		Crd[Crd.last-1].height	= 30; 

		if (SN_STATUS_PANE_STAT.col_curr == 1)
			Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_PORTRAIT].x; 
		else
			Crd[Crd.last-1].x	= Crd[Crd.last-3].x+Crd[Crd.last-3].width;

		if (SN_STATUS_PANE_STAT.row_curr == 1)
			Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_XP_BAR].y-Crd[Crd.last-1].height; 
		else
			Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_XP_BAR].y-Crd[Crd.last-1].height-Crd[Crd.last-3].height;

		SN_STATUS_PANE_STAT_FLP[SN_STATUS_PANE_STAT.curr] = addArrayElement(Crd, Crd.last); 
		Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
		Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
		Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
		Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 

		if (SN_STATUS_PANE_STAT.col_curr == 1)
			Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_PORTRAIT_FLP].x; 
		else
			Crd[Crd.last-1].x	= Crd[Crd.last-3].x+Crd[Crd.last-3].width;

		if (SN_STATUS_PANE_STAT.row_curr == 1)
			Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_XP_BAR_FLP].y+Crd[SN_STATUS_PANE_ICON_XP_BAR_FLP].height; 
		else
			Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_ICON_XP_BAR_FLP].y+Crd[SN_STATUS_PANE_ICON_XP_BAR_FLP].height+Crd[Crd.last-3].height;

		if (SN_STATUS_PANE_STAT.col_curr == SN_STATUS_PANE_STAT.col)
		{
			SN_STATUS_PANE_STAT.row_curr++;
			SN_STATUS_PANE_STAT.col_curr = 0;
		}
		SN_STATUS_PANE_STAT.col_curr++;
	}
	SN_STATUS_PANE_STAT.last = SN_STATUS_PANE_STAT.curr;

	SN_STATUS_PANE_COMMAND = new Array();
	SN_STATUS_PANE_COMMAND.tab = new Object();
	SN_STATUS_PANE_COMMAND.list = new Object();
	SN_STATUS_PANE_COMMAND.button = new Object();
	SN_STATUS_PANE_COMMAND.tab.max = 13;		// Maximum number of buttons (either single or lists).
	SN_STATUS_PANE_COMMAND.list.max = 11;		// Maximum number of entries in a list.
	SN_STATUS_PANE_COMMAND.button.max = 5;		// Number of tabs that are single buttons (no list).
	SN_STATUS_PANE_COMMAND.split = 9;			// When we reach this button, split the rows (remainder are vertical, not horizontal).
	SN_STATUS_PANE_COMMAND.span = 2;			// Spacing between lists.
	for (loop = 0; loop < SN_STATUS_PANE_COMMAND.list.max+1; loop++)
		SN_STATUS_PANE_COMMAND[loop] = new Array();
		
	SN_STATUS_PANE_COMMAND_FLP = new Array();
	for (loop = 0; loop < SN_STATUS_PANE_COMMAND.list.max+1; loop++)
		SN_STATUS_PANE_COMMAND_FLP[loop] = new Array();

	for (SN_STATUS_PANE_COMMAND.tab.curr = 1; SN_STATUS_PANE_COMMAND.tab.curr <= SN_STATUS_PANE_COMMAND.tab.max; SN_STATUS_PANE_COMMAND.tab.curr++)
	{
		for (SN_STATUS_PANE_COMMAND.list.curr = 1; SN_STATUS_PANE_COMMAND.list.curr <= SN_STATUS_PANE_COMMAND.list.max; SN_STATUS_PANE_COMMAND.list.curr++)
		{
			SN_STATUS_PANE_COMMAND[SN_STATUS_PANE_COMMAND.list.curr][SN_STATUS_PANE_COMMAND.tab.curr] = new Number( addArrayElement(Crd, Crd.last) ); 
			Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
			Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
			Crd[Crd.last-1].width	= crd_portrait_sml_width; 
			Crd[Crd.last-1].height	= crd_portrait_sml_height; 
			
			SN_STATUS_PANE_COMMAND[SN_STATUS_PANE_COMMAND.list.curr][SN_STATUS_PANE_COMMAND.tab.curr].type = new Object();
			SN_STATUS_PANE_COMMAND[SN_STATUS_PANE_COMMAND.list.curr][SN_STATUS_PANE_COMMAND.tab.curr].name = new Object();
			SN_STATUS_PANE_COMMAND[SN_STATUS_PANE_COMMAND.list.curr][SN_STATUS_PANE_COMMAND.tab.curr].last = new Object();

			if (SN_STATUS_PANE_COMMAND.tab.curr >= SN_STATUS_PANE_COMMAND.split)
			{
				if (SN_STATUS_PANE_COMMAND.list.curr == 1)
				{
					Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_BG].x+Crd[SN_STATUS_PANE_BG].width; 

					if (SN_STATUS_PANE_COMMAND.tab.curr == SN_STATUS_PANE_COMMAND.split && SN_STATUS_PANE_COMMAND.list.curr == 1)
						Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_BG].y; 
					else
						Crd[Crd.last-1].y	= Crd[Crd.last-3].y+Crd[Crd.last-3].height; 		
				}
				else
				{
					Crd[Crd.last-1].x	= Crd[Crd.last-3].x+Crd[Crd.last-3].width+SN_STATUS_PANE_COMMAND.span; 				
					Crd[Crd.last-1].y	= Crd[Crd.last-3].y;
				}
			}
			else
			{
				if (SN_STATUS_PANE_COMMAND.list.curr == 1)
				{
					if (SN_STATUS_PANE_COMMAND.tab.curr == 1 && SN_STATUS_PANE_COMMAND.list.curr == 1)
						Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_BG].x; 
					else
						Crd[Crd.last-1].x	= Crd[Crd.last-3].x+Crd[Crd.last-3].width; 		
	
					Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_BG].y+Crd[SN_STATUS_PANE_BG].height; 
				}
				else
				{
					Crd[Crd.last-1].x	= Crd[Crd.last-3].x; 				
					Crd[Crd.last-1].y	= Crd[Crd.last-3].y+Crd[Crd.last-3].height+SN_STATUS_PANE_COMMAND.span;
				}
			}

			SN_STATUS_PANE_COMMAND_FLP[SN_STATUS_PANE_COMMAND.list.curr][SN_STATUS_PANE_COMMAND.tab.curr] = addArrayElement(Crd, Crd.last); 
			Crd[Crd.last-1].rleft	= Crd[Crd.last-2].rleft;	Crd[Crd.last-1].rtop	= top_screen; 
			Crd[Crd.last-1].rright	= Crd[Crd.last-2].rright;	Crd[Crd.last-1].rbottom	= top_screen; 
			Crd[Crd.last-1].width	= Crd[Crd.last-2].width; 
			Crd[Crd.last-1].height	= Crd[Crd.last-2].height; 

			if (SN_STATUS_PANE_COMMAND.tab.curr >= SN_STATUS_PANE_COMMAND.split)
			{
				if (SN_STATUS_PANE_COMMAND.list.curr == 1)
				{
					Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_BG].x+Crd[SN_STATUS_PANE_BG].width; 

					if (SN_STATUS_PANE_COMMAND.tab.curr == SN_STATUS_PANE_COMMAND.split && SN_STATUS_PANE_COMMAND.list.curr == 1)
						Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_BG].y; 
					else
						Crd[Crd.last-1].y	= Crd[Crd.last-3].y+Crd[Crd.last-3].height; 		
				}
				else
				{
					Crd[Crd.last-1].x	= Crd[Crd.last-3].x+Crd[Crd.last-3].width+SN_STATUS_PANE_COMMAND.span; 				
					Crd[Crd.last-1].y	= Crd[Crd.last-3].y;
				}
			}
			else
			{
				if (SN_STATUS_PANE_COMMAND.list.curr == 1)
				{
					if (SN_STATUS_PANE_COMMAND.tab.curr == 1 && SN_STATUS_PANE_COMMAND.list.curr == 1)
						Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_BG].x; 
					else
						Crd[Crd.last-1].x	= Crd[Crd.last-3].x+Crd[Crd.last-3].width; 		
	
					Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_BG].y+Crd[SN_STATUS_PANE_BG].height; 
				}
				else
				{
					Crd[Crd.last-1].x	= Crd[Crd.last-3].x; 				
					Crd[Crd.last-1].y	= Crd[Crd.last-3].y+Crd[Crd.last-3].height+SN_STATUS_PANE_COMMAND.span;
				}
			}
		}
	}

	SN_STATUS_PANE_COMMAND_PROGRESS = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= left_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= crd_portrait_sml_width; 
	Crd[Crd.last-1].height	= crd_portrait_sml_height;
	Crd[Crd.last-1].x	= Crd[SN_STATUS_PANE_BG].x+Crd[SN_STATUS_PANE_BG].width; 
	Crd[Crd.last-1].y	= Crd[SN_STATUS_PANE_BG].y;
}

// ====================================================================

function UpdateList(listIcon, listCol)
{
	// Populates a given column of command icons with appropriate build portraits for the selected object.
	// Returns an array of this selection.

	// Build unit list.
	if (selection[0].traits.id.civ_code && selection[0].actions.create && selection[0].actions.create.list)
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
			setPortrait("SN_STATUS_PANE_COMMAND_" + listCol + "_1", "sheet_action", "", listIcon);
			GUIObjectUnhide("SN_STATUS_PANE_COMMAND_" + listCol + "_1");

			// Store content info in tab button for future reference.
			SN_STATUS_PANE_COMMAND[1][listCol].type = "list";

			// Extract entity list into an array.
			listArray = parseDelimiterString(listName, ";", listName.length);

			// Reset list length.
			SN_STATUS_PANE_COMMAND[1][listCol].last = 0;

			// Populate appropriate command buttons.
			for (createLoop = 0; createLoop < SN_STATUS_PANE_COMMAND.list.max; createLoop++)
			{
				if (createLoop < listArray.length)
				{
					// Get name of entity to display in list.
					UpdateListEntityName = selection[0].traits.id.civ_code + "_" + listArray[createLoop];

					setPortrait("SN_STATUS_PANE_COMMAND_" + listCol + "_" + parseInt(createLoop+2), getEntityTemplate(UpdateListEntityName).traits.id.icon, selection[0].traits.id.civ_code, getEntityTemplate(UpdateListEntityName).traits.id.icon_cell);
					getGUIObjectByName("SN_STATUS_PANE_COMMAND_" + listCol + "_" + parseInt(createLoop+2)).caption = "";
					GUIObjectUnhide("SN_STATUS_PANE_COMMAND_" + listCol + "_" + parseInt(createLoop+2));
					
					// Store content info in tab button for future reference.
					SN_STATUS_PANE_COMMAND[parseInt(createLoop+2)][listCol].name = listArray[createLoop];
					SN_STATUS_PANE_COMMAND[parseInt(createLoop+2)][listCol].last++;		
				}
				else
					GUIObjectHide("SN_STATUS_PANE_COMMAND_" + listCol + "_" + parseInt(createLoop+2));
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
		setPortrait("SN_STATUS_PANE_COMMAND_" + listCol + "_1", "sheet_action", "", listIcon);
		GUIObjectUnhide("SN_STATUS_PANE_COMMAND_" + listCol + "_1");

		// Store content info in tab button for future reference.
		SN_STATUS_PANE_COMMAND[1][listCol].type = "command";
		SN_STATUS_PANE_COMMAND[1][listCol].last = 0;
		SN_STATUS_PANE_COMMAND[1][listCol].name = listIcon;
		
		return (listCol-1);
	}
	else
		return (listCol);
}

// ====================================================================

function PressCommandButton(GUIObject, list, tab)
{
	switch (list)
	{
	case 1:
		GUIObject.caption = "";
		if (SN_STATUS_PANE_COMMAND[list][tab].type == "list")
		{
//console.write("Clicked [" + list + "," + tab + "]: tab of type " + SN_STATUS_PANE_COMMAND[list][tab].type + "; " + SN_STATUS_PANE_COMMAND[list][tab].last + "; " + SN_STATUS_PANE_COMMAND[list][tab].name);
			// Click the tab button to toggle visibility of its list (if it's of a list type).
			GUIObjectToggle("SN_STATUS_PANE_COMMAND_" + tab + "_GROUP");
		}
		else
		{
			// Perform appropriate actions for different command buttons.
console.write("Clicked [" + list + "," + tab + "]: command of type " + SN_STATUS_PANE_COMMAND[list][tab].type + "; " + SN_STATUS_PANE_COMMAND[list][tab].last + "; " + SN_STATUS_PANE_COMMAND[list][tab].name);

			switch (SN_STATUS_PANE_COMMAND[list][tab].name)
			{
			case action_patrol:
				// setCursor(...)
				selectLocation(
					function (x, y) {
						issueCommand(selection, NMT_Patrol, x, y);
					});
				break;
			case action_attack:
				// setCursor(...)
				selectEntity(
					function (target) {
						issueCommand(selection, NMT_AttackMelee, target);
					});
				break;
			}
		}
		break;
	default:
		// Left-clicked list button.
		console.write("Clicked [" + list + "," + tab + "]: list of type " + SN_STATUS_PANE_COMMAND[list][tab].type + "; " + SN_STATUS_PANE_COMMAND[list][tab].name);
		// Attempt to add the entry to the queue.
		attempt_add_to_build_queue( selection[0], selection[0].traits.id.civ_code + "_" + SN_STATUS_PANE_COMMAND[list][tab].name, list, tab);
		break;
	}
}	

// ====================================================================

function UpdateCommandButtons()
{
	if( shouldUpdateStat( "actions.create.list" ) )
	{
		// Everything in this block is tied to properties in
		// actions.create.list, the above check should limit the
		// number of times this update is needlessly made.
		
		// Update train/research/build lists.
		listCounter		= 1; 
		unitArray 		= UpdateList(action_tab_train, listCounter); 		if (unitArray != 0)		 listCounter++;
		structcivArray 	= UpdateList(action_tab_buildciv, listCounter);		if (structcivArray != 0) listCounter++;
		structmilArray 	= UpdateList(action_tab_buildmil, listCounter);		if (structmilArray != 0) listCounter++;
		techArray 		= UpdateList(action_tab_research, listCounter);		if (techArray != 0)		 listCounter++;
		formationArray 	= UpdateList(action_tab_formation, listCounter);	if (formationArray != 0) listCounter++;
		stanceArray 	= UpdateList(action_tab_stance, listCounter);		if (stanceArray != 0)	 listCounter++;
	}
	
	if( shouldUpdateStat( "actions" ) )
	{
		// Update commands.
		commandCounter = SN_STATUS_PANE_COMMAND.tab.max;
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
			GUIObjectHide("SN_STATUS_PANE_COMMAND_" + commandClearLoop + "_1");
			// If this slot could possibly contain a list, hide that too.
			GUIObjectHide("SN_STATUS_PANE_COMMAND_" + commandClearLoop + "_GROUP");
		}
	}

	// Update production queue.
	GUIObject = getGUIObjectByName("SN_STATUS_PANE_COMMAND_PROGRESS");
	// If the entity has a production item underway,
	if (selection[0].actions.create && selection[0].actions.create.progress && selection[0].actions.create.progress.valueOf() && selection[0].actions.create.progress.valueOf().current && selection[0].actions.create.queue.valueOf() && selection[0].actions.create.queue.valueOf()[0].traits.creation.time)
	{
		// Set the value of the production progress bar.
		GUIObject.caption = ((Math.round(Math.round(selection[0].actions.create.progress.valueOf().current)) * 100 ) / Math.round(selection[0].actions.create.queue.valueOf()[0].traits.creation.time));
		// Set position of progress bar.
		GUIObject.size = getGUIObjectByName("SN_STATUS_PANE_COMMAND_" + selection[0].actions.create.queue.valueOf()[0].tab + "_" + selection[0].actions.create.queue.valueOf()[0].list).size;
		// Set progress bar tooltip.
		GUIObject.tooltip = "Training " + selection[0].actions.create.queue.valueOf()[0].traits.id.generic + " ... " + (Math.round(selection[0].actions.create.queue.valueOf()[0].traits.creation.time-Math.round(selection[0].actions.create.progress.valueOf().current)) + " seconds remaining.");
		// Reveal progressbar.
		GUIObject.hidden  = false;
		
		// Seek through queue.
		for( queueEntry = 0; queueEntry < selection[0].actions.create.queue.valueOf().length; queueEntry++)
		{
			// Update list buttons so that they match the number of entries of that type in the queue.
			getGUIObjectByName("SN_STATUS_PANE_COMMAND_" + selection[0].actions.create.queue.valueOf()[queueEntry].tab + "_" + selection[0].actions.create.queue.valueOf()[queueEntry].list).caption++;
		}
	}
	else
	{
		// Hide the progress bar.
		GUIObject.hidden  = true;
		GUIObject.tooltip = "";
	}
}

// ====================================================================

// Update-on-alteration trickery...

// We don't really want to update every single time we get a
// selection-changed or property-changed event; that could happen
// a lot. Instead, use this bunch of globals to cache any changes
// that happened between GUI updates.

// This boolean determines whether the selection has been changed.
var selectionChanged = false;

// This boolean determines what the template of the selected object
// was when last we looked
var selectionTemplate = null;

// This array holds the name of all properties that need to be updated
var selectionPropertiesChanged = new Array();

// This array holds a list of all the objects we hold property-change 
// watches on
var propertyWatches = new Array();
 
// This function resets all the update variables, above
function resetUpdateVars()
{
	if( selectionChanged )
	{
		for( watchedObject in propertyWatches )
			propertyWatches[watchedObject].unwatchAll( selectionWatchHandler ); // Remove the handler
		
		propertyWatches = new Array();
		if( selection[0] )
		{
			// Watch the object itself
			selection[0].watchAll( selectionWatchHandler );
			propertyWatches.push( selection[0] );
			// And every parent back up the tree (changes there will affect
			// displayed properties via inheritance)
			var parent = selection[0].template
			while( parent )
			{
				parent.watchAll( selectionWatchHandler );
				propertyWatches.push( selection[0] );
				parent = parent.parent;
			}
		}
	}
	selectionChanged = false;
	if( selection[0] ) 
	{
		selectionTemplate = selection[0].template;
	}
	else
		selectionTemplate = null;
		
	selectionPropertiesChanged = new Array();
}

// This function returns whether we should update a particular statistic
// in the GUI (e.g. "actions.attack") - this should happen if: the selection
// changed, the selection had its template altered (changing lots of stuff)
// or an assignment has been made to that stat or any property within that
// stat. 
function shouldUpdateStat( statname )
{
	if( selectionChanged || ( selectionTemplate != selection[0].template ) )
		return( true );
	for( var property in selectionPropertiesChanged )
	{
		// If property starts with statname
		if( selectionPropertiesChanged[property].substring( 0, statname.length ) == statname )
			return( true );
	}
	return( false );	
}

// This function is a handler for the 'selectionChanged' event,
// it updates the selectionChanged flag
function selectionChangedHandler()
{
	selectionChanged = true;
}

// Register it.
addGlobalHandler( "selectionChanged", selectionChangedHandler );

// This function is a handler for a watch event; it updates the
// selectionPropertiesChanged array
function selectionWatchHandler( propname, oldvalue, newvalue )
{
	selectionPropertiesChanged.push( propname );
	// This bit's important (watches allow the handler to change the value
	// before it gets written; we don't want to affect things, so make sure
	// the value we send back is the one that was going to be written)
	return( newvalue ); 
}

function UpdateStatusOrb()
{
	// Update heading.
	if( shouldUpdateStat( "player" ) || shouldUpdateStat( "traits.id.civ" ) )
	{
		GUIObject = getGUIObjectByName("SN_STATUS_PANE_HEADING");
		GUIObject.caption = selection[0].player.name; // Player name (placeholder; replace with proper callsign).
		if (selection[0].traits.id.civ)
			GUIObject.caption += " [icon=bullet_icon] " + selection[0].traits.id.civ;
	}
	
	// Update name text.
	if( shouldUpdateStat( "traits.id" ) )
	{
		GUIObject = getGUIObjectByName("SN_STATUS_PANE_NAME1");
		GUIObject.caption = "";
		
		// Personal name.
		if (selection[0].traits.id.personal && selection[0].traits.id.personal != "")
			GUIObject.caption += selection[0].traits.id.personal + "\n";
		// Generic name.
		if (selection[0].traits.id.generic)
			GUIObject.caption += selection[0].traits.id.generic + "\n";
		// Specific/ranked name.
		if (selection[0].traits.id.ranked)
		{
			GUIObject = getGUIObjectByName("SN_STATUS_PANE_NAME1");
			GUIObject.caption += selection[0].traits.id.ranked + "\n";
		}
		else{
			if (selection[0].traits.id.specific)
			{
				GUIObject.caption += selection[0].traits.id.specific + "\n";
			}
		}
	}
	if( shouldUpdateStat( "traits.id.icon" ) )
	{
		// Update portrait
		if (selection[0].traits.id.icon)
			setPortrait("SN_STATUS_PANE_PORTRAIT", selection[0].traits.id.icon, selection[0].traits.id.civ_code, selection[0].traits.id.icon_cell);
	}
	if( shouldUpdateStat( "traits.up.rank" ) )
	{
		// Update rank.
		GUIObject = getGUIObjectByName("SN_STATUS_PANE_ICON_RANK");
		if (selection[0].traits.up.rank > 1)
		{
			GUIObject.sprite = "ui_icon_sheet_statistic";
			GUIObject.cell_id = stat_rank1 + (selection[0].traits.up.rank-2);
		}
		else
			GUIObject.sprite = "";
	}
	if( shouldUpdateStat( "traits.health" ) )
	{
		// Update hitpoints
		if (selection[0].traits.health.curr && selection[0].traits.health.max)
		{
			getGUIObjectByName("SN_STATUS_PANE_ICON_HP_TEXT").caption = Math.round(selection[0].traits.health.curr) + "/" + Math.round(selection[0].traits.health.max);
			getGUIObjectByName("SN_STATUS_PANE_ICON_HP_TEXT").hidden = false;
			getGUIObjectByName("SN_STATUS_PANE_ICON_HP_BAR").caption = ((Math.round(selection[0].traits.health.curr) * 100 ) / Math.round(selection[0].traits.health.max));
			getGUIObjectByName("SN_STATUS_PANE_ICON_HP_BAR").hidden = false;
		}
		else
		{
			getGUIObjectByName("SN_STATUS_PANE_ICON_HP_TEXT").hidden = true;
			getGUIObjectByName("SN_STATUS_PANE_ICON_HP_BAR").hidden = true;
		}
	}
	
	if( shouldUpdateStat( "traits.up" ) )
	{
		// Update upgrade points
		if (selection[0].traits.up && selection[0].traits.up.curr && selection[0].traits.up.req)
		{
			getGUIObjectByName("SN_STATUS_PANE_ICON_XP_TEXT").caption = Math.round(selection[0].traits.up.curr) + "/" + Math.round(selection[0].traits.up.req);
			getGUIObjectByName("SN_STATUS_PANE_ICON_XP_TEXT").hidden = false;
			getGUIObjectByName("SN_STATUS_PANE_ICON_XP_BAR").caption = ((Math.round(selection[0].traits.up.curr) * 100 ) / Math.round(selection[0].traits.up.req));
			getGUIObjectByName("SN_STATUS_PANE_ICON_XP_BAR").hidden = false;
		}
		else
		{
			getGUIObjectByName("SN_STATUS_PANE_ICON_XP_TEXT").hidden = true;
			getGUIObjectByName("SN_STATUS_PANE_ICON_XP_BAR").hidden = true;
		}
	}
	if( shouldUpdateStat( "traits.garrison" ) )
	{
		// Update Supply/Garrison
		GUIObject = getGUIObjectByName("SN_STATUS_PANE_2STAT");
		GUIObject.caption = '';

		if (selection[0].traits.garrison)
		{
			if (selection[0].traits.garrison.curr && selection[0].traits.garrison.max)
			{
				GUIObject.caption += '[icon="icon_statistic_garrison"] [color="100 100 255"]' + selection[0].traits.garrison.curr + '/' + selection[0].traits.garrison.max + '[/color] ';
			}
		}
	}
	if( shouldUpdateStat( "traits.supply" ) )
	{
		GUIObject = getGUIObjectByName("SN_STATUS_PANE_2STAT");
		GUIObject.caption = '';

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
	}
	
	// Update Attack stats
	if( shouldUpdateStat( "actions.attack" ) )
	{
		if (selection[0].actions.attack && selection[0].actions.attack.damage && selection[0].actions.attack.damage > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT1").caption = '[icon="icon_statistic_attack"]' + selection[0].actions.attack.damage;
		else
			getGUIObjectByName("SN_STATUS_PANE_STAT1").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.hack && selection[0].actions.attack.hack > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT2").caption = '[icon="icon_statistic_hack"]' + Math.round(selection[0].actions.attack.hack*100) + '%';
		else
			getGUIObjectByName("SN_STATUS_PANE_STAT2").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.pierce && selection[0].actions.attack.pierce > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT3").caption = '[icon="icon_statistic_pierce"]' + Math.round(selection[0].actions.attack.pierce*100) + '%';
		else
			getGUIObjectByName("SN_STATUS_PANE_STAT3").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.crush && selection[0].actions.attack.crush > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT4").caption = '[icon="icon_statistic_crush"]' + Math.round(selection[0].actions.attack.crush*100) + '%';
		else
			getGUIObjectByName("SN_STATUS_PANE_STAT4").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.range && selection[0].actions.attack.range > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT5").caption = '[icon="icon_statistic_range"]' + selection[0].actions.attack.range;
		else
			getGUIObjectByName("SN_STATUS_PANE_STAT5").caption = "";

		if (selection[0].actions.attack && selection[0].actions.attack.accuracy && selection[0].actions.attack.accuracy > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT6").caption = '[icon="icon_statistic_accuracy"]' + Math.round(selection[0].actions.attack.accuracy*100) + '%';
		else
			getGUIObjectByName("SN_STATUS_PANE_STAT6").caption = "";
	}
	// Update Armour & Other stats
	if( shouldUpdateStat( "traits.armour" ) )
	{
		if (selection[0].traits.armour && selection[0].traits.armour.value && selection[0].traits.armour.value > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT7").caption = '[icon="icon_statistic_armour"]' + selection[0].traits.armour.value;
		else getGUIObjectByName("SN_STATUS_PANE_STAT7").caption = "";
		if (selection[0].traits.armour && selection[0].traits.armour.hack && selection[0].traits.armour.hack > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT8").caption = '[icon="icon_statistic_hack"]' + Math.round(selection[0].traits.armour.hack*100) + '%';
		else getGUIObjectByName("SN_STATUS_PANE_STAT8").caption = "";
		if (selection[0].traits.armour && selection[0].traits.armour.pierce && selection[0].traits.armour.pierce > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT9").caption = '[icon="icon_statistic_pierce"]' + Math.round(selection[0].traits.armour.pierce*100) + '%';
		else getGUIObjectByName("SN_STATUS_PANE_STAT9").caption = "";
		if (selection[0].traits.armour && selection[0].traits.armour.crush && selection[0].traits.armour.crush > 0)
			getGUIObjectByName("SN_STATUS_PANE_STAT10").caption = '[icon="icon_statistic_crush"]' + Math.round(selection[0].traits.armour.crush*100) + '%';
		else getGUIObjectByName("SN_STATUS_PANE_STAT10").caption = "";
	}
	if( shouldUpdateStat( "actions.move" ) )
	{
		if (selection[0].actions.move && selection[0].actions.move.speed)
			getGUIObjectByName("SN_STATUS_PANE_STAT11").caption = '[icon="icon_statistic_speed"]' + selection[0].actions.move.speed;
			else getGUIObjectByName("SN_STATUS_PANE_STAT11").caption = "";
	}
	if( shouldUpdateStat( "traits.vision" ) )
	{
		if (selection[0].traits.vision && selection[0].traits.vision.los)
			getGUIObjectByName("SN_STATUS_PANE_STAT12").caption = '[icon="icon_statistic_los"]' + selection[0].traits.vision.los;
			else getGUIObjectByName("SN_STATUS_PANE_STAT12").caption = "";
	}
	
	// Reveal Status Orb
	getGUIObjectByName("session_status_orb").hidden = false;

	// Update Command Buttons.
	UpdateCommandButtons();
	
	resetUpdateVars();
}

