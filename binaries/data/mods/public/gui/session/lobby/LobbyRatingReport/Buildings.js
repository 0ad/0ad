/**
 * This class reports the structures built, lost, and destroyed of some selected structure classes.
 */
LobbyRatingReport.prototype.Buildings = class
{
	insertValues(report, playerStates)
	{
		let lower = txt => txt.substr(0, 1).toLowerCase() + txt.substr(1);
		let time = playerStates[0].sequences.time.length - 1;

		for (let buildingClass in playerStates[0].sequences.buildingsConstructed)
			report[lower(buildingClass) + "BuildingsConstructed"] = playerStates.map(playerState =>
				playerState.sequences.buildingsConstructed[buildingClass][time]).join(",") + ",";

		for (let buildingClass in playerStates[0].sequences.buildingsLost)
			report[lower(buildingClass) + "BuildingsLost"] = playerStates.map(playerState =>
				playerState.sequences.buildingsLost[buildingClass][time]).join(",") + ",";

		for (let buildingClass in playerStates[0].sequences.enemyBuildingsDestroyed)
			report["enemy" + buildingClass + "BuildingsDestroyed"] = playerStates.map(playerState =>
				playerState.sequences.enemyBuildingsDestroyed[buildingClass][time]).join(",") + ",";
	}
};
