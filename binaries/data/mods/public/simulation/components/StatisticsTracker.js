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
	}
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
 		"resourcesGathered": this.resourcesGathered
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
		var classes = cmpTargetEntityIdentity.GetClassesList();
		// we want to deal only with real structures, not foundations
		var cmpFoundation = Engine.QueryInterface(targetEntity, IID_Foundation);
		var targetIsStructure = classes.indexOf("Structure") != -1 && cmpFoundation == null;
		var targetIsUnit = classes.indexOf("Unit") != -1;
		var targetIsCivCentre = classes.indexOf("CivCentre") != -1;
				
		var cmpTargetOwnership = Engine.QueryInterface(targetEntity, IID_Ownership);
		
		// don't increase counters if target player is gaia (player 0)
		if (cmpTargetOwnership.GetOwner() != 0)
		{
			if (targetIsUnit) this.enemyUnitsKilled++;
			if (targetIsStructure) this.enemyBuildingsDestroyed++;
			if (targetIsCivCentre) this.enemyCivCentresDestroyed++;
		}
	}
};

StatisticsTracker.prototype.LostEntity = function(lostEntity)
{
	var cmpLostEntityIdentity = Engine.QueryInterface(lostEntity, IID_Identity);
	if (cmpLostEntityIdentity)
	{
		var classes = cmpLostEntityIdentity.GetClassesList();
		// we want to deal only with real structures, not foundations
		var cmpFoundation = Engine.QueryInterface(lostEntity, IID_Foundation);
		var lostEntityIsStructure = classes.indexOf("Structure") != -1 && cmpFoundation == null;
		var lostEntityIsUnit = classes.indexOf("Unit") != -1;

		if (lostEntityIsUnit) this.unitsLost++;
		if (lostEntityIsStructure) this.buildingsLost++;
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

Engine.RegisterComponentType(IID_StatisticsTracker, "StatisticsTracker", StatisticsTracker);
