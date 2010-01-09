/*
	DESCRIPTION	: Contains global GUI functions, which will later be accessible from every GUI script/file.
	NOTES		: So far, only the message box-related functions are implemented.
*/

// *******************************************
// messageBox
// *******************************************
// @params:     int mbWidth, int mbHeight, string mbMessage, string mbTitle, int mbMode, arr mbButtonCaptions
// @return:     void
// @desc:       Displays a new modal message box.
// *******************************************

function messageBox (mbWidth, mbHeight, mbMessage, mbTitle, mbMode, mbButtonCaptions, mbButtonsCode)
{

	Engine.PushGuiPage("page_msgbox.xml", {
		width: mbWidth,
		height: mbHeight,
		message: mbMessage,
		title: mbTitle,
		mode: mbMode,
		buttonCaptions: mbButtonCaptions,
		buttonCode: mbButtonsCode
	});
}

// ====================================================================

function updateFPS()
{	
	getGUIObjectByName("fpsCounter").caption = "FPS: " + getFPS();
}

// ====================================================================
