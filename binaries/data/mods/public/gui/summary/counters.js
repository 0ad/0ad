// FUNCTIONS FOR CALCULATING SCORES
var g_TeamHelperData = [];

function resetDataHelpers()
{
	g_TeamHelperData = [];
}

function updateCountersPlayer(playerState, counters, idGUI)
{
	for (let w in counters)
	{
		let fn = counters[w].fn;
		Engine.GetGUIObjectByName(idGUI + "[" + w + "]").caption = fn && fn(playerState, w);
	}
}

// Updates g_TeamHelperData by appending some data from playerState
function calculateTeamCounters(playerState)
{
	if (!g_TeamHelperData[playerState.team])
		g_TeamHelperData[playerState.team] = {
			"food": 0,
			"vegetarianFood": 0,
			"female": 0,
			"worker": 0,
			"enemyUnitsKilled": 0,
			"unitsLost": 0,
			"percentMapControlled": 0,
			"peakPercentMapControlled": 0,
			"percentMapExplored": 0,
			"totalBought": 0,
			"totalSold": 0
		};

	g_TeamHelperData[playerState.team].food += playerState.statistics.resourcesGathered.food;
	g_TeamHelperData[playerState.team].vegetarianFood += playerState.statistics.resourcesGathered.vegetarianFood;

	g_TeamHelperData[playerState.team].female += playerState.statistics.unitsTrained.Female;
	g_TeamHelperData[playerState.team].worker += playerState.statistics.unitsTrained.Worker;

	g_TeamHelperData[playerState.team].enemyUnitsKilled += playerState.statistics.enemyUnitsKilled.total;
	g_TeamHelperData[playerState.team].unitsLost += playerState.statistics.unitsLost.total;

	g_TeamHelperData[playerState.team].percentMapControlled = playerState.statistics.teamPercentMapControlled;
	g_TeamHelperData[playerState.team].peakPercentMapControlled = playerState.statistics.teamPeakPercentMapControlled;

	g_TeamHelperData[playerState.team].percentMapExplored = playerState.statistics.teamPercentMapExplored;

	for (let type in playerState.statistics.resourcesBought)
		g_TeamHelperData[playerState.team].totalBought += playerState.statistics.resourcesBought[type];

	for (let type in playerState.statistics.resourcesSold)
		g_TeamHelperData[playerState.team].totalSold += playerState.statistics.resourcesSold[type];
}

function calculateEconomyScore(playerState)
{
	let total = 0;
	for (let type in playerState.statistics.resourcesGathered)
		total += playerState.statistics.resourcesGathered[type];

	return Math.round(total / 10);
}

function calculateMilitaryScore(playerState)
{
	return Math.round((playerState.statistics.enemyUnitsKilledValue +
		playerState.statistics.enemyBuildingsDestroyedValue) / 10);
}

function calculateExplorationScore(playerState)
{
	return playerState.statistics.percentMapExplored * 10;
}

function calculateScoreTotal(playerState)
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

		let teamTotalScore = 0;
		for (let w in counters)
		{
			let total = 0;

			if (w == 2)	// Team exploration score (not additive)
				total = g_TeamHelperData[t].percentMapExplored * 10;
			else
				for (let p = 0; p < g_Teams[t]; ++p)
					total += +Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + p + "][" + w + "]").caption;

			if (w < 3)
			{
				teamTotalScore += total;
				Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = total;
			}
			else
				Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotalScore;
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

function calculateTotalResources(playerState)
{
	let totalGathered = 0;
	let totalUsed = 0;

	for (let type of g_ResourcesTypes)
	{
		totalGathered += playerState.statistics.resourcesGathered[type];
		totalUsed += playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type];
	}

	return g_IncomeColor + totalGathered + '[/color] / ' + g_OutcomeColor + totalUsed + '[/color]';
}

function calculateTreasureCollected(playerState)
{
	return playerState.statistics.treasuresCollected;
}

function calculateLootCollected(playerState)
{
	return playerState.statistics.lootCollected;
}

function calculateTributeSent(playerState)
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

function calculateBarterEfficiency(playerState)
{
	let totalBought = 0;
	let totalSold = 0;

	for (let type in playerState.statistics.resourcesBought)
		totalBought += playerState.statistics.resourcesBought[type];

	for (let type in playerState.statistics.resourcesSold)
		totalSold += playerState.statistics.resourcesSold[type];

	return Math.floor(totalSold > 0 ? (totalBought / totalSold) * 100 : 0) + "%";
}

function calculateTradeIncome(playerState)
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

			if (w == 4)
				teamTotal = Math.floor(g_TeamHelperData[t].totalSold > 0 ? (g_TeamHelperData[t].totalBought / g_TeamHelperData[t].totalSold) * 100 : 0) + "%"
			else if (w > 4)
				teamTotal = total.i;
			else
				teamTotal = g_IncomeColor + '+' + total.i + '[/color] ' + g_OutcomeColor + '-' + total.o + '[/color]';

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}

function calculateVegetarianRatio(playerState)
{
	if (playerState.statistics.resourcesGathered.vegetarianFood && playerState.statistics.resourcesGathered.food)
		return Math.floor((playerState.statistics.resourcesGathered.vegetarianFood / playerState.statistics.resourcesGathered.food) * 100) + "%";
	else
		return 0 + "%";
}

function calculateFeminization(playerState)
{
	if (playerState.statistics.unitsTrained.Worker && playerState.statistics.unitsTrained.Female)
		return Math.floor((playerState.statistics.unitsTrained.Female / playerState.statistics.unitsTrained.Worker) * 100) + "%";
	else
		return 0 + "%";
}

function calculateKillDeathRatio(playerState)
{
	if (!playerState.statistics.enemyUnitsKilled.total)
		return g_DefaultDecimal;

	if (!playerState.statistics.unitsLost.total)	// and enemyUnitsKilled.total > 0
		return g_InfiniteSymbol; // infinity symbol

	return Math.round((playerState.statistics.enemyUnitsKilled.total / playerState.statistics.unitsLost.total)*100)/100;
}

function calculateMapExploration(playerState)
{
	return playerState.statistics.percentMapExplored + "%";
}

function calculateMapFinalControl(playerState)
{
	return playerState.statistics.percentMapControlled + "%";
}

function calculateMapPeakControl(playerState)
{
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
				teamTotal = (g_TeamHelperData[t].food == 0 ? "0" : Math.floor((g_TeamHelperData[t].vegetarianFood / g_TeamHelperData[t].food) * 100)) + "%";
			else if (w == 1)
				teamTotal = (g_TeamHelperData[t].worker == 0 ? "0" : Math.floor((g_TeamHelperData[t].female / g_TeamHelperData[t].worker) * 100)) + "%";
			else if (w == 2)
			{
				if (!g_TeamHelperData[t].enemyUnitsKilled)
					teamTotal = g_DefaultDecimal;
				else if (!g_TeamHelperData[t].unitsLost)	// and enemyUnitsKilled.total > 0
					teamTotal = g_InfiniteSymbol; // infinity symbol
				else
					teamTotal = Math.round((g_TeamHelperData[t].enemyUnitsKilled / g_TeamHelperData[t].unitsLost) * 100) / 100;
			}
			else if (w == 3)
				teamTotal = g_TeamHelperData[t].percentMapExplored + "%";
			else if (w == 4)
				teamTotal = g_TeamHelperData[t].percentMapControlled + "%";
			else if (w == 5)
				teamTotal = g_TeamHelperData[t].peakPercentMapControlled + "%";

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}
