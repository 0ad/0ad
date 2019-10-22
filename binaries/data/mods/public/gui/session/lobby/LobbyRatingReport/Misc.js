/**
 * This class counts trade, tributes, loot and map exploration.
 */
LobbyRatingReport.prototype.Misc = class
{
	insertValues(report, playerStates)
	{
		for (let category of this.MiscCategories)
			report[category] = playerStates.map(playerState =>
				playerState.sequences[category][playerState.sequences.time.length - 1]).join(",") + ",";
	}
};

LobbyRatingReport.prototype.Misc.prototype.MiscCategories = [
	"tradeIncome",
	"tributesSent",
	"tributesReceived",
	"treasuresCollected",
	"lootCollected",
	"percentMapExplored"
];
