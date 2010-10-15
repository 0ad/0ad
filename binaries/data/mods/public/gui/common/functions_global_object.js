/*
	DESCRIPTION	: Contains global GUI functions, which will later be accessible from every GUI script/file.
	NOTES		: So far, only the message box-related functions are implemented.
*/

// *******************************************
// messageBox
// *******************************************
// @params:     int mbWidth, int mbHeight, string mbMessage, string mbTitle, int mbMode, arr mbButtonCaptions, arr mbButtonsCode
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

function s3tcWarning(isMesa)
{
	var msg =
		"Your graphics drivers do not support S3TC compressed textures. " +
		"The game will work correctly, but may have reduced performance.\n\n" +
		(isMesa ?
			"To fix this, you may have to install the \"libtxc_dxtn\" library. " +
			"See http://trac.wildfiregames.com/wiki/CompressedTextures for more information."
		:
			"Please try updating your graphics drivers to ensure you have full hardware acceleration."
		);

	messageBox(560, 270, msg, "Warning", 0,
		["Open web page", "Ignore warning"],
		[function() { Engine.OpenURL("http://trac.wildfiregames.com/wiki/CompressedTextures"); }, undefined]
	);
}
