/**
 * If there is an XmppClient, this class informs the XPartaMuPP lobby bot that
 * this match is being setup so that others can join.
 * It informs of the lobby of some setting values and the participating clients.
 */
class GameRegisterStanza
{
	constructor(initData, setupWindow, netMessages, gameSettingsControl, playerAssignmentsControl, mapCache)
	{
		this.mapCache = mapCache;

		this.serverName = initData.serverName;
		this.serverPort = initData.serverPort;
		this.stunEndpoint = initData.stunEndpoint;

		this.mods = JSON.stringify(Engine.GetEngineInfo().mods);
		this.timer = undefined;

		// Only send a lobby update when its data changed
		this.lastStanza = undefined;

		// Events
		let sendImmediately = this.sendImmediately.bind(this);
		playerAssignmentsControl.registerClientJoinHandler(sendImmediately);
		playerAssignmentsControl.registerClientLeaveHandler(sendImmediately);

		setupWindow.registerClosePageHandler(this.onClosePage.bind(this));
		gameSettingsControl.registerGameAttributesBatchChangeHandler(this.onGameAttributesBatchChange.bind(this));
		netMessages.registerNetMessageHandler("start", this.onGameStart.bind(this));
	}

	onGameAttributesBatchChange()
	{
		if (this.lastStanza)
			this.sendDelayed();
		else
			this.sendImmediately();
	}

	onGameStart()
	{
		if (!g_IsController || !Engine.HasXmppClient())
			return;

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
	 * Send the relevant gamesettings to the lobbybot in a deferred manner.
	 */
	sendDelayed()
	{
		if (!g_IsController || !Engine.HasXmppClient())
			return;

		if (this.timer !== undefined)
			clearTimeout(this.timer);

		this.timer = setTimeout(this.sendImmediately.bind(this), this.Timeout);
	}

	/**
	 * Send the relevant gamesettings to the lobbybot immediately.
	 */
	sendImmediately()
	{
		if (!g_IsController || !Engine.HasXmppClient())
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
			"port": this.serverPort,
			"hostUsername": Engine.LobbyGetNick(),
			"mapName": g_GameAttributes.map,
			"niceMapName": this.mapCache.getTranslatableMapName(g_GameAttributes.mapType, g_GameAttributes.map),
			"mapSize": g_GameAttributes.mapType == "random" ? g_GameAttributes.settings.Size : "Default",
			"mapType": g_GameAttributes.mapType,
			"victoryConditions": g_GameAttributes.settings.VictoryConditions.join(","),
			"nbp": clients.connectedPlayers,
			"maxnbp": g_GameAttributes.settings.PlayerData.length,
			"players": clients.list,
			"stunIP": this.stunEndpoint ? this.stunEndpoint.ip : "",
			"stunPort": this.stunEndpoint ? this.stunEndpoint.port : "",
			"mods": this.mods
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
	 * The playerData format from g_GameAttributes is kept to reuse the GUI function presenting the data.
	 */
	formatClientsForStanza()
	{
		let connectedPlayers = 0;
		let playerData = [];

		for (let guid in g_PlayerAssignments)
		{
			let pData = { "Name": g_PlayerAssignments[guid].name };

			if (g_GameAttributes.settings.PlayerData[g_PlayerAssignments[guid].player - 1])
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
 * Send the current gamesettings to the lobby bot if the settings didn't change for this number of milliseconds.
 */
GameRegisterStanza.prototype.Timeout = 2000;
