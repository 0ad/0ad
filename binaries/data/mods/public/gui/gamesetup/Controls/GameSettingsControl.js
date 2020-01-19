/**
 * This class provides a property independent interface to g_GameAttributes events.
 * Classes may use this interface in order to react to changing g_GameAttributes.
 */
class GameSettingsControl
{
	constructor(setupWindow, netMessages, startGameControl, mapCache)
	{
		this.startGameControl = startGameControl;
		this.mapCache = mapCache;
		this.gameSettingsFile = new GameSettingsFile(setupWindow);

		this.previousMap = undefined;
		this.depth = 0;

		// This property may be read from publicly
		this.autostart = false;

		this.gameAttributesChangeHandlers = new Set();
		this.gameAttributesBatchChangeHandlers = new Set();
		this.gameAttributesFinalizeHandlers = new Set();
		this.pickRandomItemsHandlers = new Set();
		this.assignPlayerHandlers = new Set();
		this.mapChangeHandlers = new Set();

		setupWindow.registerLoadHandler(this.onLoad.bind(this));
		setupWindow.registerGetHotloadDataHandler(this.onGetHotloadData.bind(this));

		startGameControl.registerLaunchGameHandler(this.onLaunchGame.bind(this));

		if (g_IsNetworked)
			netMessages.registerNetMessageHandler("gamesetup", this.onGamesetupMessage.bind(this));
	}

	registerMapChangeHandler(handler)
	{
		this.mapChangeHandlers.add(handler);
	}

	unregisterMapChangeHandler(handler)
	{
		this.mapChangeHandlers.delete(handler);
	}

	/**
	 * This message is triggered everytime g_GameAttributes change.
	 * Handlers may subsequently change g_GameAttributes and trigger this message again.
	 */
	registerGameAttributesChangeHandler(handler)
	{
		this.gameAttributesChangeHandlers.add(handler);
	}

	unregisterGameAttributesChangeHandler(handler)
	{
		this.gameAttributesChangeHandlers.delete(handler);
	}

	/**
	 * This message is triggered after g_GameAttributes changed and recursed gameAttributesChangeHandlers finished.
	 * The use case for this is to update GUI objects which do not change g_GameAttributes but only display the attributes.
	 */
	registerGameAttributesBatchChangeHandler(handler)
	{
		this.gameAttributesBatchChangeHandlers.add(handler);
	}

	unregisterGameAttributesBatchChangeHandler(handler)
	{
		this.gameAttributesBatchChangeHandlers.delete(handler);
	}

	registerGameAttributesFinalizeHandler(handler)
	{
		this.gameAttributesFinalizeHandlers.add(handler);
	}

	unregisterGameAttributesFinalizeHandler(handler)
	{
		this.gameAttributesFinalizeHandlers.delete(handler);
	}

	registerAssignPlayerHandler(handler)
	{
		this.assignPlayerHandlers.add(handler);
	}

	unregisterAssignPlayerHandler(handler)
	{
		this.assignPlayerHandlers.delete(handler);
	}

	registerPickRandomItemsHandler(handler)
	{
		this.pickRandomItemsHandlers.add(handler);
	}

	unregisterPickRandomItemsHandler(handler)
	{
		this.pickRandomItemsHandlers.delete(handler);
	}

	onLoad(initData, hotloadData)
	{
		if (initData && initData.map && initData.mapType)
		{
			Object.defineProperty(this, "autostart", {
				"value": true,
				"writable": false,
				"configurable": false
			});

			// TODO: Fix g_GameAttributes, g_GameAttributes.settings,
			// g_GameAttributes.settings.PlayerData object references and
			// copy over each attribute individually when receiving
			// settings from the server or the local file.
			g_GameAttributes = {
				"mapType": initData.mapType,
				"map": initData.map
			};

			this.updateGameAttributes();
			// Don't launchGame before all Load handlers finished
		}
		else
		{
			if (hotloadData)
				g_GameAttributes = hotloadData.gameAttributes;
			else if (g_IsController && this.gameSettingsFile.enabled)
				g_GameAttributes = this.gameSettingsFile.loadFile();

			this.updateGameAttributes();
			this.setNetworkGameAttributes();
		}
	}

