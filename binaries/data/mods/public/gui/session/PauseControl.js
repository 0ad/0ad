/**
 * Controller to pause or resume the game and remember which players paused the game.
 *
 * If the current player ordered a pause manually, it is called explicit pause.
 * If the player opened a dialog in single-player mode, the game is paused implicitly.
 */
class PauseControl
{
	constructor()
	{
		/**
		 * This is true if the current player has paused the game using the pause button or hotkey.
		 * The game may also be paused without this being true in single-player mode when opening a dialog.
		 */
		this.explicitPause = false;

		/**
		 * List of GUIDs of players who have currently paused the game, if the game is networked.
		 */
		this.pausingClients = [];

		/**
		 * Event handlers called when anyone paused.
		 */
		this.pauseHandlers = [];

		registerNetworkStatusChangeHandler(this.onNetworkStatusChangeHandler.bind(this));
	}

	onNetworkStatusChangeHandler()
	{
		if (g_Disconnected)
		{
			Engine.SetPaused(true, false);
			this.callPauseHandlers();
		}
	}

	registerPauseHandler(handler)
	{
		this.pauseHandlers.push(handler);
	}

	callPauseHandlers()
	{
		for (let handler of this.pauseHandlers)
			handler();
	}

	/**
	 * Called from UI dialogs, but only in single-player mode.
	 */
	implicitPause()
	{
		this.setPaused(true, false);
	}

	implicitResume()
	{
		this.setPaused(false, false);
	}

	/**
	 * Returns true if the current player is allowed to pause the game currently.
	 */
	canPause(explicit)
	{
		// Don't pause the game in multiplayer mode when opening dialogs.
		// The NetServer only supports pausing after all clients finished loading the game.
		return !g_IsNetworked || explicit && g_IsNetworkedActive && (!g_IsObserver || g_IsController);
	}

	setPaused(pause, explicit)
	{
		if (!this.canPause(explicit))
			return;

		if (explicit)
			this.explicitPause = pause;

		// If explicit, send network message informing other clients
		Engine.SetPaused(this.explicitPause || pause || g_Disconnected, explicit);

		if (g_IsNetworked)
			this.setClientPauseState(Engine.GetPlayerGUID(), this.explicitPause);
		else
			this.callPauseHandlers();
	}

	/**
	 * Called when a client pauses or resumes in a multiplayer game.
	 */
	setClientPauseState(guid, paused)
	{
		// Update the list of pausing clients.
		let index = this.pausingClients.indexOf(guid);
		if (paused && index == -1)
			this.pausingClients.push(guid);
		else if (!paused && index != -1)
			this.pausingClients.splice(index, 1);

		Engine.SetPaused(!!this.pausingClients.length, false);
		this.callPauseHandlers();
	}
}
