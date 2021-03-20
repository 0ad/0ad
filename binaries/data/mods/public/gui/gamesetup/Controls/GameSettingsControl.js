/**
 * 'Controller' for the GUI handling of gamesettings.
 */
class GameSettingsControl
{
	constructor(setupWindow, netMessages, startGameControl, mapCache)
	{
		this.startGameControl = startGameControl;
		this.mapCache = mapCache;
		this.gameSettingsFile = new GameSettingsFile(this);

		this.guiData = new GameSettingsGuiData();

		this.updateLayoutHandlers = new Set();
		this.settingsChangeHandlers = new Set();

		setupWindow.registerLoadHandler(this.onLoad.bind(this));
		setupWindow.registerGetHotloadDataHandler(this.onGetHotloadData.bind(this));

		startGameControl.registerLaunchGameHandler(this.onLaunchGame.bind(this));

		setupWindow.registerClosePageHandler(this.onClose.bind(this));

		if (g_IsNetworked)
			netMessages.registerNetMessageHandler("gamesetup", this.onGamesetupMessage.bind(this));
	}

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

	onLoad(initData, hotloadData)
	{
		if (hotloadData)
			this.parseSettings(hotloadData.initAttributes);
		else if (g_IsController && this.gameSettingsFile.enabled)
		{
			let settings = this.gameSettingsFile.loadFile();
			if (settings)
				this.parseSettings(settings);
		}

		this.updateLayout();
		this.setNetworkInitAttributes();
	}

	onClose()
	{
		this.gameSettingsFile.saveFile();
	}

	onGetHotloadData(object)
	{
		object.initAttributes = this.getSettings();
	}

	onGamesetupMessage(message)
	{
		if (!message.data || g_IsController)
			return;

		this.parseSettings(message.data);

		// This assumes that messages aren't sent spuriously without changes
		// (which is generally fair), but technically it would be good
		// to check if the new data is different from the previous data.
		for (let handler of this.settingsChangeHandlers)
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
		if (settings.guiData)
			this.guiData.Deserialize(settings.guiData);
		g_GameSettings.fromInitAttributes(settings);
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
		g_GameSettings.setNetworkInitAttributes();
	}

	onLaunchGame()
	{
		// Save the file before random settings are resolved.
		this.gameSettingsFile.saveFile();
	}
}


/**
 * Wait (at most) this many milliseconds before sending network messages.
 */
GameSettingsControl.prototype.Timeout = 400;
