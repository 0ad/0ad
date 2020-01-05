/**
 * This is a container for classes that extend the report object.
 * Keep in sync with the lobby bot code, the StatisticsTracker.
 */
class LobbyRatingReport
{
}

/**
 * This class reports the state of the current game to the lobby bot when the current player has been defeated or won.
 */
class LobbyRatingReporter
{
	constructor()
	{
		if (!LobbyRatingReporter.Available())
			throw new Error("Lobby rating service is not available");

		registerPlayersFinishedHandler(this.onPlayersFinished.bind(this));
	}

	onPlayersFinished(players)
	{
		// Observers don't send the state, players send it only once per match
		if (players.indexOf(Engine.GetPlayerID()) != -1)
			return;

		let extendedSimState = Engine.GuiInterfaceCall("GetExtendedSimulationState");

		let report = {
			"playerID": Engine.GetPlayerID(),
			"matchID": g_GameAttributes.matchID,
			"mapName": g_GameAttributes.settings.Name,
			"timeElapsed": extendedSimState.timeElapsed,
		};

		// Remove gaia
		let playerStates = clone(extendedSimState.players).slice(1);

		for (let name in LobbyRatingReport.prototype)
			new LobbyRatingReport.prototype[name]().insertValues(report, playerStates);

		Engine.SendGameReport(report);
	}
}

/**
 * Only 1v1 games are rated, account for gaia.
 */
LobbyRatingReporter.Available = function()
{
	return Engine.HasXmppClient() &&
		!g_IsReplay &&
		Engine.GetPlayerID() != -1 &&
		g_GameAttributes.settings.RatingEnabled &&
		g_GameAttributes.settings.PlayerData.length == 3;
};
