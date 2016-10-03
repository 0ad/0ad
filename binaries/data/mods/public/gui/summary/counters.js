var g_TeamHelperData = [];

function resetDataHelpers()
{
	g_TeamHelperData = [];
}

function formatTrained(trained, killed, lost)
{
	return g_TrainedColor + trained + '[/color] / ' +
		g_KilledColor + killed + '[/color] / ' +
		g_LostColor + lost + '[/color]';
}

function formatCaptured(constructed, destroyed, captured, lost)
{
	return g_TrainedColor + constructed + '[/color] / ' +
		g_KilledColor + destroyed + '[/color]\n' +
		g_CapturedColor + captured + '[/color] / ' +
		g_LostColor + lost + '[/color]\n';
}

function formatIncome(income, outcome)
{
	return g_IncomeColor + income + '[/color] / ' +
		g_OutcomeColor + outcome + '[/color]';
}

function formatPercent(divident, divisor)
{
	if (!divisor)
		return "0%";

	return Math.floor(100 * divident / divisor) + "%";
}

function formatRatio(divident, divisor)
{
	if (!divident)
		return "0.00";

	if (!divisor)
		return g_InfiniteSymbol;

	return Math.round(divident / divisor * 100) / 100;
}

/**
 * Remove color tags, whitespace, + and % to read numerical values from the GUI objects.
 * Remove \n only when removeLineFeed == true
 * TODO: access the data directly instead of this ugly hack.
 */
