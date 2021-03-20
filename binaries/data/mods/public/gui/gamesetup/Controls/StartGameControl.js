/**
 * Cheat prevention:
 *
 * 1. Ensure that the host cannot start the game unless all clients agreed on the game settings using the ready system.
 *
 * TODO:
 * 2. Ensure that the host cannot start the game with InitAttributes different from the agreed ones.
 * This may be achieved by:
 * - Determining the seed collectively.
 * - passing the agreed game settings to the engine when starting the game instance
 * - rejecting new game settings from the server after the game launch event
 */
class StartGameControl
{
	constructor(netMessages)
	{
		this.gameLaunchHandlers = new Set();

		// This may be read from publicly
		this.gameStarted = false;

		// In MP, the host launches the game and switches right away,
		// clients switch when they receive the appropriate message.
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

		g_GameSettings.launchGame(g_PlayerAssignments);

		// Switch to the loading page right away,
		// the GUI will otherwise show the unrandomised settings.
		this.switchToLoadingPage();
	}

	switchToLoadingPage()
	{
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": g_GameSettings.toInitAttributes(),
			"playerAssignments": g_PlayerAssignments
		});
	}
}
