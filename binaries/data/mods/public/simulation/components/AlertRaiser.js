function AlertRaiser() {}

AlertRaiser.prototype.Schema =
	"<element name='MaximumLevel'><data type='nonNegativeInteger'/></element>" +
	"<element name='Range'><data type='nonNegativeInteger'/></element>";

AlertRaiser.prototype.Init = function()
{
	this.level = 0;
	this.garrisonedUnits = [];
	this.prodBuildings = [];
	this.walkingUnits = [];
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
	PlaySound("alert" + this.level, this.entity);
};

/**
 * Used when units are spawned and need to follow alert orders.
 *  @param {number[]} units - Entity IDs of spawned units.
 */
AlertRaiser.prototype.UpdateUnits = function(units)
{
	for (let unit of units)
	{
		let cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
		if (!cmpUnitAI || !cmpUnitAI.ReactsToAlert(this.level))
			continue;

		cmpUnitAI.ReplaceOrder("Alert", { "raiser": this.entity, "force": true });
		this.walkingUnits.push(unit);
	}
};

AlertRaiser.prototype.IncreaseAlertLevel = function()
{
	if (!this.CanIncreaseLevel())
		return false;

	++this.level;
	this.SoundAlert();

	// Find buildings/units owned by this unit's player
	let players = [];
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership)
		players = [cmpOwnership.GetOwner()];

	// Select production buildings to put "under alert", including the raiser itself if possible
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let buildings = cmpRangeManager.ExecuteQuery(this.entity, 0, this.template.Range, players, IID_ProductionQueue);
	if (Engine.QueryInterface(this.entity, IID_ProductionQueue))
		buildings.push(this.entity);

	for (let building of buildings)
	{
		let cmpProductionQueue = Engine.QueryInterface(building, IID_ProductionQueue);
		cmpProductionQueue.PutUnderAlert(this.entity);
		this.prodBuildings.push(building);
	}

	// Select units to put under alert, according to their reaction to this level
	let units = cmpRangeManager.ExecuteQuery(this.entity, 0, this.template.Range, players, IID_UnitAI).filter(ent => {
		let cmpUnitAI = Engine.QueryInterface(ent, IID_UnitAI);
		return !cmpUnitAI.IsUnderAlert() && cmpUnitAI.ReactsToAlert(this.level);
	});

	for (let unit of units)
	{
		let cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
		cmpUnitAI.ReplaceOrder("Alert", { "raiser": this.entity, "force": true });
		this.walkingUnits.push(unit);
	}

	return true;
};

AlertRaiser.prototype.OnUnitGarrisonedAfterAlert = function(msg)
{
	this.garrisonedUnits.push({ "holder": msg.holder, "unit": msg.unit });

	let index = this.walkingUnits.indexOf(msg.unit);
	if (index != -1)
		this.walkingUnits.splice(index, 1);
};

AlertRaiser.prototype.EndOfAlert = function()
{
	this.level = 0;
	this.SoundAlert();

	// First, handle units not yet garrisoned
	for (let unit of this.walkingUnits)
	{
		let cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
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
	for (let slot of this.garrisonedUnits)
	{
		let cmpGarrisonHolder = Engine.QueryInterface(slot.holder, IID_GarrisonHolder);
		let cmpUnitAI = Engine.QueryInterface(slot.unit, IID_UnitAI);
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
	for (let building of this.prodBuildings)
	{
		let cmpProductionQueue = Engine.QueryInterface(building, IID_ProductionQueue);
		if (cmpProductionQueue)
			cmpProductionQueue.ResetAlert();
	}
	this.prodBuildings = [];

	return true;
};

Engine.RegisterComponentType(IID_AlertRaiser, "AlertRaiser", AlertRaiser);