function cleanGUICaption(team, player, counter, removeLineFeed = true)
{
	let caption = Engine.GetGUIObjectByName("valueDataTeam[" + team + "][" + player + "][" + counter + "]").caption;
	if (removeLineFeed)
		return caption.replace(/\[([\w\' \\\"\/\=]*)\]|\+|\%|\s/g, "");
	else
		return caption.replace(/\[([\w\' \\\"\/\=]*)\]|[\t\r \f]/g, "");
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
		playerState.statistics.enemyBuildingsDestroyedValue +
		playerState.statistics.buildingsCapturedValue) / 10);
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
	return formatCaptured(
		playerState.statistics.buildingsConstructed[type],
		playerState.statistics.enemyBuildingsDestroyed[type],
		playerState.statistics.buildingsCaptured[type],
		playerState.statistics.buildingsLost[type]);
}

function calculateBuildingsTeam(counters)
{
	for (let t in g_Teams)
	{
		if (t == -1)
			continue;

		for (let w in counters)
		{
			let total = {
				"constructed" : 0,
				"destroyed" : 0,
				"captured" : 0,
				"lost" : 0
			};

			for (let p = 0; p < g_Teams[t]; ++p)
			{
				let splitCaption = cleanGUICaption(t, p, w, false).split("\n");
				let first = splitCaption[0].split("/");
				let second = splitCaption[1].split("/");

				total.constructed += +first[0];
				total.destroyed += +first[1];
				total.captured += +second[0];
				total.lost += +second[1];
			}

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption =
				formatCaptured(total.constructed, total.destroyed, total.captured, total.lost);
		}
	}
}

function calculateUnitsTeam(counters)
{
	for (let t in g_Teams)
	{
		if (t == -1)
			continue;

		for (let w in counters)
		{
			let total =
			{
				"trained": 0,
				"killed": 0,
				"captured" : 0,
				"lost": 0
			};

			for (let p = 0; p < g_Teams[t]; ++p)
			{
				if (w == 0 || w == 6)
				{
					let splitCaption = cleanGUICaption(t, p, w, false).split("\n");
					let first = splitCaption[0].split("/");
					let second = splitCaption[1].split("/");

					total.trained += +first[0];
					total.killed += +first[1];
					total.captured += +second[0];
					total.lost += +second[1];
				}
				else
				{
					let splitCaption = cleanGUICaption(t, p, w).split("/");
					total.trained += +splitCaption[0];
					total.killed += +splitCaption[1];
					total.lost += +splitCaption[2];
				}
			}

			let formattedCaption = "";

			if (w == 0 || w == 6)
				formattedCaption = formatCaptured(total.trained, total.killed, total.captured, total.lost);
			else
				formattedCaption = formatTrained(total.trained, total.killed, total.lost);

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = formattedCaption;
		}
	}
}

function calculateUnitsWithCaptured(playerState, position)
{
	let type = g_UnitsTypes[position];

	return formatCaptured(
		playerState.statistics.unitsTrained[type],
		playerState.statistics.enemyUnitsKilled[type],
		playerState.statistics.unitsCaptured[type],
		playerState.statistics.unitsLost[type]);
}

function calculateUnits(playerState, position)
{
	let type = g_UnitsTypes[position];

	return formatTrained(
		playerState.statistics.unitsTrained[type],
		playerState.statistics.enemyUnitsKilled[type],
		playerState.statistics.unitsLost[type]);
}

function calculateResources(playerState, position)
{
	let type = g_ResourcesTypes[position];

	return formatIncome(
		playerState.statistics.resourcesGathered[type],
		playerState.statistics.resourcesUsed[type] - playerState.statistics.resourcesSold[type]);
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

	return formatIncome(totalGathered, totalUsed);
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
	return formatIncome(
		playerState.statistics.tributesSent,
		playerState.statistics.tributesReceived);
}

function calculateResourcesTeam(counters)
{
	for (let t in g_Teams)
	{
		if (t == -1)
			continue;

		for (let w in counters)
		{
			let total = {
				"income": 0,
				"outcome": 0
			};

			for (let p = 0; p < g_Teams[t]; ++p)
			{
				let caption = cleanGUICaption(t, p, w);

				if (w >= 6)
					total.income += +caption;
				else
				{
					let splitCaption = caption.split("/");

					total.income += +splitCaption[0];
					total.outcome += +splitCaption[1];
				}
			}

			let teamTotal;
			if (w >= 6)
				teamTotal = total.income;
			else
				teamTotal = formatIncome(total.income, total.outcome);

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}

function calculateResourceExchanged(playerState, position)
{
	let type = g_ResourcesTypes[position];

	return formatIncome(
		playerState.statistics.resourcesBought[type],
		playerState.statistics.resourcesSold[type]);
}

function calculateBarterEfficiency(playerState)
{
	let totalBought = 0;
	let totalSold = 0;

	for (let type in playerState.statistics.resourcesBought)
		totalBought += playerState.statistics.resourcesBought[type];

	for (let type in playerState.statistics.resourcesSold)
		totalSold += playerState.statistics.resourcesSold[type];

	return formatPercent(totalBought, totalSold);
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
			let total = {
				"income": 0,
				"outcome": 0
			};

			for (let p = 0; p < g_Teams[t]; ++p)
			{
				let caption = cleanGUICaption(t, p, w);

				if (w >= 4)
					total.income += +caption;
				else
				{
					let splitCaption = caption.split("/");
					total.income += +splitCaption[0];
					total.outcome += +splitCaption[1];
				}
			}

			let teamTotal;
			if (w == 4)
				teamTotal = formatPercent(g_TeamHelperData[t].totalBought, g_TeamHelperData[t].totalSold);
			else if (w > 4)
				teamTotal = total.income;
			else
				teamTotal = formatIncome(total.income, total.outcome);

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}

function calculateVegetarianRatio(playerState)
{
	return formatPercent(
		playerState.statistics.resourcesGathered.vegetarianFood,
		playerState.statistics.resourcesGathered.food);
}

function calculateFeminization(playerState)
{
	return formatPercent(
		playerState.statistics.unitsTrained.Female,
		playerState.statistics.unitsTrained.Worker);
}

function calculateKillDeathRatio(playerState)
{
	return formatRatio(
		playerState.statistics.enemyUnitsKilled.total,
		playerState.statistics.unitsLost.total);
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
			let teamTotal;

			if (w == 0)
				teamTotal = formatPercent(g_TeamHelperData[t].vegetarianFood, g_TeamHelperData[t].food);
			else if (w == 1)
				teamTotal = formatPercent(g_TeamHelperData[t].female, g_TeamHelperData[t].worker);
			else if (w == 2)
				teamTotal = formatRatio(g_TeamHelperData[t].enemyUnitsKilled, g_TeamHelperData[t].unitsLost);
			else if (w == 3)
				teamTotal = g_TeamHelperData[t].percentMapExplored + "%";
			else if (w == 4)
				teamTotal = g_TeamHelperData[t].peakPercentMapControlled + "%";
			else if (w == 5)
				teamTotal = g_TeamHelperData[t].percentMapControlled + "%";

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = teamTotal;
		}
	}
}
