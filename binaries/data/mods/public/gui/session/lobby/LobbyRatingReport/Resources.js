/**
 * This class reports the amount of resources that each player has obtained or used.
 */
LobbyRatingReport.prototype.Resources = class
{
	insertValues(report, playerStates)
	{
		let time = playerStates[0].sequences.time.length - 1;

		for (let action of this.Actions)
			for (let resCode of g_ResourceData.GetCodes())
				report[resCode + action] = playerStates.map(playerState =>
					playerState.sequences["resources" + action][resCode][time]).join(",") + ",";

		report.vegetarianFoodGathered = playerStates.map(
			playerState => playerState.sequences.resourcesGathered.vegetarianFood[time]).join(",") + ",";
	}
};

LobbyRatingReport.prototype.Resources.prototype.Actions = [
	"Gathered",
	"Used",
	"Sold",
	"Bought"
];
