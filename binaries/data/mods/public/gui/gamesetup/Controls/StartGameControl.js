/**
 * Cheat prevention:
 *
 * 1. Ensure that the host cannot start the game unless all clients agreed on the gamesettings using the ready system.
 *
 * TODO:
 * 2. Ensure that the host cannot start the game with GameAttributes different from the agreed ones.
 * This may be achieved by:
 * - Determining the seed collectively.
 * - passing the agreed gamesettings to the engine when starting the game instance
 * - rejecting new gamesettings from the server after the game launch event
 */
class StartGameControl
{
	constructor(netMessages)
	{
		this.gameLaunchHandlers = new Set();

		// This may be read from publicly
		this.gameStarted = false;

		netMessages.registerNetMessageHandler("start", this.switchToLoadingPage.bind(this));
	}

	registerLaunchGameHandler(handler)
	{
		this.gameLaunchHandlers.add(handler);
	}

	launchGame()
	{
		this.gameStarted = true;

		for (let handler of this.gameLaunchHandlers)
			handler();

		if (g_IsNetworked)
			Engine.StartNetworkGame();
		else
		{
			Engine.StartGame(g_GameAttributes, g_PlayerAssignments.local.player);
			this.switchToLoadingPage();
		}
	}

	switchToLoadingPage()
	{
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": g_GameAttributes,
			"playerAssignments": g_PlayerAssignments
		});
	}
}
