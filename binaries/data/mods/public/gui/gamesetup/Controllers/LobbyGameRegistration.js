/**
 * If there is an XmppClient, this class informs the XPartaMuPP lobby bot that
 * this match is being setup so that others can join.
 * It informs of the lobby of some setting values and the participating clients.
 */
class LobbyGameRegistrationController
{
	constructor(initData, setupWindow, netMessages, mapCache, playerAssignmentsController)
	{
		this.mapCache = mapCache;

		this.serverName = initData.serverName;
		this.hasPassword = initData.hasPassword;

		this.mods = JSON.stringify(Engine.GetEngineInfo().mods);
		this.timer = undefined;

		// Only send a lobby update when its data changed
		this.lastStanza = undefined;

		// Events
		setupWindow.registerClosePageHandler(this.onClosePage.bind(this));
		netMessages.registerNetMessageHandler("start", this.onGameStart.bind(this));
		playerAssignmentsController.registerPlayerAssignmentsChangeHandler(this.sendImmediately.bind(this));

		g_GameSettings.map.watch(() => this.onSettingsChange(), ["map", "type"]);
		g_GameSettings.mapSize.watch(() => this.onSettingsChange(), ["size"]);
		g_GameSettings.victoryConditions.watch(() => this.onSettingsChange(), ["active"]);
		g_GameSettings.playerCount.watch(() => this.onSettingsChange(), ["nbPlayers"]);
	}

	onSettingsChange()
	{
		if (this.lastStanza)
			this.sendDelayed();
		else
			this.sendImmediately();
	}

	onGameStart()
	{
		this.sendImmediately();
		let clients = this.formatClientsForStanza();
		Engine.SendChangeStateGame(clients.connectedPlayers, clients.list);
	}

	onClosePage()
	{
		if (g_IsController && Engine.HasXmppClient())
			Engine.SendUnregisterGame();
	}

	/**
	 * Send the relevant game settings to the lobby bot in a deferred manner.
	 */
	sendDelayed()
	{
		if (!g_IsController || !Engine.HasXmppClient())
			return;

		// Already sending an update - do nothing.
		if (this.timer !== undefined)
			return;

		this.timer = setTimeout(this.sendImmediately.bind(this), this.Timeout);
	}

	/**
	 * Send the relevant game settings to the lobby bot immediately.
	 */
	sendImmediately()
	{
		if (!g_IsController || !Engine.HasXmppClient())
			return;

		// Wait until a map has been selected.
		if (!g_GameSettings.map.map)
			return;

		Engine.ProfileStart("sendRegisterGameStanza");

		if (this.timer !== undefined)
		{
			clearTimeout(this.timer);
			this.timer = undefined;
		}

		let clients = this.formatClientsForStanza();

		let stanza = {
			"name": this.serverName,
			"hostUsername": Engine.LobbyGetNick(),
			"hostJID": "", // Overwritten by C++, placeholder.
			"mapName": g_GameSettings.map.map,
			// TODO: if the map name was always up-to-date we wouldn't need the mapcache here.
			"niceMapName": this.mapCache.getTranslatableMapName(g_GameSettings.map.type, g_GameSettings.map.map),
			"mapSize": g_GameSettings.map.type == "random" ? g_GameSettings.mapSize.size : "Default",
			"mapType": g_GameSettings.map.type,
			"victoryConditions": Array.from(g_GameSettings.victoryConditions.active).join(","),
			"nbp": clients.connectedPlayers,
			"maxnbp": g_GameSettings.playerCount.nbPlayers,
			"players": clients.list,
			"mods": this.mods,
			"hasPassword": this.hasPassword || ""
		};

		// Only send the stanza if one of these properties changed
		if (this.lastStanza && Object.keys(stanza).every(prop => this.lastStanza[prop] == stanza[prop]))
			return;

		this.lastStanza = stanza;
		Engine.SendRegisterGame(stanza);
		Engine.ProfileStop();
	}

	/**
	 * Send a list of playernames and distinct between players and observers.
	 * Don't send teams, AIs or anything else until the game was started.
	 */
	formatClientsForStanza()
	{
		let connectedPlayers = 0;
		let playerData = [];

		for (let guid in g_PlayerAssignments)
		{
			let pData = { "Name": g_PlayerAssignments[guid].name };

			if (g_PlayerAssignments[guid].player <= g_GameSettings.playerCount.nbPlayers)
				++connectedPlayers;
			else
				pData.Team = "observer";

			playerData.push(pData);
		}

		return {
			"list": playerDataToStringifiedTeamList(playerData),
			"connectedPlayers": connectedPlayers
		};
	}
}

/**
 * Send the current game settings to the lobby bot if the settings didn't change for this number of milliseconds.
 */
LobbyGameRegistrationController.prototype.Timeout = 2000;
