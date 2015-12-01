// FUNCTIONS FOR CALCULATING SCORES
var teamMiscHelperData = [];

function resetDataHelpers()
{
	teamMiscHelperData = [];
}

function updateCountersPlayer(playerState, counters, idGUI)
{
	for (var w in counters)
	{
		var fn = counters[w].fn;
		Engine.GetGUIObjectByName(idGUI + "[" + w + "]").caption = fn && fn(playerState, w);
	}
}

function calculateEconomyScore(playerState, position)
{
	let total = 0;
	for each (var res in playerState.statistics.resourcesGathered)
		total += res;

	return Math.round(total / 10);
}

function calculateMilitaryScore(playerState, position)
{
	return Math.round((playerState.statistics.enemyUnitsKilledValue +
		playerState.statistics.enemyBuildingsDestroyedValue) / 10);
}

function calculateExplorationScore(playerState, position)
{
	return playerState.statistics.percentMapExplored * 10;
}

function calculateScoreTotal(playerState, position)
{
	return calculateEconomyScore(playerState) +
		calculateMilitaryScore(playerState) +
		calculateExplorationScore(playerState);
}

function calculateScoreTeam(counters)
{
	for (var t in g_Teams)
	{
		if (t == -1)
			continue;

		for (var w in counters)
		{
			var total = 0;
			for (var p = 0; p < g_Teams[t]; ++p)
				total += (+Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]").caption);

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = total;
		}
	}
}

function calculateBuildings(playerState, position)
{
	var type = BUILDINGS_TYPES[position];
	return TRAINED_COLOR + playerState.statistics.buildingsConstructed[type] + '[/color] / ' +
	LOST_COLOR + playerState.statistics.buildingsLost[type] + '[/color] / ' +
	KILLED_COLOR + playerState.statistics.enemyBuildingsDestroyed[type] + '[/color]';
}

