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
	// Display heading.
	JukeBoxBkg = getGUIObjectByName("MANUAL_BKG");
	JukeBoxBkg.caption = "Jukebox";
}

// ====================================================================