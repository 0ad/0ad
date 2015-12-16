// FUNCTIONS FOR CALCULATING SCORES
var g_TeamMiscHelperData = [];

function resetDataHelpers()
{
	g_TeamMiscHelperData = [];
}

function updateCountersPlayer(playerState, counters, idGUI)
{
	for (let w in counters)
	{
		let fn = counters[w].fn;
		Engine.GetGUIObjectByName(idGUI + "[" + w + "]").caption = fn && fn(playerState, w);
	}
}

function calculateEconomyScore(playerState, position)
{
	let total = 0;
	for each (let res in playerState.statistics.resourcesGathered)
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
	for (let t in g_Teams)
	{
		if (t == -1)
			continue;

		for (let w in counters)
		{
			let total = 0;
			for (let p = 0; p < g_Teams[t]; ++p)
				total += (+Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]").caption);

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = total;
		}
	}
}

function calculateBuildings(playerState, position)
{
	let type = g_BuildingsTypes[position];
	return g_TrainedColor + playerState.statistics.buildingsConstructed[type] + '[/color] / ' +
		g_LostColor + playerState.statistics.buildingsLost[type] + '[/color] / ' +
		g_KilledColor + playerState.statistics.enemyBuildingsDestroyed[type] + '[/color]';
}

function calculateColorsTeam(counters)
{
	for (let t in g_Teams)
	{
		if (t == -1)
			continue;

		for (let w in counters)
		{
			let total = {
				"c": 0,
				"l": 0,
				"d": 0
			};

			for (let p = 0; p < g_Teams[t]; ++p)
			{
				let caption = Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]").caption;
				// clean [Color=""], [/Color] and white space for make the sum more easy
				caption = caption.replace(/\[([\w\' \\\"\/\=]*)\]|\s/g, "");

				let splitCaption = caption.split("/");

				total.c += (+splitCaption[0]);
				total.l += (+splitCaption[1]);
				total.d += (+splitCaption[2]);
			}

			let teamTotal = g_TrainedColor + total.c + '[/color] / ' +
				g_LostColor + total.l + '[/color] / ' + g_KilledColor + total.d + '[/color]';

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}

function calculateUnits(playerState, position)
{
	let type = g_UnitsTypes[position];
	return g_TrainedColor + playerState.statistics.unitsTrained[type] + '[/color] / ' +
		g_LostColor + playerState.statistics.unitsLost[type] + '[/color] / ' +
		g_KilledColor + playerState.statistics.enemyUnitsKilled[type] + '[/color]';
}

function calculateResources(playerState, position)
{
	let type = g_ResourcesTypes[position];
	return g_IncomeColor + playerState.statistics.resourcesGathered[type] + '[/color] / ' +
		g_OutcomeColor + (playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type]) + '[/color]';
}

function calculateTotalResources(playerState, position)
{
	let totalGathered = 0;
	let totalUsed = 0;

	for each (let type in g_ResourcesTypes)
	{
		totalGathered += playerState.statistics.resourcesGathered[type];
		totalUsed += playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type];
	}

	return g_IncomeColor + totalGathered + '[/color] / ' + g_OutcomeColor + totalUsed + '[/color]';
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
	return g_IncomeColor + playerState.statistics.tributesSent + "[/color] / " +
	       g_OutcomeColor + playerState.statistics.tributesReceived + "[/color]";
}

function calculateResourcesTeam(counters)
{
	for (let t in g_Teams)
	{
		if (t == -1)
			continue;

		for (let w in counters)
		{
			let teamTotal = "undefined";

			let total = {
				i : 0,
				o : 0
			};

			for (let p = 0; p < g_Teams[t]; ++p)
			{
				let caption = Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]").caption;
				// clean [Color=""], [/Color] and white space for make the sum more easy
				caption = caption.replace(/\[([\w\' \\\"\/\=]*)\]|\s/g, "");

				if (w >= 6)
					total.i += (+caption);
				else
				{
					let splitCaption = caption.split("/");

					total.i += (+splitCaption[0]);
					total.o += (+splitCaption[1]);
				}
			}

			if (w >= 6)
				teamTotal = total.i;
			else
				teamTotal = g_IncomeColor + total.i + "[/color] / " + g_OutcomeColor + total.o + "[/color]";

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}

function calculateResourceExchanged(playerState, position)
{
	let type = g_ResourcesTypes[position];
	return g_IncomeColor + '+' + playerState.statistics.resourcesBought[type] + '[/color] ' +
		g_OutcomeColor + '-' + playerState.statistics.resourcesSold[type] + '[/color]';
}

function calculateBatteryEfficiency(playerState, position)
{
	let totalBought = 0;
	for each (let boughtAmount in playerState.statistics.resourcesBought)
		totalBought += boughtAmount;
	let totalSold = 0;
	for each (let soldAmount in playerState.statistics.resourcesSold)
		totalSold += soldAmount;

	return Math.floor(totalSold > 0 ? (totalBought / totalSold) * 100 : 0) + "%";
}

function calculateTradeIncome(playerState, position)
{
	return playerState.statistics.tradeIncome;
}

function calculateMarketTeam(counters)
{
	for (let t in g_Teams)
	{
		if (t == -1)
			continue;

		for (let w in counters)
		{
			let teamTotal = "undefined";

			let total = {
				"i": 0,
				"o": 0
			};

			for (let p = 0; p < g_Teams[t]; ++p)
			{
				let caption = Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]").caption;
				// clean [Color=""], [/Color], white space, + and % for make the sum more easy
				caption = caption.replace(/\[([\w\' \\\"\/\=]*)\]|\s|\+|\%/g, "");

				if (w >= 4)
					total.i += (+caption);
				else
				{
					let splitCaption = caption.split("-");
					total.i += (+splitCaption[0]);
					total.o += (+splitCaption[1]);
				}
			}

			if (w >= 4)
				teamTotal = total.i +(w == 4 ? "%" : "");
			else
				teamTotal = g_IncomeColor + '+' + total.i + '[/color] ' + g_IncomeColor + '-' + total.o + '[/color]';

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}

function calculateVegetarianRatio(playerState, position)
{
	if (!g_TeamMiscHelperData[playerState.team])
		g_TeamMiscHelperData[playerState.team] = [];

	if (!g_TeamMiscHelperData[playerState.team][position])
		g_TeamMiscHelperData[playerState.team][position] = { "food": 0, "vegetarianFood": 0 };

	if (playerState.statistics.resourcesGathered.vegetarianFood && playerState.statistics.resourcesGathered.food)
	{
		g_TeamMiscHelperData[playerState.team][position].food += playerState.statistics.resourcesGathered.food;
		g_TeamMiscHelperData[playerState.team][position].vegetarianFood += playerState.statistics.resourcesGathered.vegetarianFood;
		return Math.floor((playerState.statistics.resourcesGathered.vegetarianFood / playerState.statistics.resourcesGathered.food) * 100) + "%";
	}
	else
		return 0 + "%";
}

function calculateFeminization(playerState, position)
{
	if (!g_TeamMiscHelperData[playerState.team])
		g_TeamMiscHelperData[playerState.team] = [];

	if (!g_TeamMiscHelperData[playerState.team][position])
		g_TeamMiscHelperData[playerState.team][position] = { "Female": 0, "Worker": 0 };

	if (playerState.statistics.unitsTrained.Worker && playerState.statistics.unitsTrained.Female)
	{
		g_TeamMiscHelperData[playerState.team][position].Female = playerState.statistics.unitsTrained.Female;
		g_TeamMiscHelperData[playerState.team][position].Worker = playerState.statistics.unitsTrained.Worker;
		return Math.floor((playerState.statistics.unitsTrained.Female / playerState.statistics.unitsTrained.Worker) * 100) + "%";
	}
	else
		return 0 + "%";
}

function calculateKillDeathRatio(playerState, position)
{
	if (!g_TeamMiscHelperData[playerState.team])
		g_TeamMiscHelperData[playerState.team] = [];

	if (!g_TeamMiscHelperData[playerState.team][position])
		g_TeamMiscHelperData[playerState.team][position] = { "enemyUnitsKilled": 0, "unitsLost": 0 };

	g_TeamMiscHelperData[playerState.team][position].enemyUnitsKilled = playerState.statistics.enemyUnitsKilled.total;
	g_TeamMiscHelperData[playerState.team][position].unitsLost = playerState.statistics.unitsLost.total;

	if (!playerState.statistics.enemyUnitsKilled.total)
		return g_DefaultDecimal;

	if (!playerState.statistics.unitsLost.total)	// and enemyUnitsKilled.total > 0
		return g_InfiniteSymbol; // infinity symbol

	return Math.round((playerState.statistics.enemyUnitsKilled.total / playerState.statistics.unitsLost.total)*100)/100;
}

function calculateMapExploration(playerState, position)
{
	if (!g_TeamMiscHelperData[playerState.team])
		g_TeamMiscHelperData[playerState.team] = [];

	g_TeamMiscHelperData[playerState.team][position] = playerState.statistics.teamPercentMapExplored;

	return playerState.statistics.percentMapExplored + "%";
}

function calculateMapFinalControl(playerState, position)
{
	if (!g_TeamMiscHelperData[playerState.team])
		g_TeamMiscHelperData[playerState.team] = [];

	g_TeamMiscHelperData[playerState.team][position] = playerState.statistics.teamPercentMapControlled;

	return playerState.statistics.percentMapControlled + "%";
}

function calculateMapPeakControl(playerState, position)
{
	if (!g_TeamMiscHelperData[playerState.team])
		g_TeamMiscHelperData[playerState.team] = [];

	g_TeamMiscHelperData[playerState.team][position] = playerState.statistics.teamPeakPercentMapControlled;

	return playerState.statistics.peakPercentMapControlled + "%";
}

function calculateMiscellaneous(counters)
{
	for (let t in g_Teams)
	{
		if (t == -1)
			continue;

		for (let w in counters)
		{
			let teamTotal = "undefined";

			if (w == 0)
				teamTotal = (g_TeamMiscHelperData[t][w].food == 0 ? "0" : Math.floor((g_TeamMiscHelperData[t][w].vegetarianFood / g_TeamMiscHelperData[t][w].food) * 100)) + "%";
			else if (w == 1)
				teamTotal = (g_TeamMiscHelperData[t][w].Worker == 0 ? "0" : Math.floor((g_TeamMiscHelperData[t][w].Female / g_TeamMiscHelperData[t][w].Worker) * 100)) + "%";
			else if (w == 2)
			{
				if (!g_TeamMiscHelperData[t][w].enemyUnitsKilled)
					teamTotal = g_DefaultDecimal;
				else if (!g_TeamMiscHelperData[t][w].unitsLost)	// and enemyUnitsKilled.total > 0
					teamTotal = g_InfiniteSymbol; // infinity symbol
				else
					teamTotal = Math.round((g_TeamMiscHelperData[t][w].enemyUnitsKilled / g_TeamMiscHelperData[t][w].unitsLost)*100)/100;
			}
			else if (w >= 3)
				teamTotal = g_TeamMiscHelperData[t][w] + "%";

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}
