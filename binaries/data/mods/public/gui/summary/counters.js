var g_TeamHelperData = [];

function resetDataHelpers()
{
	g_TeamHelperData = [];
}

function calculatePercent(divident, divisor)
{
	return { "percent": divisor ? Math.floor(100 * divident / divisor) : 0 };
}

function calculateRatio(divident, divisor)
{
	return divident ? +((divident / divisor).toFixed(2)) : 0;
}

function formatSummaryValue(values)
{
	if (typeof values != "object")
		return values === Infinity ? g_InfinitySymbol : values;

	let ret = "";
	for (let type in values)
		ret += (g_SummaryTypes[type].color ?
			'[color="' + g_SummaryTypes[type].color + '"]' + values[type] + '[/color]' :
			values[type]) + g_SummaryTypes[type].postfix;
	return ret;
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

function updateCountersPlayer(playerState, counters, headings, idGUI)
{
	let index = playerState.sequences.time.length - 1;
	for (let w in counters)
	{
		let fn = counters[w].fn;
		Engine.GetGUIObjectByName(idGUI + "[" + w + "]").caption = formatSummaryValue(fn && fn(playerState, index, headings[+w+1].identifier));
	}
}

/**
 * Add two arrays element-wise. So addArray([1, 2], [7, 42]) will result in [8, 44].
 *
 * @param {Array} array1 - first summand array.
 * @param {Array} array2 - second summand array.
 * @returns {Array} the element-wise sum of array1 and array2.
 */
function addArray(array1, array2)
{
	array1 = array1.map((value, index) => value + array2[index]);
}

// Updates g_TeamHelperData by appending some data from playerState
function calculateTeamCounters(playerState)
{
	if (!g_TeamHelperData[playerState.team])
	{
		g_TeamHelperData[playerState.team] = {};
		for (let value of ["food", "vegetarianFood", "femaleCitizen", "worker", "enemyUnitsKilled",
		                   "unitsLost", "percentMapControlled", "peakPercentMapControlled",
		                   "percentMapExplored", "totalBought", "totalSold"])
			g_TeamHelperData[playerState.team][value] = new Array(playerState.sequences.time.length).fill(0);
	}

	addArray(g_TeamHelperData[playerState.team].food, playerState.sequences.resourcesGathered.food);
	addArray(g_TeamHelperData[playerState.team].vegetarianFood, playerState.sequences.resourcesGathered.vegetarianFood);

	addArray(g_TeamHelperData[playerState.team].femaleCitizen, playerState.sequences.unitsTrained.FemaleCitizen);
	addArray(g_TeamHelperData[playerState.team].worker, playerState.sequences.unitsTrained.Worker);

	addArray(g_TeamHelperData[playerState.team].enemyUnitsKilled, playerState.sequences.enemyUnitsKilled.total);
	addArray(g_TeamHelperData[playerState.team].unitsLost, playerState.sequences.unitsLost.total);

	g_TeamHelperData[playerState.team].percentMapControlled = playerState.sequences.teamPercentMapControlled;
	g_TeamHelperData[playerState.team].peakPercentMapControlled = playerState.sequences.teamPeakPercentMapControlled;

	g_TeamHelperData[playerState.team].percentMapExplored = playerState.sequences.teamPercentMapExplored;

	for (let type in playerState.sequences.resourcesBought)
		addArray(g_TeamHelperData[playerState.team].totalBought, playerState.sequences.resourcesBought[type]);

	for (let type in playerState.sequences.resourcesSold)
		addArray(g_TeamHelperData[playerState.team].totalSold, playerState.sequences.resourcesSold[type]);
}

function calculateEconomyScore(playerState, index)
{
	let total = 0;
	for (let type of g_ResourceData.GetCodes())
		total += playerState.sequences.resourcesGathered[type][index];

	// Subtract costs for sheep/goats/pigs to get the net food gain for corralling
	total -= playerState.sequences.domesticUnitsTrainedValue[index];

	total += playerState.sequences.tradeIncome[index];
	return Math.round(total / 10);
}

function calculateMilitaryScore(playerState, index)
{
	return Math.round((playerState.sequences.enemyUnitsKilledValue[index] +
		playerState.sequences.unitsCapturedValue[index] +
		playerState.sequences.enemyBuildingsDestroyedValue[index] +
		playerState.sequences.buildingsCapturedValue[index]) / 10);
}

function calculateExplorationScore(playerState, index)
{
	return playerState.sequences.percentMapExplored[index] * 10;
}

function calculateScoreTotal(playerState, index)
{
	return calculateEconomyScore(playerState, index) +
		calculateMilitaryScore(playerState, index) +
		calculateExplorationScore(playerState, index);
}

function calculateScoreTeam(counters, index)
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
				total = g_TeamHelperData[t].percentMapExplored[index] * 10;
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

function calculateBuildings(playerState, index, type)
{
	return {
		"constructed": playerState.sequences.buildingsConstructed[type][index],
		"destroyed": playerState.sequences.enemyBuildingsDestroyed[type][index],
		"captured": playerState.sequences.buildingsCaptured[type][index],
		"lost": playerState.sequences.buildingsLost[type][index]
	};
}

function calculateBuildingsTeam(counters, index)
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
				formatSummaryValue(total);
		}
	}
}

