/**
 * Controller for the GUI handling of gamesettings.
 */
class GameSettingsController
{
	constructor(setupWindow, netMessages, playerAssignmentsController, mapCache)
	{
		this.setupWindow = setupWindow;
		this.mapCache = mapCache;
		this.persistentMatchSettings = new PersistentMatchSettings(g_IsNetworked);

		this.guiData = new GameSettingsGuiData();

		// When joining a game, the complete set of attributes
		// may not have been received yet.
		this.loading = true;

		// If this is true, the ready controller won't reset readiness.
		// TODO: ideally the ready controller would be somewhat independent from this one,
		// possibly by listening to gamesetup messages itself.
		this.gameStarted = false;

		this.updateLayoutHandlers = new Set();
		this.settingsChangeHandlers = new Set();
		this.loadingChangeHandlers = new Set();
		this.settingsLoadedHandlers = new Set();

		setupWindow.registerLoadHandler(this.onLoad.bind(this));
		setupWindow.registerGetHotloadDataHandler(this.onGetHotloadData.bind(this));

		setupWindow.registerClosePageHandler(this.onClose.bind(this));

		if (g_IsNetworked)
		{
			if (g_IsController)
				playerAssignmentsController.registerClientJoinHandler(this.onClientJoin.bind(this));
			else
				// In MP, the host launches the game and switches right away,
				// clients switch when they receive the appropriate message.
				netMessages.registerNetMessageHandler("start", this.switchToLoadingPage.bind(this));
			netMessages.registerNetMessageHandler("gamesetup", this.onGamesetupMessage.bind(this));
		}
	}

	/**
	 * @param handler will be called when the layout needs to be updated.
	 */
	registerUpdateLayoutHandler(handler)
	{
		this.updateLayoutHandlers.add(handler);
	}

	/**
	 * @param handler will be called when any setting change.
	 * (this isn't exactly what happens but the behaviour should be similar).
	 */
	registerSettingsChangeHandler(handler)
	{
		this.settingsChangeHandlers.add(handler);
	}

	/**
	 * @param handler will be called when the 'loading' state change.
	 */
	registerLoadingChangeHandler(handler)
	{
		this.loadingChangeHandlers.add(handler);
	}

	/**
	 * @param handler will be called when the initial settings have been loaded.
	 */
	registerSettingsLoadedHandler(handler)
	{
		this.settingsLoadedHandlers.add(handler);
	}

	onLoad(initData, hotloadData)
	{
		if (hotloadData)
			this.parseSettings(hotloadData.initAttributes);
		else if (g_IsController && (initData?.gameSettings || this.persistentMatchSettings.enabled))
		{
			// Allow opting-in to persistence when sending initial data (though default off)
			if (initData?.gameSettings)
				this.persistentMatchSettings.enabled = !!initData.gameSettings?.usePersistence;
			const settings = initData?.gameSettings || this.persistentMatchSettings.loadFile();
			if (settings)
				this.parseSettings(settings);
		}
		// If the new settings led to AI & players conflict, remove the AI.
		for (const guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player !== -1 &&
				g_GameSettings.playerAI.get(g_PlayerAssignments[guid].player - 1))
				g_GameSettings.playerAI.set(g_PlayerAssignments[guid].player - 1, undefined);

		this.updateLayout();
		this.setNetworkInitAttributes();

		// If we are the controller, we are done loading.
		if (hotloadData || !g_IsNetworked || g_IsController)
		{
			for (const handler of this.settingsLoadedHandlers)
				handler();
			this.setLoading(false);
		}
	}

	onClientJoin()
	{
		/**
		 * A note on network synchronization:
		 * The net server does not keep the current state of attributes,
		 * nor does it act like a message queue, so a new client
		 * will only receive updates after they've joined.
		 * In particular, new joiners start with no information,
		 * so the controller must first send them a complete copy of the settings.
		 * However, messages could be in-flight towards the controller,
		 * but the new client may never receive these or have already received them,
		 * leading to an ordering issue that might desync the new client.
		 *
		 * The simplest solution is to have the (single) controller
		 * act as the single source of truth. Any other message must
		 * first go through the controller, which will send updates.
		 * This enforces the ordering of the controller.
		 * In practical terms, if e.g. players controlling their own civ is implemented,
		 * the message will need to be ignored by everyone but the controller,
		 * and the controller will need to send an update once it rejects/accepts the changes,
		 * which will then update the other clients.
		 * Of course, the original client GUI may want to temporarily show a different state.
		 * Note that the final attributes are sent on game start anyways, so any
		 * synchronization issue that might happen at that point can be resolved.
		 */
		Engine.SendGameSetupMessage({
			"type": "initial-update",
			"initAttribs": this.getSettings()
		});
	}

