function initSubWindows()
{
	SN_INGAME_MENU_BG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].x	= -100; 
	Crd[Crd.last-1].y	= -150; 
	Crd[Crd.last-1].width	= (Crd[Crd.last-1].x * -1) * 2; 
	Crd[Crd.last-1].height	= (Crd[Crd.last-1].y * -1) * 2; 

	SN_INGAME_MENU_BTN = new Object();
	SN_INGAME_MENU_BTN.span = 5;

	SN_INGAME_MENU_BTN_RETURN = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= Crd[SN_INGAME_MENU_BG].width; 
	Crd[Crd.last-1].height	= 34; 
	Crd[Crd.last-1].x	= Crd[SN_INGAME_MENU_BG].x; 
	Crd[Crd.last-1].y	= Crd[SN_INGAME_MENU_BG].y+Crd[SN_INGAME_MENU_BG].height-Crd[Crd.last-1].height; 

	SN_INGAME_MENU_BTN_EXIT = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= Crd[SN_INGAME_MENU_BTN_RETURN].width; 
	Crd[Crd.last-1].height	= Crd[SN_INGAME_MENU_BTN_RETURN].height; 
	Crd[Crd.last-1].x	= Crd[SN_INGAME_MENU_BTN_RETURN].x; 
	Crd[Crd.last-1].y	= Crd[SN_INGAME_MENU_BTN_RETURN].y-Crd[SN_INGAME_MENU_BTN_RETURN].height-SN_INGAME_MENU_BTN.span; 

	SN_INGAME_MENU_BTN_RESIGN = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= mid_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= mid_screen; 
	Crd[Crd.last-1].width	= Crd[SN_INGAME_MENU_BTN_RETURN].width; 
	Crd[Crd.last-1].height	= Crd[SN_INGAME_MENU_BTN_RETURN].height; 
	Crd[Crd.last-1].x	= Crd[SN_INGAME_MENU_BTN_RETURN].x; 
	Crd[Crd.last-1].y	= Crd[SN_INGAME_MENU_BTN_EXIT].y-Crd[SN_INGAME_MENU_BTN_EXIT].height-SN_INGAME_MENU_BTN.span; 
}

// ====================================================================