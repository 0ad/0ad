/**
 * This class manages the button that allows the player to close the lobby page.
 */
class QuitButton
{
	constructor(dialog, leaderboardPage, profilePage)
	{
		let closeDialog = this.closeDialog.bind(this);
		let returnToMainMenu = this.returnToMainMenu.bind(this);
		let onPress = dialog ? closeDialog : returnToMainMenu;

		let leaveButton = Engine.GetGUIObjectByName("leaveButton");
		leaveButton.onPress = onPress;
		leaveButton.caption = dialog ?
			translateWithContext("previous page", "Back") :
			translateWithContext("previous page", "Main Menu");

		if (dialog)
		{
			Engine.SetGlobalHotkey("lobby", "Press", onPress);
			Engine.SetGlobalHotkey("cancel", "Press", onPress);

			let cancelHotkey = Engine.SetGlobalHotkey.bind(Engine, "cancel", "Press", onPress);
			leaderboardPage.registerClosePageHandler(cancelHotkey);
			profilePage.registerClosePageHandler(cancelHotkey);
		}
	}

	closeDialog()
	{
		Engine.LobbySetPlayerPresence("playing");
		Engine.PopGuiPage();
	}

	returnToMainMenu()
	{
		Engine.StopXmppClient();
		Engine.SwitchGuiPage("page_pregame.xml");
	}
}
