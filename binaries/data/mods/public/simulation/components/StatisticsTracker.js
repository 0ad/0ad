function StatisticsTracker() {}

const g_UpdateSequenceInterval = 30 * 1000;

StatisticsTracker.prototype.Schema =
	"<empty/>";

StatisticsTracker.prototype.Init = function()
{
	this.unitsClasses = [
		"Infantry",
		"Worker",
		"FemaleCitizen",
		"Cavalry",
		"Champion",
		"Hero",
		"Siege",
		"Ship",
		"Trader"
	];
	this.unitsTrained = {
		"Infantry": 0,
		"Worker": 0,
		"FemaleCitizen": 0,
		"Cavalry": 0,
		"Champion": 0,
		"Hero": 0,
		"Siege": 0,
		"Ship": 0,
		"Trader": 0,
		"total": 0
	};
	this.domesticUnitsTrainedValue = 0;
	this.unitsLost = {
		"Infantry": 0,
		"Worker": 0,
		"FemaleCitizen": 0,
		"Cavalry": 0,
		"Champion": 0,
		"Hero": 0,
		"Siege": 0,
		"Ship": 0,
		"Trader": 0,
		"total": 0
	};
	this.unitsLostValue = 0;
	this.enemyUnitsKilled = {
		"Infantry": 0,
		"Worker": 0,
		"FemaleCitizen": 0,
		"Cavalry": 0,
		"Champion": 0,
		"Hero": 0,
		"Siege": 0,
		"Ship": 0,
		"Trader": 0,
		"total": 0
	};
	this.enemyUnitsKilledValue = 0;
	this.unitsCaptured = {
		"Infantry": 0,
		"Worker": 0,
		"FemaleCitizen": 0,
		"Cavalry": 0,
		"Champion": 0,
		"Hero": 0,
		"Siege": 0,
		"Ship": 0,
		"Trader": 0,
		"total": 0
	};
	this.unitsCapturedValue = 0;

	this.buildingsClasses = [
		"House",
		"Economic",
		"Outpost",
		"Military",
		"Fortress",
		"CivCentre",
		"Wonder"
	];
	this.buildingsConstructed = {
		"House": 0,
		"Economic": 0,
		"Outpost": 0,
		"Military": 0,
		"Fortress": 0,
		"CivCentre": 0,
		"Wonder": 0,
		"total": 0
	};
	this.buildingsLost = {
		"House": 0,
		"Economic": 0,
		"Outpost": 0,
		"Military": 0,
		"Fortress": 0,
		"CivCentre": 0,
		"Wonder": 0,
		"total": 0
	};
	this.buildingsLostValue = 0;
	this.enemyBuildingsDestroyed = {
		"House": 0,
		"Economic": 0,
		"Outpost": 0,
		"Military": 0,
		"Fortress": 0,
		"CivCentre": 0,
		"Wonder": 0,
		"total": 0
	};
	this.enemyBuildingsDestroyedValue = 0;
	this.buildingsCaptured = {
		"House": 0,
		"Economic": 0,
		"Outpost": 0,
		"Military": 0,
		"Fortress": 0,
		"CivCentre": 0,
		"Wonder": 0,
		"total": 0
	};
	this.buildingsCapturedValue = 0;

	this.resourcesGathered = {
		"vegetarianFood": 0
	};
	this.resourcesUsed = {};
	this.resourcesSold = {};
	this.resourcesBought = {};
	for (let res of Resources.GetCodes())
	{
		this.resourcesGathered[res] = 0;
		this.resourcesUsed[res] = 0;
		this.resourcesSold[res] = 0;
		this.resourcesBought[res] = 0;
	}

	this.tributesSent = 0;
	this.tributesReceived = 0;
	this.tradeIncome = 0;
	this.treasuresCollected = 0;
	this.lootCollected = 0;
	this.peakPercentMapControlled = 0;
	this.teamPeakPercentMapControlled = 0;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.updateTimer = cmpTimer.SetInterval(
		this.entity, IID_StatisticsTracker, "UpdateSequences", 0, g_UpdateSequenceInterval);
};

/**
 * Returns a subset of statistics that will be added to the simulation state,
 * thus called each turn. Basic statistics should not contain data that would
 * be expensive to compute.
 *
 * Note: as of now, nothing in the game needs that, but some AIs developed by
 * modders need it in the API.
 */
StatisticsTracker.prototype.GetBasicStatistics = function()
{
	return {
		"resourcesGathered": this.resourcesGathered,
		"percentMapExplored": this.GetPercentMapExplored()
	};
};

