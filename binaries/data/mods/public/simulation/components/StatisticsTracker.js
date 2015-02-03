function StatisticsTracker() {}

StatisticsTracker.prototype.Schema =
	"<a:component type='system'/><empty/>";

StatisticsTracker.prototype.Init = function()
{
	// units
	this.unitsClasses = [
		"Infantry",
		"Worker",
		"Female",
		"Cavalry",
		"Champion",
		"Hero",
		"Ship"
	];	
	this.unitsTrained = {
		"Infantry": 0,
		"Worker": 0,
		"Female": 0,
		"Cavalry": 0,
		"Champion": 0,
		"Hero": 0,
		"Ship": 0,
		"total": 0
	};
	this.unitsLost = {
		"Infantry": 0,
		"Worker": 0,
		"Female": 0,
		"Cavalry": 0,
		"Champion": 0,
		"Hero": 0,
		"Ship": 0,
		"total": 0
	};
	this.unitsLostValue = 0;
	this.enemyUnitsKilled = {
		"Infantry": 0,
		"Worker": 0,
		"Female": 0,
		"Cavalry": 0,
		"Champion": 0,
		"Hero": 0,
		"Ship": 0,
		"total": 0
	};
	this.enemyUnitsKilledValue = 0;
	// buildings
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
	// resources
	this.resourcesGathered = {
		"food": 0,
		"wood": 0,
		"metal": 0,
		"stone": 0,
		"vegetarianFood": 0
	};
	this.resourcesUsed = {
		"food": 0,
		"wood": 0,
		"metal": 0,
		"stone": 0
	};
	this.resourcesSold = {
		"food": 0,
		"wood": 0,
		"metal": 0,
		"stone": 0
	};
	this.resourcesBought = {
		"food": 0,
		"wood": 0,
		"metal": 0,
		"stone": 0
	};
	this.tributesSent = 0;
	this.tributesReceived = 0;
	this.tradeIncome = 0;
	this.treasuresCollected = 0;
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
		"unitsLost": this.unitsLost,
		"unitsLostValue": this.unitsLostValue,
		"enemyUnitsKilled": this.enemyUnitsKilled,
		"enemyUnitsKilledValue": this.enemyUnitsKilledValue,
		"buildingsConstructed": this.buildingsConstructed,
		"buildingsLost": this.buildingsLost,
		"buildingsLostValue": this.buildingsLostValue,
		"enemyBuildingsDestroyed": this.enemyBuildingsDestroyed,
		"enemyBuildingsDestroyedValue": this.enemyBuildingsDestroyedValue,
		"resourcesGathered": this.resourcesGathered,
		"resourcesUsed": this.resourcesUsed,
		"resourcesSold": this.resourcesSold,
		"resourcesBought": this.resourcesBought,
		"tributesSent": this.tributesSent,
		"tributesReceived": this.tributesReceived,
		"tradeIncome": this.tradeIncome,
		"treasuresCollected": this.treasuresCollected,
		"percentMapExplored": this.GetPercentMapExplored(),
		"teamPercentMapExplored": this.GetTeamPercentMapExplored()
	};
};

/**
 * Increments counter associated with certain entity/counter and type of given entity.
 * @param cmpIdentity The entity identity component
 * @param counter The name of the counter to increment (e.g. "unitsTrained")
 * @param type The type of the counter (e.g. "workers")
 */
StatisticsTracker.prototype.CounterIncrement = function(cmpIdentity, counter, type)
{
	var classes = cmpIdentity.GetClassesList();
	if (!classes)
		return;
	if (classes.indexOf(type) != -1)
		this[counter][type]++;
};

/** 
 * Counts the total number of units trained as well as an individual count for 
 * each unit type. Based on templates.
 * @param trainedUnit The unit that has been trained 
 */ 
StatisticsTracker.prototype.IncreaseTrainedUnitsCounter = function(trainedUnit)
{
	var cmpUnitEntityIdentity = Engine.QueryInterface(trainedUnit, IID_Identity);

	if (!cmpUnitEntityIdentity)
		return;

	for each (var type in this.unitsClasses)
		this.CounterIncrement(cmpUnitEntityIdentity, "unitsTrained", type);

	this.unitsTrained.total++;
};

/** 
 * Counts the total number of buildings constructed as well as an individual count for 
 * each building type. Based on templates.
 * @param constructedBuilding The building that has been constructed 
 */ 
StatisticsTracker.prototype.IncreaseConstructedBuildingsCounter = function(constructedBuilding)
{
	var cmpBuildingEntityIdentity = Engine.QueryInterface(constructedBuilding, IID_Identity);
		
	if (!cmpBuildingEntityIdentity)
		return;

	for each(var type in this.buildingsClasses)
		this.CounterIncrement(cmpBuildingEntityIdentity, "buildingsConstructed", type);

	this.buildingsConstructed.total++;
};

