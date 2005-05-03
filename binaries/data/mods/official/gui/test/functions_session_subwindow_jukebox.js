function initJukebox()
{
	// ============================================ CONSTANTS ================================================

	JUKEBOX = new Object();
	JUKEBOX.span = 5;

	// ============================================= GLOBALS =================================================

	// Background of jukebox.
	JUKEBOX_BKG = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 50; 
	Crd[Crd.last-1].height	= 50; 
	Crd[Crd.last-1].x	= 50; 
	Crd[Crd.last-1].y	= 50; 

	// Title background of jukebox.
	JUKEBOX_TITLEBAR = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= mid_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= mid_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 100; 
	Crd[Crd.last-1].height	= 32; 
	Crd[Crd.last-1].x	= 0; 
	Crd[Crd.last-1].y	= Crd[JUKEBOX_BKG].y;

	// List of tracks.
	JUKEBOX_LIST_FILE = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= left_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 100; 
	Crd[Crd.last-1].height	= 75; 
	Crd[Crd.last-1].x	= 100; 
	Crd[Crd.last-1].y	= 100;

	// Combobox to select category of tracks to list.
	JUKEBOX_CATEGORY = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= bottom_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= bottom_screen; 
	Crd[Crd.last-1].width	= 100; 
	Crd[Crd.last-1].height	= 32; 
	Crd[Crd.last-1].x	= 20; 
	Crd[Crd.last-1].y	= 20;

	// Jukebox exit button.
	JUKEBOX_EXIT_BUTTON = addArrayElement(Crd, Crd.last); 
	Crd[Crd.last-1].rleft	= right_screen;	Crd[Crd.last-1].rtop	= top_screen; 
	Crd[Crd.last-1].rright	= right_screen;	Crd[Crd.last-1].rbottom	= top_screen; 
	Crd[Crd.last-1].width	= 16;
	Crd[Crd.last-1].height	= 16;
	Crd[Crd.last-1].x	= 26;
	Crd[Crd.last-1].y	= 25;
}

// ====================================================================

function jukeboxDisplay()
{
//	// Display heading.
//	JukeBoxBkg = getGUIObjectByName("JUKEBOX_TITLEBAR");
//	JukeBoxBkg.caption = "Jukebox";
}

// ====================================================================