StatisticsTracker.prototype.GetStatistics = function()
{
	return {
		"unitsTrained": this.unitsTrained,
		"domesticUnitsTrainedValue": this.domesticUnitsTrainedValue,
		"unitsLost": this.unitsLost,
		"unitsLostValue": this.unitsLostValue,
		"enemyUnitsKilled": this.enemyUnitsKilled,
		"enemyUnitsKilledValue": this.enemyUnitsKilledValue,
		"unitsCaptured": this.unitsCaptured,
		"unitsCapturedValue": this.unitsCapturedValue,
		"buildingsConstructed": this.buildingsConstructed,
		"buildingsLost": this.buildingsLost,
		"buildingsLostValue": this.buildingsLostValue,
		"enemyBuildingsDestroyed": this.enemyBuildingsDestroyed,
		"enemyBuildingsDestroyedValue": this.enemyBuildingsDestroyedValue,
		"buildingsCaptured": this.buildingsCaptured,
		"buildingsCapturedValue": this.buildingsCapturedValue,
		"resourcesGathered": this.resourcesGathered,
		"resourcesUsed": this.resourcesUsed,
		"resourcesSold": this.resourcesSold,
		"resourcesBought": this.resourcesBought,
		"tributesSent": this.tributesSent,
		"tributesReceived": this.tributesReceived,
		"tradeIncome": this.tradeIncome,
		"treasuresCollected": this.treasuresCollected,
		"lootCollected": this.lootCollected,
		"percentMapExplored": this.GetPercentMapExplored(),
		"teamPercentMapExplored": this.GetTeamPercentMapExplored(),
		"percentMapControlled": this.GetPercentMapControlled(),
		"teamPercentMapControlled": this.GetTeamPercentMapControlled(),
		"peakPercentMapControlled": this.peakPercentMapControlled,
		"teamPeakPercentMapControlled": this.teamPeakPercentMapControlled
	};
};

StatisticsTracker.prototype.GetSequences = function()
{
	if (!this.sequences)
		return {};

	let ret = clone(this.sequences);
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	ret.time.push(cmpTimer.GetTime() / 1000);
	this.PushValue(this.GetStatistics(), ret);
	return ret;
};

/**
 * Used to print statistics for non-visual autostart games.
 * @return The player's statistics as a JSON string beautified with some indentations.
 */
StatisticsTracker.prototype.GetStatisticsJSON = function()
{
	let cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);

	let playerStatistics = {
		"playerID": cmpPlayer.GetPlayerID(),
		"playerState": cmpPlayer.GetState(),
		"statistics": this.GetStatistics()
	};

	return JSON.stringify(playerStatistics, null, "\t");
};

/**
 * Increments counter associated with certain entity/counter and type of given entity.
 * @param cmpIdentity - the entity identity component.
 * @param counter - the name of the counter to increment (e.g. "unitsTrained").
 * @param type - the type of the counter (e.g. "workers").
 */
StatisticsTracker.prototype.CounterIncrement = function(cmpIdentity, counter, type)
{
	var classes = cmpIdentity.GetClassesList();
	if (!classes)
		return;

	if (classes.indexOf(type) != -1)
		++this[counter][type];
};

/**
 * Counts the total number of units trained as well as an individual count for
 * each unit type. Based on templates.
 */
StatisticsTracker.prototype.IncreaseTrainedUnitsCounter = function(trainedUnit)
{
	let cmpUnitEntityIdentity = Engine.QueryInterface(trainedUnit, IID_Identity);

	if (!cmpUnitEntityIdentity)
		return;

	let cmpCost = Engine.QueryInterface(trainedUnit, IID_Cost);
	let costs = cmpCost && cmpCost.GetResourceCosts();

	for (let type of this.unitsClasses)
		this.CounterIncrement(cmpUnitEntityIdentity, "unitsTrained", type);

	++this.unitsTrained.total;

	if (cmpUnitEntityIdentity.HasClass("Domestic") && costs)
		for (let type in costs)
			this.domesticUnitsTrainedValue += costs[type];

};

/**
 * Counts the total number of buildings constructed as well as an individual count for
 * each building type. Based on templates.
 */
StatisticsTracker.prototype.IncreaseConstructedBuildingsCounter = function(constructedBuilding)
{
	var cmpBuildingEntityIdentity = Engine.QueryInterface(constructedBuilding, IID_Identity);

	if (!cmpBuildingEntityIdentity)
		return;

	for (let type of this.buildingsClasses)
		this.CounterIncrement(cmpBuildingEntityIdentity, "buildingsConstructed", type);

	++this.buildingsConstructed.total;
};

