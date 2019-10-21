/**
 * This class reports the chosen settings and victory state of the participating players.
 */
LobbyRatingReport.prototype.Players = class
{
	insertValues(report, playerStates)
	{
		Object.assign(report, {
			"playerStates": playerStates.map(playerState => playerState.state).join(",") + ",",
			"civs": playerStates.map(playerState => playerState.civ).join(",") + ",",
			"teams": playerStates.map(playerState => playerState.team).join(",") + ",",
			"teamsLocked": String(playerStates.every(playerState => playerState.teamsLocked))
		});
	}
};