function calculateUnitsTeam(counters, index)
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
				let splitCaption = cleanGUICaption(t, p, w, false).split("\n");
				let first = splitCaption[0].split("/");
				total.trained += +first[0];
				total.killed += +first[1];

				if (w == 0 || w == 6)
				{
					let second = splitCaption[1].split("/");
					total.captured += +second[0];
					total.lost += +second[1];
				}
				else
					total.lost += +splitCaption[1];
			}

			if (w != 0 && w != 6)
				delete total.captured;

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = formatSummaryValue(total);
		}
	}
}

function calculateUnitsWithCaptured(playerState, index, type)
{
	return {
		"trained": playerState.sequences.unitsTrained[type][index],
		"killed": playerState.sequences.enemyUnitsKilled[type][index],
		"captured": playerState.sequences.unitsCaptured[type][index],
		"lost": playerState.sequences.unitsLost[type][index]
	};
}

function calculateUnits(playerState, index, type)
{
	return {
		"trained": playerState.sequences.unitsTrained[type][index],
		"killed": playerState.sequences.enemyUnitsKilled[type][index],
		"lost": playerState.sequences.unitsLost[type][index]
	};
}

function calculateResources(playerState, index, type)
{
	return {
		"gathered": playerState.sequences.resourcesGathered[type][index],
		"used": playerState.sequences.resourcesUsed[type][index] - playerState.sequences.resourcesSold[type][index]
	};
}

function calculateTotalResources(playerState, index)
{
	let totalGathered = 0;
	let totalUsed = 0;

	for (let type of g_ResourceData.GetCodes())
	{
		totalGathered += playerState.sequences.resourcesGathered[type][index];
		totalUsed += playerState.sequences.resourcesUsed[type][index] - playerState.sequences.resourcesSold[type][index];
	}

	return { "gathered": totalGathered, "used": totalUsed };
}

function calculateTreasureCollected(playerState, index)
{
	return playerState.sequences.treasuresCollected[index];
}

function calculateLootCollected(playerState, index)
{
	return playerState.sequences.lootCollected[index];
}

function calculateTributeSent(playerState, index)
{
	return {
		"sent": playerState.sequences.tributesSent[index],
		"received": playerState.sequences.tributesReceived[index]
	};
}

function calculateResourcesTeam(counters, index)
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
			else if (w == 5)
				teamTotal = { "sent": total.income, "received": total.outcome };
			else
				teamTotal = { "gathered": total.income, "used": total.outcome };

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = formatSummaryValue(teamTotal);
		}
	}
}

function calculateResourceExchanged(playerState, index, type)
{
	return {
		"bought": playerState.sequences.resourcesBought[type][index],
		"sold": playerState.sequences.resourcesSold[type][index]
	};
}

function calculateBarterEfficiency(playerState, index)
{
	let totalBought = 0;
	let totalSold = 0;

	for (let type in playerState.sequences.resourcesBought)
		totalBought += playerState.sequences.resourcesBought[type][index];

	for (let type in playerState.sequences.resourcesSold)
		totalSold += playerState.sequences.resourcesSold[type][index];

	return calculatePercent(totalBought, totalSold);
}

function calculateTradeIncome(playerState, index)
{
	return playerState.sequences.tradeIncome[index];
}

function calculateMarketTeam(counters, index)
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
				teamTotal = calculatePercent(g_TeamHelperData[t].totalBought[index], g_TeamHelperData[t].totalSold[index]);
			else if (w > 4)
				teamTotal = total.income;
			else
				teamTotal = total;

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = formatSummaryValue(teamTotal);
		}
	}
}

function calculateVegetarianRatio(playerState, index)
{
	return calculatePercent(
		playerState.sequences.resourcesGathered.vegetarianFood[index],
		playerState.sequences.resourcesGathered.food[index]);
}

function calculateFeminization(playerState, index)
{
	return calculatePercent(
		playerState.sequences.unitsTrained.FemaleCitizen[index],
		playerState.sequences.unitsTrained.Worker[index]);
}

function calculateKillDeathRatio(playerState, index)
{
	return calculateRatio(
		playerState.sequences.enemyUnitsKilled.total[index],
		playerState.sequences.unitsLost.total[index]);
}

function calculateMapExploration(playerState, index)
{
	return { "percent": playerState.sequences.percentMapExplored[index] };
}

function calculateMapFinalControl(playerState, index)
{
	return { "percent": playerState.sequences.percentMapControlled[index] };
}

function calculateMapPeakControl(playerState, index)
{
	return { "percent": playerState.sequences.peakPercentMapControlled[index] };
}

function calculateMiscellaneousTeam(counters, index)
{
	for (let t in g_Teams)
	{
		if (t == -1)
			continue;

		for (let w in counters)
		{
			let teamTotal;

			if (w == 0)
				teamTotal = calculatePercent(g_TeamHelperData[t].vegetarianFood[index], g_TeamHelperData[t].food[index]);
			else if (w == 1)
				teamTotal = calculatePercent(g_TeamHelperData[t].femaleCitizen[index], g_TeamHelperData[t].worker[index]);
			else if (w == 2)
				teamTotal = calculateRatio(g_TeamHelperData[t].enemyUnitsKilled[index], g_TeamHelperData[t].unitsLost[index]);
			else if (w == 3)
				teamTotal = { "percent": g_TeamHelperData[t].percentMapExplored[index] };
			else if (w == 4)
				teamTotal = { "percent": g_TeamHelperData[t].peakPercentMapControlled[index] };
			else if (w == 5)
				teamTotal = { "percent": g_TeamHelperData[t].percentMapControlled[index] };

			Engine.GetGUIObjectByName("valueDataTeam[" + t + "][" + w + "]").caption = formatSummaryValue(teamTotal);
		}
	}
}
