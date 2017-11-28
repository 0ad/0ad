var g_TeamHelperData = [];

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

function getPlayerValuesPerTeam(team, index, type, counters, headings)
{
	let fn = counters[headings.map(heading => heading.identifier).indexOf(type) - 1].fn;
	return g_Teams[team].map(player => fn(g_GameData.sim.playerStates[player], index, type));
}

function updateCountersPlayer(playerState, counters, headings, idGUI, index)
{
	for (let n in counters)
	{
		let fn = counters[n].fn;
		Engine.GetGUIObjectByName(idGUI + "[" + n + "]").caption =
			formatSummaryValue(fn && fn(playerState, index, headings[+n + 1].identifier));
	}
}

function updateCountersTeam(teamFn, counters, headings, index)
{
	for (let team in g_Teams)
	{
		if (team == -1)
			continue;

		for (let n in counters)
			Engine.GetGUIObjectByName("valueDataTeam[" + team + "][" + n + "]").caption =
				formatSummaryValue(teamFn(team, index, headings[+n + 1].identifier, counters, headings));
	}
}

/**
 * Add two objects property-wise and writes the result to obj1.
 * So summaryAddObject([1, 2], [7, 42]) will result in [8, 44] and
 * summaryAddObject({ "f": 3, "o", 5 }, { "f": -1, "o", 42 }) will result in { "f": 2, "o", 47 }.
 *
 * @param {Object} obj1 - First summand object. This will be set to the sum of both.
 * @param {Object} obj2 - Second summand object.
 */
function summaryAddObject(obj1, obj2)
{
	for (let p in obj1)
		obj1[p] += obj2[p];
}

/**
 * The sum of all elements of an array. So summaryArraySum([1, 2]) will be 3 and
 * summaryArraySum([{ "f": 3, "o", 5 }, { "f": -1, "o", 42 }]) will be { "f": 2, "o", 47 }.
 *
 * @param {Array} array - The array to sum up.
 * @returns the sum of all elements.
 */
function summaryArraySum(array)
{
	return array.reduce((sum, val) => {
		if (typeof sum != "object")
			return sum + val;
		summaryAddObject(sum, val);
		return sum;
	});
}

function calculateTeamCounterDataHelper()
{
	for (let i = 0; i < g_PlayerCount; ++i)
	{
		let playerState = g_GameData.sim.playerStates[i + 1];

		if (!g_TeamHelperData[playerState.team])
		{
			g_TeamHelperData[playerState.team] = {};
			for (let value of ["food", "vegetarianFood", "femaleCitizen", "worker", "enemyUnitsKilled",
			                   "unitsLost", "mapControl", "mapControlPeak",
			                   "mapExploration", "totalBought", "totalSold"])
				g_TeamHelperData[playerState.team][value] = new Array(playerState.sequences.time.length).fill(0);
		}

		summaryAddObject(g_TeamHelperData[playerState.team].food, playerState.sequences.resourcesGathered.food);
		summaryAddObject(g_TeamHelperData[playerState.team].vegetarianFood, playerState.sequences.resourcesGathered.vegetarianFood);

		summaryAddObject(g_TeamHelperData[playerState.team].femaleCitizen, playerState.sequences.unitsTrained.FemaleCitizen);
		summaryAddObject(g_TeamHelperData[playerState.team].worker, playerState.sequences.unitsTrained.Worker);

		summaryAddObject(g_TeamHelperData[playerState.team].enemyUnitsKilled, playerState.sequences.enemyUnitsKilled.total);
		summaryAddObject(g_TeamHelperData[playerState.team].unitsLost, playerState.sequences.unitsLost.total);

		g_TeamHelperData[playerState.team].mapControl = playerState.sequences.teamPercentMapControlled;
		g_TeamHelperData[playerState.team].mapControlPeak = playerState.sequences.teamPeakPercentMapControlled;

		g_TeamHelperData[playerState.team].mapExploration = playerState.sequences.teamPercentMapExplored;

		for (let type in playerState.sequences.resourcesBought)
			summaryAddObject(g_TeamHelperData[playerState.team].totalBought, playerState.sequences.resourcesBought[type]);

		for (let type in playerState.sequences.resourcesSold)
			summaryAddObject(g_TeamHelperData[playerState.team].totalSold, playerState.sequences.resourcesSold[type]);
	}
}

function calculateEconomyScore(playerState, index)
{
	let total = 0;
	for (let type of g_ResourceData.GetCodes())
		total += playerState.sequences.resourcesGathered[type][index];

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

function calculateScoreTeam(team, index, type, counters, headings)
{
	if (type == "explorationScore")
		return g_TeamHelperData[team].mapExploration[index] * 10;
	if (type == "totalScore")
		return ["economyScore", "militaryScore", "explorationScore"].map(
			t => calculateScoreTeam(team, index, t, counters, headings)).reduce(
			(sum, value) => sum + value);

	return summaryArraySum(getPlayerValuesPerTeam(team, index, type, counters, headings));
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

function calculateBuildingsTeam(team, index, type, counters, headings)
{
	return summaryArraySum(getPlayerValuesPerTeam(team, index, type, counters, headings));
}

function calculateUnitsTeam(team, index, type, counters, headings)
{
	return summaryArraySum(getPlayerValuesPerTeam(team, index, type, counters, headings));
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

function calculateLivestockTrained(playerState, index)
{
	return playerState.sequences.unitsTrained.Domestic[index];
}

function calculateResourcesTeam(team, index, type, counters, headings)
{
	return summaryArraySum(getPlayerValuesPerTeam(team, index, type, counters, headings));
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

function calculateMarketTeam(team, index, type, counters, headings)
{
	if (type == "barterEfficency")
		return calculatePercent(g_TeamHelperData[team].totalBought[index], g_TeamHelperData[team].totalSold[index]);

	return summaryArraySum(getPlayerValuesPerTeam(team, index, type, counters, headings));
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

function calculateMiscellaneousTeam(team, index, type, counters, headings)
{
	if (type == "vegetarianRatio")
		return calculatePercent(g_TeamHelperData[team].vegetarianFood[index], g_TeamHelperData[team].food[index]);

	if (type == "feminization")
		return calculatePercent(g_TeamHelperData[team].femaleCitizen[index], g_TeamHelperData[team].worker[index]);

	if (type == "killDeath")
		return calculateRatio(g_TeamHelperData[team].enemyUnitsKilled[index], g_TeamHelperData[team].unitsLost[index]);

	if (type == "bribes")
		return summaryArraySum(getPlayerValuesPerTeam(team, index, type, counters, headings));

	return { "percent": g_TeamHelperData[team][type][index] };
}

function calculateBribes(playerState, index, type)
{
	return {
		"succeeded": playerState.sequences.successfulBribes[index],
		"failed": playerState.sequences.failedBribes[index]
	};
}
