/*
	DESCRIPTION	: Error-handling utility functions.
	NOTES	: 
*/

// ====================================================================
function cancelOnError(msg)
{
	// Delete game objects
	endGame();
	
	// Return to pregame
	Engine.SwitchGuiPage("page_pregame.xml");
	
	// Display error dialog if message given
	if (msg)
	{
		Engine.PushGuiPage("page_msgbox.xml", {
			width: 400,
			height: 200,
			message: '[font="serif-bold-18"]' + msg + '[/font]',
			title: "Loading Aborted",
			mode: 2
		});
	}
	
	// Reset cursor
	setCursor("arrow-default");
}
