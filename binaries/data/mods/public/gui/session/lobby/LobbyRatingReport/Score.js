/**
 * This class computes the economic and military score points of each player.
 */
LobbyRatingReport.prototype.Score = class
{
	insertValues(report, playerStates)
	{
		Object.assign(report, {
			"economyScore": playerStates.map(this.economyScore.bind(this)).join(",") + ",",
			"militaryScore": playerStates.map(this.militaryScore.bind(this)).join(",") + ",",
			"totalScore": playerStates.map(this.totalScore.bind(this)).join(",") + ",",
		});
	}

	/**
	 * Keep this in sync with summary screen score computation!
	 */
	economyScore(playerState)
	{
		let total = 0;
		let time = playerState.sequences.time.length - 1;

		// Notice that this avoids the vegetarianFood property of resourcesGathered
		for (let resCode of g_ResourceData.GetCodes())
			total += playerState.sequences.resourcesGathered[resCode][time];

		total += playerState.sequences.tradeIncome[time];

		return Math.round(total / 10);
	}

	militaryScore(playerState)
	{
		let time = playerState.sequences.time.length - 1;

		let totalDestruction =
			playerState.sequences.enemyUnitsKilledValue[time] +
			playerState.sequences.enemyBuildingsDestroyedValue[time] +
			playerState.sequences.unitsCapturedValue[time] +
			playerState.sequences.buildingsCapturedValue[time];

		return Math.round(totalDestruction / 10);
	}

	explorationScore(playerState)
	{
		let time = playerState.sequences.time.length - 1;
		return playerState.sequences.percentMapExplored[time] * 10;
	}

	totalScore(playerState)
	{
		return this.economyScore(playerState) +
		       this.militaryScore(playerState) +
		       this.explorationScore(playerState);
	}
};
