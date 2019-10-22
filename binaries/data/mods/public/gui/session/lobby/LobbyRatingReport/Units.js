/**
 * This class reports the units trained, lost and killed of some selected unit classes.
 */
LobbyRatingReport.prototype.Units = class
{
	insertValues(report, playerStates)
	{
		let lower = txt => txt.substr(0, 1).toLowerCase() + txt.substr(1);
		let time = playerStates[0].sequences.time.length - 1;

		for (let unitClass in playerStates[0].sequences.unitsTrained)
			report[lower(unitClass) + "UnitsTrained"] = playerStates.map(playerState =>
				playerState.sequences.unitsTrained[unitClass][time]).join(",") + ",";

		for (let unitClass in playerStates[0].sequences.unitsLost)
			report[lower(unitClass) + "UnitsLost"] = playerStates.map(playerState =>
				playerState.sequences.unitsLost[unitClass][time]).join(",") + ",";

		for (let unitClass in playerStates[0].sequences.enemyUnitsKilled)
			report["enemy" + unitClass + "UnitsKilled"] = playerStates.map(playerState =>
				playerState.sequences.enemyUnitsKilled[unitClass][time]).join(",") + ",";
	}
};
