/**
 * Send the current list of players, teams, AIs, observers and defeated/won and offline states to the lobby.
 * This report excludes the matchsettings, since they do not change during the match.
 *
 * The playerData format from g_GameAttributes is kept to reuse the GUI function presenting the data,
 * but the payload size is minimized by only extracting properties relevant for display.
 */
class LobbyGamelistReporter
{
	constructor()
	{
		if (!LobbyGamelistReporter.Available())
			throw new Error("Lobby gamelist service not available");

		let updater = this.sendGamelistUpdate.bind(this);
		registerPlayersInitHandler(updater);
		registerPlayersFinishedHandler(updater);
		registerPlayerAssignmentsChangeHandler(updater);
	}

	sendGamelistUpdate()
	{
		Engine.SendChangeStateGame(
			this.countConnectedPlayers(),
			playerDataToStringifiedTeamList([...this.getPlayers(), ...this.getObservers()]));
	}

	getPlayers()
	{
		let players = [];

		// Skip gaia
		for (let playerID = 1; playerID < g_GameAttributes.settings.PlayerData.length; ++playerID)
		{
			let pData = g_GameAttributes.settings.PlayerData[playerID];

			let player = {
				"Name": pData.Name,
				"Civ": pData.Civ
			};

			if (g_GameAttributes.settings.LockTeams)
				player.Team = pData.Team;

			if (pData.AI)
			{
				player.AI = pData.AI;
				player.AIDiff = pData.AIDiff;
				player.AIBehavior = pData.AIBehavior;
			}

			if (g_Players[playerID].offline)
				player.Offline = true;

			// Whether the player has won or was defeated
			let state = g_Players[playerID].state;
			if (state != "active")
				player.State = state;

			players.push(player);
		}
		return players;
	}

	getObservers()
	{
		let observers = [];
		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player == -1)
				observers.push({
					"Name": g_PlayerAssignments[guid].name,
					"Team": "observer"
				});
		return observers;
	}

	countConnectedPlayers()
	{
		let connectedPlayers = 0;
		for (let guid in g_PlayerAssignments)
			if (g_PlayerAssignments[guid].player != -1)
				++connectedPlayers;
		return connectedPlayers;
	}
}

LobbyGamelistReporter.Available = function()
{
	return Engine.HasXmppClient() && g_IsController;
};
