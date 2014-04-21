/*
	DESCRIPTION	: Error-handling utility functions.
	NOTES	: 
*/

// ====================================================================
function cancelOnError(msg)
{
	// Delete game objects
	Engine.EndGame();
	
	// Return to pregame
	Engine.SwitchGuiPage("page_pregame.xml");
	
	// Display error dialog if message given
	if (msg)
	{
		Engine.PushGuiPage("page_msgbox.xml", {
			width: 500,
			height: 200,
			message: '[font="serif-bold-18"]' + msg + '[/font]',
			title: translate("Loading Aborted"),
			mode: 2
		});
	}
	
	// Reset cursor
	Engine.SetCursor("arrow-default");
}
