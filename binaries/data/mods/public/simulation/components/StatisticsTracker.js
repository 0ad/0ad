function StatisticsTracker() {}

StatisticsTracker.prototype.Schema =
	"<a:component type='system'/><empty/>";

StatisticsTracker.prototype.Init = function()
{
	// units
	this.unitsTrained = 0;
	this.unitsLost = 0;
	this.enemyUnitsKilled = 0;
	//buildings
	this.buildingsConstructed = 0;
	this.buildingsLost = 0;
	this.enemyBuildingsDestroyed = 0;
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
		"enemyUnitsKilled": this.enemyUnitsKilled,
		"buildingsConstructed": this.buildingsConstructed,
		"buildingsLost": this.buildingsLost,
		"enemyBuildingsDestroyed": this.enemyBuildingsDestroyed,
		"civCentresBuilt": this.civCentresBuilt,
		"enemyCivCentresDestroyed": this.enemyCivCentresDestroyed,
		"resourcesGathered": this.resourcesGathered,
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
				this.enemyUnitsKilled++;
			if (targetIsStructure)
				this.enemyBuildingsDestroyed++;
			if (targetIsCivCentre)
				this.enemyCivCentresDestroyed++;
		}
	}
};

StatisticsTracker.prototype.LostEntity = function(lostEntity)
{
	var cmpLostEntityIdentity = Engine.QueryInterface(lostEntity, IID_Identity);
	if (cmpLostEntityIdentity)
	{
		var cmpFoundation = Engine.QueryInterface(lostEntity, IID_Foundation);
		// We want to deal only with real structures, not foundations
		var lostEntityIsStructure = cmpLostEntityIdentity.HasClass("Structure") && cmpFoundation == null;
		var lostEntityIsDomesticAnimal = cmpLostEntityIdentity.HasClass("Animal") && cmpLostEntityIdentity.HasClass("Domestic");
		// Don't count domestic animals as units
		var lostEntityIsUnit = cmpLostEntityIdentity.HasClass("Unit") && !lostEntityIsDomesticAnimal;

		if (lostEntityIsUnit)
			this.unitsLost++;
		if (lostEntityIsStructure)
			this.buildingsLost++;
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