	onGetHotloadData(object)
	{
		object.initAttributes = this.getSettings();
	}

	onGamesetupMessage(message)
	{
		// For now, the controller only can send updates, so no need to listen to messages.
		if (!message.data || g_IsController)
			return;

		if (message.data.type !== "update" &&
			message.data.type !== "initial-update")
		{
			error("Unknown message type " + message.data.type);
			return;
		}

		if (message.data.type === "initial-update")
		{
			// Ignore initial updates if we've already received settings.
			if (!this.loading)
				return;
			this.parseSettings(message.data.initAttribs);
			for (const handler of this.settingsLoadedHandlers)
				handler();
			this.setLoading(false);
		}
		else
			this.parseSettings(message.data.initAttribs);

		// This assumes that messages aren't sent spuriously without changes
		// (which is generally fair), but technically it would be good
		// to check if the new data is different from the previous data.
		for (const handler of this.settingsChangeHandlers)
			handler();
	}

	/**
	 * Returns the InitAttributes, augmented by GUI-specific data.
	 */
	getSettings()
	{
		let ret = g_GameSettings.toInitAttributes();
		ret.guiData = this.guiData.Serialize();
		return ret;
	}

	/**
	 * Parse the following settings.
	 */
	parseSettings(settings)
	{
		g_GameSettings.fromInitAttributes(settings);
		if (settings.guiData)
			this.guiData.Deserialize(settings.guiData);
	}

	setLoading(loading)
	{
		if (this.loading === loading)
			return;
		this.loading = loading;
		for (let handler of this.loadingChangeHandlers)
			handler(loading);
	}

	/**
	 * This should be called whenever the GUI layout needs to be updated.
	 * Triggers on the next GUI tick to avoid un-necessary layout.
	 */
	updateLayout()
	{
		if (this.layoutTimer)
			return;
		this.layoutTimer = setTimeout(() => {
			for (let handler of this.updateLayoutHandlers)
				handler();
			delete this.layoutTimer;
		}, 0);
	}

	/**
	 * This function is to be called when a GUI control has initiated a value change.
	 *
	 * To avoid an infinite loop, do not call this function when a game setup message was
	 * received and the data had only been modified deterministically.
	 *
	 * This is run on a timer to avoid flooding the network with messages,
	 * e.g. when modifying a slider.
	 */
	setNetworkInitAttributes()
	{
		for (let handler of this.settingsChangeHandlers)
			handler();

		if (g_IsNetworked && this.timer === undefined)
			this.timer = setTimeout(this.setNetworkInitAttributesImmediately.bind(this), this.Timeout);
	}

	setNetworkInitAttributesImmediately()
	{
		if (this.timer)
		{
			clearTimeout(this.timer);
			delete this.timer;
		}
		// See note in onClientJoin on network synchronization.
		if (g_IsController)
			Engine.SendGameSetupMessage({
				"type": "update",
				"initAttribs": this.getSettings()
			});
	}

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
	launchGame()
	{
		// Save the file before random settings are resolved.
		this.savePersistentMatchSettings();

		// Mark the game as started so the readyController won't reset state.
		this.gameStarted = true;

		// This will resolve random settings & send game start messages.
		// TODO: this will trigger observers, which is somewhat wasteful.
		g_GameSettings.launchGame(g_PlayerAssignments);

		// Switch to the loading page right away,
		// the GUI will otherwise show the unrandomised settings.
		this.switchToLoadingPage();
	}

	switchToLoadingPage(attributes)
	{
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": attributes?.initAttributes || g_GameSettings.toInitAttributes(),
			"playerAssignments": g_PlayerAssignments
		});
	}

	onClose()
	{
		this.savePersistentMatchSettings();
	}

	savePersistentMatchSettings()
	{
		// TODO: ought to only save a subset of settings.
		this.persistentMatchSettings.saveFile(this.getSettings());
	}
}


/**
 * Wait (at most) this many milliseconds before sending network messages.
 */
GameSettingsController.prototype.Timeout = 400;