StatisticsTracker.prototype.KilledEntity = function(targetEntity)
{
	var cmpTargetEntityIdentity = Engine.QueryInterface(targetEntity, IID_Identity);
	if (!cmpTargetEntityIdentity)
		return;

	var cmpCost = Engine.QueryInterface(targetEntity, IID_Cost);
	var costs = cmpCost && cmpCost.GetResourceCosts();

	var cmpTargetOwnership = Engine.QueryInterface(targetEntity, IID_Ownership);

	// Ignore gaia
	if (cmpTargetOwnership.GetOwner() == 0)
		return;

	if (cmpTargetEntityIdentity.HasClass("Unit") && !cmpTargetEntityIdentity.HasClass("Domestic"))
	{
		for (let type of this.unitsClasses)
			this.CounterIncrement(cmpTargetEntityIdentity, "enemyUnitsKilled", type);

		++this.enemyUnitsKilled.total;

		if (costs)
			for (let type in costs)
				this.enemyUnitsKilledValue += costs[type];
	}

	let cmpFoundation = Engine.QueryInterface(targetEntity, IID_Foundation);
	if (cmpTargetEntityIdentity.HasClass("Structure") && !cmpFoundation)
	{
		for (let type of this.buildingsClasses)
			this.CounterIncrement(cmpTargetEntityIdentity, "enemyBuildingsDestroyed", type);

		++this.enemyBuildingsDestroyed.total;

		if (costs)
			for (let type in costs)
				this.enemyBuildingsDestroyedValue += costs[type];
	}
};

StatisticsTracker.prototype.LostEntity = function(lostEntity)
{
	var cmpLostEntityIdentity = Engine.QueryInterface(lostEntity, IID_Identity);
	if (!cmpLostEntityIdentity)
		return;

	var cmpCost = Engine.QueryInterface(lostEntity, IID_Cost);
	var costs = cmpCost && cmpCost.GetResourceCosts();

	if (cmpLostEntityIdentity.HasClass("Unit") && !cmpLostEntityIdentity.HasClass("Domestic"))
	{
		for (let type of this.unitsClasses)
			this.CounterIncrement(cmpLostEntityIdentity, "unitsLost", type);

		++this.unitsLost.total;

		if (costs)
			for (let type in costs)
				this.unitsLostValue += costs[type];
	}

	let cmpFoundation = Engine.QueryInterface(lostEntity, IID_Foundation);
	if (cmpLostEntityIdentity.HasClass("Structure") && !cmpFoundation)
	{
		for (let type of this.buildingsClasses)
			this.CounterIncrement(cmpLostEntityIdentity, "buildingsLost", type);

		++this.buildingsLost.total;

		if (costs)
			for (let type in costs)
				this.buildingsLostValue += costs[type];
	}
};

StatisticsTracker.prototype.CapturedEntity = function(capturedEntity)
{
	let cmpCapturedEntityIdentity = Engine.QueryInterface(capturedEntity, IID_Identity);
	if (!cmpCapturedEntityIdentity)
		return;

	let cmpCost = Engine.QueryInterface(capturedEntity, IID_Cost);
	let costs = cmpCost && cmpCost.GetResourceCosts();

	if (cmpCapturedEntityIdentity.HasClass("Unit"))
	{
		for (let type of this.unitsClasses)
			this.CounterIncrement(cmpCapturedEntityIdentity, "unitsCaptured", type);

		++this.unitsCaptured.total;

		if (costs)
			for (let type in costs)
				this.unitsCapturedValue += costs[type];
	}

	if (cmpCapturedEntityIdentity.HasClass("Structure"))
	{
		for (let type of this.buildingsClasses)
			this.CounterIncrement(cmpCapturedEntityIdentity, "buildingsCaptured", type);

		++this.buildingsCaptured.total;

		if (costs)
			for (let type in costs)
				this.buildingsCapturedValue += costs[type];
	}
};

/**
 * @param {string} type - generic type of resource.
 * @param {number} amount - amount of resource, whick should be added.
 * @param {string} specificType - specific type of resource.
 */
StatisticsTracker.prototype.IncreaseResourceGatheredCounter = function(type, amount, specificType)
{
	this.resourcesGathered[type] += amount;

	if (type == "food" && (specificType == "fruit" || specificType == "grain"))
		this.resourcesGathered.vegetarianFood += amount;
};

/**
 * @param {string} type - generic type of resource.
 * @param {number} amount - amount of resource, which should be added.
 */
StatisticsTracker.prototype.IncreaseResourceUsedCounter = function(type, amount)
{
	this.resourcesUsed[type] += amount;
};

StatisticsTracker.prototype.IncreaseTreasuresCollectedCounter = function()
{
	++this.treasuresCollected;
};

