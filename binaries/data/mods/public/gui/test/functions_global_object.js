/*
	DESCRIPTION	: Contains global GUI functions, which will later be accessible from every GUI script/file.
	NOTES		: So far, only the message box-related functions are implemented.
*/

/*
 ***************************************

 File version:        0.1
 Edited by:           Malte, Nov 14 2004

 ***************************************

 Further comments:

 Changelog:
 v0.1:        - first version.

 ***************************************
*/

// *******************************************
// messageBox
// *******************************************
// @params:     int mbWidth, int mbHeight, string mbMessage, string mbTitle, int mbMode, arr mbButtonCaptions
// @return:     void
// @desc:       Displays a new message box. So far, we can only display one message box at
//              a time, since the message box is based on one base-object which is modified
//              according to the parameters. Later, we should probably be able to create GUI
//              Objects (like the message box) at runtime.
// *******************************************

/*
TODO: Make button code work for any number of buttons without extending the code each time! ((arrays | nested variables) & calculating sizes)
*/

// ====================================================================

function messageBox (mbWidth, mbHeight, mbMessage, mbTitle, mbMode, mbButtonCaptions, mbButtonsCode)
{
	mbMainObj 	= getGUIObjectByName ("mbMain");
	mbTitleObj 	= getGUIObjectByName ("mbTitleBar");
	mbTextObj 	= getGUIObjectByName ("mbText");
	
	mbButton1Obj 	= getGUIObjectByName ("mbButton1");
	mbButton2Obj 	= getGUIObjectByName ("mbButton2");
	mbButton3Obj 	= getGUIObjectByName ("mbButton3");

	// Calculate size
	mbLRDiff = mbWidth / 2;     // Message box left/right difference from 50% of screen
	mbUDDiff = mbHeight / 2;    // Message box up/down difference from 50% of screen
	
	mbSizeString = "50%-" + mbLRDiff + " 50%-" + mbUDDiff + " 50%+" + mbLRDiff + " 50%+" + mbUDDiff;
	
	mbMainObj.size = mbSizeString;
	
	// Texts
	mbTitleObj.caption	= mbTitle;
	mbTextObj.caption 	= mbMessage;

	// Message box modes
	// There is a number of standard modes, and if none of these is used (mbMode == 0), the button captions will be
	// taken from the array mbButtonCaptions; there currently is a maximum of three buttons.
	switch (mbMode)
	{
	case 1:
		// Simple Yes/No question box
		mbButtonCaptions = new Array("Yes", "No");
		break;
	case 2:
		// Okay-only box
		mbButtonCaptions = new Array("OK");
		break;
	case 3:
		// Retry/Abort/Ignore box (will we ever need this?!)
		mbButtonCaptions = new Array("Retry", "Ignore", "Abort");
	default:
		break;
	}

	// Buttons
	switch (mbButtonCaptions.length)
	{
	case 1:
		// One Button only
		mbButton1Obj.caption = mbButtonCaptions[0];
		mbButton1Obj.size = "30% 100%-80 70% 100%-50";
		mbButton1Obj.hidden = false;
		mbButton2Obj.hidden = true;
		mbButton3Obj.hidden = true;
		break;
	case 2:
		// Two Buttons
		mbButton1Obj.caption = mbButtonCaptions[0];
		mbButton2Obj.caption = mbButtonCaptions[1];
		mbButton1Obj.size = "10% 100%-80 45% 100%-50";
		mbButton2Obj.size = "55% 100%-80 90% 100%-50";
		mbButton1Obj.hidden = false;
		mbButton2Obj.hidden = false;
		mbButton3Obj.hidden = true;
		break;
	case 3:
		// Three Buttons
		mbButton1Obj.caption = mbButtonCaptions[0];
		mbButton2Obj.caption = mbButtonCaptions[1];
		mbButton3Obj.caption = mbButtonCaptions[2];
		mbButton1Obj.size = "10% 100%-80 30% 100%-50";
		mbButton2Obj.size = "40% 100%-80 60% 100%-50";
		mbButton3Obj.size = "70% 100%-80 90% 100%-50";
		mbButton1Obj.hidden = false;
		mbButton2Obj.hidden = false;
		mbButton3Obj.hidden = false;
		break;
	}

	// Show the message box
	guiUnHide (mbMainObj.name);
	
	// Testing
	getGUIGlobal().mbButton1Code = mbButtonsCode[0];
	getGUIGlobal().mbButton2Code = mbButtonsCode[1];
	getGUIGlobal().mbButton3Code = mbButtonsCode[2];
}

// ====================================================================

function updateFPS()
{	
	getGUIObjectByName("fpsCounter").caption = "FPS: " + getFPS();
}

// ====================================================================