function calculateColorsTeam(counters)
{
	for (var t in g_Teams)
	{
		if (t == -1)
			continue;

		for (var w in counters)
		{
			var total = {
				c : 0,
				l : 0,
				d : 0
			};
			for (var p = 0; p < g_Teams[t]; ++p)
			{
				var caption = Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]").caption;
				// clean [Color=""], [/Color] and white space for make the sum more easy
				caption = caption.replace(/\[([\w\' \\\"\/\=]*)\]|\s/g, "");

				var splitCaption = caption.split("/");

				total.c += (+splitCaption[0]);
				total.l += (+splitCaption[1]);
				total.d += (+splitCaption[2]);
			}
			var teamTotal = TRAINED_COLOR + total.c + '[/color] / ' +
			LOST_COLOR + total.l + '[/color] / ' + KILLED_COLOR + total.d + '[/color]';

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}

function calculateUnits(playerState, position)
{
	var type = UNITS_TYPES[position];
	return TRAINED_COLOR + playerState.statistics.unitsTrained[type] + '[/color] / ' +
	LOST_COLOR + playerState.statistics.unitsLost[type] + '[/color] / ' +
	KILLED_COLOR + playerState.statistics.enemyUnitsKilled[type] + '[/color]';
}

function calculateResources(playerState, position)
{
	var type = RESOURCES_TYPES[position];
	return INCOME_COLOR + playerState.statistics.resourcesGathered[type] + '[/color] / ' +
	OUTCOME_COLOR + (playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type]) + '[/color]';
}

function calculateTotalResources(playerState, position)
{
	var totalGathered = 0;
	var totalUsed = 0;

	for each (var type in RESOURCES_TYPES)
	{
		totalGathered += playerState.statistics.resourcesGathered[type];
		totalUsed += playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type];
	}

	return INCOME_COLOR + totalGathered + '[/color] / ' + OUTCOME_COLOR + totalUsed + '[/color]';
}

function calculateTreasureCollected(playerState, position)
{
	return playerState.statistics.treasuresCollected;
}

function calculateLootCollected(playerState, position)
{
	return playerState.statistics.lootCollected;
}

function calculateTributeSent(playerState, position)
{
	return INCOME_COLOR + playerState.statistics.tributesSent + "[/color] / " + OUTCOME_COLOR + playerState.statistics.tributesReceived + "[/color]";
}

function calculateResourcesTeam(counters)
{
	for (var t in g_Teams)
	{
		if (t == -1)
			continue;

		for (var w in counters)
		{
			var teamTotal = "undefined";

			var total = {
				i : 0,
				o : 0
			};
			for (var p = 0; p < g_Teams[t]; ++p)
			{
				var caption = Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]").caption;
				// clean [Color=""], [/Color] and white space for make the sum more easy
				caption = caption.replace(/\[([\w\' \\\"\/\=]*)\]|\s/g, "");

				if (w >= 6)
					total.i += (+caption);
				else
				{
					var splitCaption = caption.split("/");

					total.i += (+splitCaption[0]);
					total.o += (+splitCaption[1]);
				}
			}

			if (w >= 6)
				teamTotal = total.i;
			else
				teamTotal = INCOME_COLOR + total.i + "[/color] / " + OUTCOME_COLOR + total.o + "[/color]";

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}

function calculateResourceExchanged(playerState, position)
{
	var type = RESOURCES_TYPES[position];
	return INCOME_COLOR + '+' + playerState.statistics.resourcesBought[type] + '[/color] ' +
		OUTCOME_COLOR + '-' + playerState.statistics.resourcesSold[type] + '[/color]';
}

function calculateBatteryEfficiency(playerState, position)
{
	var totalBought = 0;
	for each (var boughtAmount in playerState.statistics.resourcesBought)
		totalBought += boughtAmount;
	var totalSold = 0;
	for each (var soldAmount in playerState.statistics.resourcesSold)
		totalSold += soldAmount;

	return Math.floor(totalSold > 0 ? (totalBought / totalSold) * 100 : 0) + "%";
}

function calculateTradeIncome(playerState, position)
{
	return playerState.statistics.tradeIncome;
}

function calculateMarketTeam(counters)
{
	for (var t in g_Teams)
	{
		if (t == -1)
			continue;

		for (var w in counters)
		{
			var teamTotal = "undefined";

			var total = {
				i : 0,
				o : 0
			};
			for (var p = 0; p < g_Teams[t]; ++p)
			{
				var caption = Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]").caption;
				// clean [Color=""], [/Color], white space, + and % for make the sum more easy
				caption = caption.replace(/\[([\w\' \\\"\/\=]*)\]|\s|\+|\%/g, "");

				if (w >= 4)
					total.i += (+caption);
				else
				{
					var splitCaption = caption.split("-");
					total.i += (+splitCaption[0]);
					total.o += (+splitCaption[1]);
				}
			}

			if (w >= 4)
				teamTotal = total.i +(w == 4 ? "%" : "");
			else
				teamTotal = INCOME_COLOR + '+' + total.i + '[/color] ' + OUTCOME_COLOR + '-' + total.o + '[/color]';

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}

function calculateVegetarianRatio(playerState, position)
{
	if (!teamMiscHelperData[playerState.team])
		teamMiscHelperData[playerState.team] = [];
	if (!teamMiscHelperData[playerState.team][position])
		teamMiscHelperData[playerState.team][position] = { "food": 0, "vegetarianFood": 0 };

	if (playerState.statistics.resourcesGathered.vegetarianFood && playerState.statistics.resourcesGathered.food)
	{
		teamMiscHelperData[playerState.team][position].food += playerState.statistics.resourcesGathered.food;
		teamMiscHelperData[playerState.team][position].vegetarianFood += playerState.statistics.resourcesGathered.vegetarianFood;
		return Math.floor((playerState.statistics.resourcesGathered.vegetarianFood / playerState.statistics.resourcesGathered.food) * 100) + "%";
	}
	else
		return 0 + "%";
}

function calculateFeminization(playerState, position)
{
	if (!teamMiscHelperData[playerState.team])
		teamMiscHelperData[playerState.team] = [];
	if (!teamMiscHelperData[playerState.team][position])
		teamMiscHelperData[playerState.team][position] = { "Female": 0, "Worker": 0 };

	if (playerState.statistics.unitsTrained.Worker && playerState.statistics.unitsTrained.Female)
	{
		teamMiscHelperData[playerState.team][position].Female = playerState.statistics.unitsTrained.Female;
		teamMiscHelperData[playerState.team][position].Worker = playerState.statistics.unitsTrained.Worker;
		return Math.floor((playerState.statistics.unitsTrained.Female / playerState.statistics.unitsTrained.Worker) * 100) + "%";
	}
	else
		return 0 + "%";
}

function calculateKillDeathRatio(playerState, position)
{
	if (!teamMiscHelperData[playerState.team])
		teamMiscHelperData[playerState.team] = [];
	if (!teamMiscHelperData[playerState.team][position])
		teamMiscHelperData[playerState.team][position] = { "enemyUnitsKilled": 0, "unitsLost": 0 };

	teamMiscHelperData[playerState.team][position].enemyUnitsKilled = playerState.statistics.enemyUnitsKilled.total;
	teamMiscHelperData[playerState.team][position].unitsLost = playerState.statistics.unitsLost.total;

	if (!playerState.statistics.enemyUnitsKilled.total)
		return DEFAULT_DECIMAL;
	if (!playerState.statistics.unitsLost.total)	// and enemyUnitsKilled.total > 0
		return INFINITE_SYMBOL; // infinity symbol

	return Math.round((playerState.statistics.enemyUnitsKilled.total / playerState.statistics.unitsLost.total)*100)/100;
}

function calculateMapExploration(playerState, position)
{
	if (!teamMiscHelperData[playerState.team])
		teamMiscHelperData[playerState.team] = [];

	teamMiscHelperData[playerState.team][position] = playerState.statistics.teamPercentMapExplored;
	return playerState.statistics.percentMapExplored + "%";
}

function calculateMapFinalControl(playerState, position)
{
	if (!teamMiscHelperData[playerState.team])
		teamMiscHelperData[playerState.team] = [];

	teamMiscHelperData[playerState.team][position] = playerState.statistics.teamPercentMapControlled;
	return playerState.statistics.percentMapControlled + "%";
}

function calculateMapPeakControl(playerState, position)
{
	if (!teamMiscHelperData[playerState.team])
		teamMiscHelperData[playerState.team] = [];

	teamMiscHelperData[playerState.team][position] = playerState.statistics.teamPeakPercentMapControlled;
	return playerState.statistics.peakPercentMapControlled + "%";
}

function calculateMiscellaneous(counters)
{
	for (var t in g_Teams)
	{
		if (t == -1)
			continue;

		for (var w in counters)
		{
			var teamTotal = "undefined";

			if (w == 0)
				teamTotal = (teamMiscHelperData[t][w].food == 0 ? "0" : Math.floor((teamMiscHelperData[t][w].vegetarianFood / teamMiscHelperData[t][w].food) * 100)) + "%";
			else if (w == 1)
				teamTotal = (teamMiscHelperData[t][w].Worker == 0 ? "0" : Math.floor((teamMiscHelperData[t][w].Female / teamMiscHelperData[t][w].Worker) * 100)) + "%";
			else if (w == 2)
			{
				if (!teamMiscHelperData[t][w].enemyUnitsKilled)
					teamTotal = DEFAULT_DECIMAL;
				else if (!teamMiscHelperData[t][w].unitsLost)	// and enemyUnitsKilled.total > 0
					teamTotal = INFINITE_SYMBOL; // infinity symbol
				else
					teamTotal = Math.round((teamMiscHelperData[t][w].enemyUnitsKilled / teamMiscHelperData[t][w].unitsLost)*100)/100;
			}
			else if (w >= 3)
				teamTotal = teamMiscHelperData[t][w] + "%";

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}
