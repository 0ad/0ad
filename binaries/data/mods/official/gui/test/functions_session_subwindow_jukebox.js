function initJukebox()
{
	// ============================================= GLOBALS =================================================

	// Background of jukebox.
	JUKEBOX_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].x	= 80; // 120; 
	Crd[Crd.last-1].y	= 180; // 150; 
	Crd[Crd.last-1].width	= Crd[Crd.last-1].x*2-Crd[Crd.last-1].x; 
	Crd[Crd.last-1].height	= Crd[Crd.last-1].y*2-Crd[Crd.last-1].y;

	// Title background of jukebox.
	JUKEBOX_TITLEBAR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 200; 
	Crd[Crd.last-1].height	= 32; 
	Crd[Crd.last-1].x	= -(Crd[Crd.last-1].width/2); 
	Crd[Crd.last-1].y	= Crd[JUKEBOX_BKG].y-Crd[Crd.last-1].height;

	// Title left of jukebox.
	JUKEBOX_TITLEBAR_LEFT = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 50; 
	Crd[Crd.last-1].height	= Crd[JUKEBOX_TITLEBAR].height; 
	Crd[Crd.last-1].x	= -Crd[JUKEBOX_TITLEBAR].width+Crd[Crd.last-1].width; 
	Crd[Crd.last-1].y	= Crd[JUKEBOX_TITLEBAR].y;

	// Title right of jukebox.
	JUKEBOX_TITLEBAR_RIGHT = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= Crd[JUKEBOX_TITLEBAR_LEFT].width; 
	Crd[Crd.last-1].height	= Crd[JUKEBOX_TITLEBAR_LEFT].height; 
	Crd[Crd.last-1].x	= -Crd[JUKEBOX_TITLEBAR].x; 
	Crd[Crd.last-1].y	= Crd[JUKEBOX_TITLEBAR_LEFT].y;

	// List of tracks.
	JUKEBOX_LIST_FILE = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].x	= Crd[JUKEBOX_BKG].x+50; 
	Crd[Crd.last-1].y	= Crd[JUKEBOX_BKG].y+50;
	Crd[Crd.last-1].width	= Crd[Crd.last-1].x*2-Crd[Crd.last-1].x; 
	Crd[Crd.last-1].height	= Crd[Crd.last-1].y*2-Crd[Crd.last-1].y+10; 

	// Combobox to select category of tracks to list.
	JUKEBOX_CATEGORY = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 90; 
	Crd[Crd.last-1].height	= 35; 
	Crd[Crd.last-1].x	= Crd[JUKEBOX_LIST_FILE].x; 
	Crd[Crd.last-1].y	= Crd[JUKEBOX_BKG].y+10;

	// Jukebox exit button.
	JUKEBOX_EXIT_BUTTON = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 16;
	Crd[Crd.last-1].height	= 16;
	Crd[Crd.last-1].x	= Crd[JUKEBOX_BKG].x-Crd[Crd.last-1].width-10;
	Crd[Crd.last-1].y	= Crd[JUKEBOX_BKG].y-Crd[Crd.last-1].height-10;
}

// ====================================================================

function jukeboxDisplay()
{
//	// Display heading.
//	JukeBoxBkg = getGUIObjectByName("JUKEBOX_TITLEBAR");
//	JukeBoxBkg.caption = "Jukebox";
}

// ====================================================================