StatisticsTracker.prototype.KilledEntity = function(targetEntity)
{
	var cmpTargetEntityIdentity = Engine.QueryInterface(targetEntity, IID_Identity);
	var cmpCost = Engine.QueryInterface(targetEntity, IID_Cost);
	var costs = cmpCost.GetResourceCosts();
	if (!cmpTargetEntityIdentity)
		return;

	var cmpFoundation = Engine.QueryInterface(targetEntity, IID_Foundation);
	// We want to deal only with real structures, not foundations
	var targetIsStructure = cmpTargetEntityIdentity.HasClass("Structure") && cmpFoundation == null;
	var targetIsDomesticAnimal = cmpTargetEntityIdentity.HasClass("Animal") && cmpTargetEntityIdentity.HasClass("Domestic");
	// Don't count domestic animals as units
	var targetIsUnit = cmpTargetEntityIdentity.HasClass("Unit") && !targetIsDomesticAnimal;

	var cmpTargetOwnership = Engine.QueryInterface(targetEntity, IID_Ownership);
    
	// Don't increase counters if target player is gaia (player 0)
	if (cmpTargetOwnership.GetOwner() == 0)
		return;

	if (targetIsUnit)
	{
		for each (var type in this.unitsClasses)
			this.CounterIncrement(cmpTargetEntityIdentity, "enemyUnitsKilled", type);

		this.enemyUnitsKilled.total++;
		
		for each (var cost in costs)
			this.enemyUnitsKilledValue += cost;
	}	
	if (targetIsStructure)
	{
		for each (var type in this.buildingsClasses)
			this.CounterIncrement(cmpTargetEntityIdentity, "enemyBuildingsDestroyed", type);

		this.enemyBuildingsDestroyed.total++;
		
		for each (var cost in costs)
			this.enemyBuildingsDestroyedValue += cost;
	}
};

StatisticsTracker.prototype.LostEntity = function(lostEntity)
{
	var cmpLostEntityIdentity = Engine.QueryInterface(lostEntity, IID_Identity);
	var cmpCost = Engine.QueryInterface(lostEntity, IID_Cost);
	var costs = cmpCost.GetResourceCosts();
	if (!cmpLostEntityIdentity)
		return;
	
	var cmpFoundation = Engine.QueryInterface(lostEntity, IID_Foundation);
	// We want to deal only with real structures, not foundations
	var lostEntityIsStructure = cmpLostEntityIdentity.HasClass("Structure") && cmpFoundation == null;
	var lostEntityIsDomesticAnimal = cmpLostEntityIdentity.HasClass("Animal") && cmpLostEntityIdentity.HasClass("Domestic");
	// Don't count domestic animals as units
	var lostEntityIsUnit = cmpLostEntityIdentity.HasClass("Unit") && !lostEntityIsDomesticAnimal;

	if (lostEntityIsUnit)
	{
		for each (var type in this.unitsClasses)
			this.CounterIncrement(cmpLostEntityIdentity, "unitsLost", type);

		this.unitsLost.total++;
		
		for each (var cost in costs)
			this.unitsLostValue += cost;	
	}	
	if (lostEntityIsStructure)
	{
		for each (var type in this.buildingsClasses)
			this.CounterIncrement(cmpLostEntityIdentity, "buildingsLost", type);

		this.buildingsLost.total++;
		
		for each (var cost in costs)
			this.buildingsLostValue += cost;
	}
};

/**
 * @param type Generic type of resource (string)
 * @param amount Amount of resource, whick should be added (integer)
 * @param specificType Specific type of resource (string, optional)
 */
StatisticsTracker.prototype.IncreaseResourceGatheredCounter = function(type, amount, specificType)
{
	this.resourcesGathered[type] += amount;
	
	if (type == "food" && (specificType == "fruit" || specificType == "grain"))
		this.resourcesGathered.vegetarianFood += amount;
};

/**
 * @param type Generic type of resource (string)
 * @param amount Amount of resource, whick should be added (integer)
 */
StatisticsTracker.prototype.IncreaseResourceUsedCounter = function(type, amount)
{
	this.resourcesUsed[type] += amount;
};

StatisticsTracker.prototype.IncreaseTreasuresCollectedCounter = function()
{
	this.treasuresCollected++;
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
		let cmpOtherPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(i), IID_Player);
		if (cmpOtherPlayer && cmpOtherPlayer.GetTeam() == team)
			teamPlayers.push(i);
	}

	return cmpRangeManager.GetUnionPercentMapExplored(teamPlayers);
};

Engine.RegisterComponentType(IID_StatisticsTracker, "StatisticsTracker", StatisticsTracker);