StatisticsTracker.prototype.IncreaseLootCollectedCounter = function(amount)
{
	for (let type in amount)
		this.lootCollected += amount[type];
};

StatisticsTracker.prototype.IncreaseResourcesSoldCounter = function(type, amount)
{
	this.resourcesSold[type] += amount;
};

StatisticsTracker.prototype.IncreaseResourcesBoughtCounter = function(type, amount)
{
	this.resourcesBought[type] += amount;
};

StatisticsTracker.prototype.IncreaseTributesSentCounter = function(amount)
{
	this.tributesSent += amount;
};

StatisticsTracker.prototype.IncreaseTributesReceivedCounter = function(amount)
{
	this.tributesReceived += amount;
};

StatisticsTracker.prototype.IncreaseTradeIncomeCounter = function(amount)
{
	this.tradeIncome += amount;
};

StatisticsTracker.prototype.GetPercentMapExplored = function()
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	return cmpRangeManager.GetPercentMapExplored(cmpPlayer.GetPlayerID());
};

/**
 * Note: cmpRangeManager.GetUnionPercentMapExplored computes statistics from scratch!
 * As a consequence, this function should not be called too often.
 */
StatisticsTracker.prototype.GetTeamPercentMapExplored = function()
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer)
		return 0;

	var team = cmpPlayer.GetTeam();
	// If teams are not locked, this statistic won't be displayed, so don't bother computing
	if (team == -1 || !cmpPlayer.GetLockTeams())
		return cmpRangeManager.GetPercentMapExplored(cmpPlayer.GetPlayerID());

	var teamPlayers = [];
	for (var i = 1; i < cmpPlayerManager.GetNumPlayers(); ++i)
	{
		let cmpOtherPlayer = QueryPlayerIDInterface(i);
		if (cmpOtherPlayer && cmpOtherPlayer.GetTeam() == team)
			teamPlayers.push(i);
	}

	return cmpRangeManager.GetUnionPercentMapExplored(teamPlayers);
};

StatisticsTracker.prototype.GetPercentMapControlled = function()
{
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	var cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
	if (!cmpPlayer || !cmpTerritoryManager)
		return 0;

	return cmpTerritoryManager.GetTerritoryPercentage(cmpPlayer.GetPlayerID());
};

StatisticsTracker.prototype.GetTeamPercentMapControlled = function()
{
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
	if (!cmpPlayer || !cmpTerritoryManager)
		return 0;

	var team = cmpPlayer.GetTeam();
	if (team == -1 || !cmpPlayer.GetLockTeams())
		return cmpTerritoryManager.GetTerritoryPercentage(cmpPlayer.GetPlayerID());

	var teamPercent = 0;
	for (let i = 1; i < cmpPlayerManager.GetNumPlayers(); ++i)
	{
		let cmpOtherPlayer = QueryPlayerIDInterface(i);
		if (cmpOtherPlayer && cmpOtherPlayer.GetTeam() == team)
			teamPercent += cmpTerritoryManager.GetTerritoryPercentage(i);
	}

	return teamPercent;
};

StatisticsTracker.prototype.OnTerritoriesChanged = function(msg)
{
	var newPercent = this.GetPercentMapControlled();
	if (newPercent > this.peakPercentMapControlled)
		this.peakPercentMapControlled = newPercent;

	newPercent = this.GetTeamPercentMapControlled();
	if (newPercent > this.teamPeakPercentMapControlled)
		this.teamPeakPercentMapControlled = newPercent;
};

/**
 * Adds the values of fromData to the end of the arrays of toData.
 * If toData misses the needed array, one will be created.
 *
 * @param fromData - an object of values or a value.
 * @param toData - an object of arrays or an array.
**/
StatisticsTracker.prototype.PushValue = function(fromData, toData)
{
	if (typeof fromData == "object")
		for (let prop in fromData)
		{
			if (typeof toData[prop] != "object")
				toData[prop] = [fromData[prop]];
			else
				this.PushValue(fromData[prop], toData[prop]);
		}
	else
		toData.push(fromData);
};

StatisticsTracker.prototype.UpdateSequences = function()
{
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);

	// Don't do this on Init, because GetStatistics doesn't work in this state of the game
	// This is because the simulation hasn't totally started/initialized and we query some simulation values
	if (!this.sequences)
	{
		this.sequences = clone(this.GetStatistics());
		this.sequences.time = [];
	}

	this.sequences.time.push(cmpTimer.GetTime() / 1000);
	this.PushValue(this.GetStatistics(), this.sequences);
};

Engine.RegisterComponentType(IID_StatisticsTracker, "StatisticsTracker", StatisticsTracker);
