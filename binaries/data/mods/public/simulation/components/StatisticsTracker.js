function StatisticsTracker() {}

StatisticsTracker.prototype.Schema =
	"<a:component type='system'/><empty/>";

StatisticsTracker.prototype.Init = function()
{
	// units
	this.unitsTrained = 0;
	this.unitsLost = 0;
	this.unitsLostValue = 0;
	this.enemyUnitsKilled = 0;
	this.enemyUnitsKilledValue = 0;
	//buildings
	this.buildingsConstructed = 0;
	this.buildingsLost = 0;
	this.buildingsLostValue = 0;
	this.enemyBuildingsDestroyed = 0;
	this.enemyBuildingsDestroyedValue = 0;
	// civ centres
	this.civCentresBuilt = 0;
	this.enemyCivCentresDestroyed = 0;
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
	this.tradeIncome = 0;
	this.treasuresCollected = 0;
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
		"civCentresBuilt": this.civCentresBuilt,
		"enemyCivCentresDestroyed": this.enemyCivCentresDestroyed,
		"resourcesGathered": this.resourcesGathered,
		"resourcesUsed": this.resourcesUsed,
		"resourcesSold": this.resourcesSold,
		"resourcesBought": this.resourcesBought,
		"tradeIncome": this.tradeIncome,
		"treasuresCollected": this.treasuresCollected,
		"percentMapExplored": this.GetPercentMapExplored()
	};
};

StatisticsTracker.prototype.IncreaseTrainedUnitsCounter = function()
{
	return this.unitsTrained++;
};

StatisticsTracker.prototype.IncreaseConstructedBuildingsCounter = function()
{
	return this.buildingsConstructed++;
};

StatisticsTracker.prototype.IncreaseBuiltCivCentresCounter = function()
{
	return this.civCentresBuilt++;
};

StatisticsTracker.prototype.KilledEntity = function(targetEntity)
{
	var cmpTargetEntityIdentity = Engine.QueryInterface(targetEntity, IID_Identity);
	var cmpCost = Engine.QueryInterface(targetEntity, IID_Cost);
	var costs = cmpCost.GetResourceCosts();
	if (cmpTargetEntityIdentity)
	{
		var cmpFoundation = Engine.QueryInterface(targetEntity, IID_Foundation);
		// We want to deal only with real structures, not foundations
		var targetIsStructure = cmpTargetEntityIdentity.HasClass("Structure") && cmpFoundation == null;
		var targetIsDomesticAnimal = cmpTargetEntityIdentity.HasClass("Animal") && cmpTargetEntityIdentity.HasClass("Domestic");
		// Don't count domestic animals as units
		var targetIsUnit = cmpTargetEntityIdentity.HasClass("Unit") && !targetIsDomesticAnimal;
		var targetIsCivCentre = cmpTargetEntityIdentity.HasClass("CivCentre");

		var cmpTargetOwnership = Engine.QueryInterface(targetEntity, IID_Ownership);

		// Don't increase counters if target player is gaia (player 0)
		if (cmpTargetOwnership.GetOwner() != 0)
		{
			if (targetIsUnit)
			{
				this.enemyUnitsKilled++;
				for (var r in costs)
				{
					this.enemyUnitsKilledValue += costs[r];
				}
			}	
			if (targetIsStructure)
			{
				this.enemyBuildingsDestroyed++;
				for (var r in costs)
				{
					this.enemyBuildingsDestroyedValue += costs[r];
				}
			}
			if (targetIsCivCentre && targetIsStructure)
				this.enemyCivCentresDestroyed++;
		}
	}
};

StatisticsTracker.prototype.LostEntity = function(lostEntity)
{
	var cmpLostEntityIdentity = Engine.QueryInterface(lostEntity, IID_Identity);
	var cmpCost = Engine.QueryInterface(lostEntity, IID_Cost);
	var costs = cmpCost.GetResourceCosts();
	if (cmpLostEntityIdentity)
	{
		var cmpFoundation = Engine.QueryInterface(lostEntity, IID_Foundation);
		// We want to deal only with real structures, not foundations
		var lostEntityIsStructure = cmpLostEntityIdentity.HasClass("Structure") && cmpFoundation == null;
		var lostEntityIsDomesticAnimal = cmpLostEntityIdentity.HasClass("Animal") && cmpLostEntityIdentity.HasClass("Domestic");
		// Don't count domestic animals as units
		var lostEntityIsUnit = cmpLostEntityIdentity.HasClass("Unit") && !lostEntityIsDomesticAnimal;

		if (lostEntityIsUnit)
		{
			this.unitsLost++;
			for (var r in costs)
			{
				this.unitsLostValue += costs[r];
			}	
		}	
		if (lostEntityIsStructure)
		{
			this.buildingsLost++;
			for (var r in costs)
			{
				this.buildingsLostValue += costs[r];
			}
		}
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
		this.resourcesGathered["vegetarianFood"] += amount;
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
	return this.treasuresCollected++;
};

StatisticsTracker.prototype.IncreaseResourcesSoldCounter = function(type, amount)
{
	this.resourcesSold[type] += amount;
}

StatisticsTracker.prototype.IncreaseResourcesBoughtCounter = function(type, amount)
{
	this.resourcesBought[type] += amount;
}

StatisticsTracker.prototype.IncreaseTradeIncomeCounter = function(amount)
{
	this.tradeIncome += amount;
}

StatisticsTracker.prototype.GetPercentMapExplored = function()
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var cmpPlayer = Engine.QueryInterface(this.entity, IID_Player);
	return cmpRangeManager.GetPercentMapExplored(cmpPlayer.GetPlayerID());
};

Engine.RegisterComponentType(IID_StatisticsTracker, "StatisticsTracker", StatisticsTracker);
