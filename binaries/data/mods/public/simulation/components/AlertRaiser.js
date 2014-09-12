function AlertRaiser() {}

AlertRaiser.prototype.Schema = 	
	"<element name='MaximumLevel'><data type='nonNegativeInteger'/></element>" +
	"<element name='Range'><data type='nonNegativeInteger'/></element>";

AlertRaiser.prototype.Init = function() 
{
	this.level = 0;

	// Remember the units ordered to garrison
	this.garrisonedUnits = [];
	this.walkingUnits = [];

	// Remember production buildings under alert
	this.prodBuildings = [];
};

AlertRaiser.prototype.GetLevel = function()
{
	return this.level;
};

AlertRaiser.prototype.HasRaisedAlert = function()
{
	return this.level > 0;
};

AlertRaiser.prototype.CanIncreaseLevel = function()
{
	return this.template.MaximumLevel > this.level;
};

AlertRaiser.prototype.SoundAlert = function()
{
	var alertString = "alert" + this.level;
	PlaySound(alertString, this.entity);
};

AlertRaiser.prototype.UpdateUnits = function(units)
{
	var level = this.GetLevel();	
	for each (var unit in units)
	{
		var cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
		if (!cmpUnitAI || !cmpUnitAI.ReactsToAlert(level))
			continue;
		cmpUnitAI.ReplaceOrder("Alert", {"raiser": this.entity, "force": true});
		this.walkingUnits.push(unit);
	}
};

AlertRaiser.prototype.IncreaseAlertLevel = function()
{
	if (!this.CanIncreaseLevel())
		return false;

	this.level++;
	this.SoundAlert();

	// Find buildings/units owned by this unit's player
	var players = [];
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership)
		players = [cmpOwnership.GetOwner()];

	// Select production buildings to put "under alert", including the raiser itself if possible
	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var level = this.GetLevel();
	var buildings = rangeMan.ExecuteQuery(this.entity, 0, this.template.Range, players, IID_ProductionQueue);
	var ownCmpProductionQueue = Engine.QueryInterface(this.entity, IID_ProductionQueue);
	if (ownCmpProductionQueue)
		buildings.push(this.entity);

	for each (var building in buildings)
	{
		var cmpProductionQueue = Engine.QueryInterface(building, IID_ProductionQueue);
		cmpProductionQueue.PutUnderAlert(this.entity);
		this.prodBuildings.push(building);
	}

	// Select units to put under alert, according to their reaction to this level
	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var level = this.GetLevel();
	var units = rangeMan.ExecuteQuery(this.entity, 0, this.template.Range, players, IID_UnitAI).filter( function(e){
		var cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
		return (!cmpUnitAI.IsUnderAlert() && cmpUnitAI.ReactsToAlert(level));
		});

	for each (var unit in units)
	{
		var cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
		cmpUnitAI.ReplaceOrder("Alert", {"raiser": this.entity, "force": true});
		this.walkingUnits.push(unit);
	}

	return true;
};

AlertRaiser.prototype.OnUnitGarrisonedAfterAlert = function(msg)
{
	this.garrisonedUnits.push({"holder": msg.holder, "unit": msg.unit});

	var index = this.walkingUnits.indexOf(msg.unit);
	if (index != -1)
		this.walkingUnits.splice(index, 1);
};

AlertRaiser.prototype.EndOfAlert = function()
{
	this.level = 0;
	this.SoundAlert();

	// First, handle units not yet garrisoned
	for each (var unit in this.walkingUnits)
	{
		var cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
		if (!cmpUnitAI)
			continue;

		cmpUnitAI.ResetAlert();
	
		if (cmpUnitAI.HasWorkOrders())
			cmpUnitAI.BackToWork();
		else
			cmpUnitAI.ReplaceOrder("Stop", undefined);
	}
	this.walkingUnits = [];

	// Then, eject garrisoned units
	for each (var slot in this.garrisonedUnits)
	{
		var cmpGarrisonHolder = Engine.QueryInterface(slot.holder, IID_GarrisonHolder);
		var cmpUnitAI = Engine.QueryInterface(slot.unit, IID_UnitAI);
		if (!cmpUnitAI)
			continue;

		// If the garrison building was destroyed, the unit is already ejected
		if (!cmpGarrisonHolder || cmpGarrisonHolder.PerformEject([slot.unit], true))
		{
			cmpUnitAI.ResetAlert();				
			if (cmpUnitAI.HasWorkOrders())
				cmpUnitAI.BackToWork();
		}
	}
	this.garrisonedUnits = [];

	// Finally, reset production buildings state
	for each (var building in this.prodBuildings)
	{
		var cmpProductionQueue = Engine.QueryInterface(building, IID_ProductionQueue);
		if (cmpProductionQueue)
			cmpProductionQueue.ResetAlert();
	}
	this.prodBuildings = [];

	return true;
};

Engine.RegisterComponentType(IID_AlertRaiser, "AlertRaiser", AlertRaiser);