	onGetHotloadData(object)
	{
		object.gameAttributes = g_GameAttributes;
	}

	onGamesetupMessage(message)
	{
		if (!message.data)
			return;

		g_GameAttributes = message.data;
		this.updateGameAttributes();
	}

	/**
	 * This is to be called whenever g_GameAttributes has been changed except on gameAttributes finalization.
	 */
	updateGameAttributes()
	{
		if (this.depth == 0)
			Engine.ProfileStart("updateGameAttributes");

		if (this.depth >= this.MaxDepth)
		{
			error("Infinite loop: " + new Error().stack);
			Engine.ProfileStop();
			return;
		}

		++this.depth;

		// Basic sanitization
		{
			if (!g_GameAttributes.settings)
				g_GameAttributes.settings = {};

			if (!g_GameAttributes.settings.PlayerData)
				g_GameAttributes.settings.PlayerData = new Array(this.DefaultPlayerCount);

			for (let i = 0; i < g_GameAttributes.settings.PlayerData.length; ++i)
				if (!g_GameAttributes.settings.PlayerData[i])
					g_GameAttributes.settings.PlayerData[i] = {};
		}

		// Map change handlers are triggered first, so that GameSettingControls can update their
		// gameAttributes model prior to applying that model in their gameAttributesChangeHandler.
		if (g_GameAttributes.map && this.previousMap != g_GameAttributes.map && g_GameAttributes.mapType)
		{
			this.previousMap = g_GameAttributes.map;
			let mapData = this.mapCache.getMapData(g_GameAttributes.mapType, g_GameAttributes.map);
			for (let handler of this.mapChangeHandlers)
				handler(mapData);
		}

		for (let handler of this.gameAttributesChangeHandlers)
			handler();

		--this.depth;

		if (this.depth == 0)
		{
			for (let handler of this.gameAttributesBatchChangeHandlers)
				handler();
			Engine.ProfileStop();
		}
	}

	/**
	 * This function is to be called when a GUI control has initiated a value change.
	 *
	 * To avoid an infinite loop, do not call this function when a gamesetup message was
	 * received and the data had only been modified deterministically.
	 */
	setNetworkGameAttributes()
	{
		if (g_IsNetworked)
			Engine.SetNetworkGameAttributes(g_GameAttributes);
	}

	getPlayerData(gameAttributes, playerIndex)
	{
		return gameAttributes &&
			gameAttributes.settings &&
			gameAttributes.settings.PlayerData &&
			gameAttributes.settings.PlayerData[playerIndex] || undefined;
	}

	assignPlayer(sourcePlayerIndex, playerIndex)
	{
		if (playerIndex == -1)
			return;

		let target = this.getPlayerData(g_GameAttributes, playerIndex);
		let source = this.getPlayerData(g_GameAttributes, sourcePlayerIndex);

		for (let handler of this.assignPlayerHandlers)
			handler(source, target);

		this.updateGameAttributes();
		this.setNetworkGameAttributes();
	}

	/**
	 * This function is called everytime a random setting selection was resolved,
	 * so that subsequent random settings are triggered too,
	 * for example picking a random biome after picking a random map.
	 */
	pickRandomItems()
	{
		for (let handler of this.pickRandomItemsHandlers)
			handler();
	}

	onLaunchGame()
	{
		if (!this.autostart)
			this.gameSettingsFile.saveFile();

		this.pickRandomItems();

		for (let handler of this.gameAttributesFinalizeHandlers)
			handler();

		this.setNetworkGameAttributes();
	}
}

GameSettingsControl.prototype.MaxDepth = 512;

/**
 * This number is used when selecting the random map type, which doesn't provide PlayerData.
 */
GameSettingsControl.prototype.DefaultPlayerCount = 4;
