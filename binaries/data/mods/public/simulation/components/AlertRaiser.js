function AlertRaiser() {}

AlertRaiser.prototype.Schema =
	"<element name='MaximumLevel'><data type='nonNegativeInteger'/></element>" +
	"<element name='Range'><data type='nonNegativeInteger'/></element>";

AlertRaiser.prototype.Init = function()
{
	this.level = 0;

	// Range at which units will search for garrison holders
	this.searchRange = 100;

	// Extra allowance for units to garrison in buildings outside the alert range
	this.bufferRange = 50;
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

AlertRaiser.prototype.IncreaseAlertLevel = function()
{
	if (!this.CanIncreaseLevel())
		return false;

	++this.level;
	this.SoundAlert();

	// Find buildings/units owned by this unit's player
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return false;

	let players = [cmpOwnership.GetOwner()];
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	// Select units to put under alert, according to their reaction to this level
	let units = cmpRangeManager.ExecuteQuery(this.entity, 0, this.template.Range, players, IID_UnitAI).filter(ent =>
		Engine.QueryInterface(ent, IID_UnitAI).ReactsToAlert(this.level)
	);

	// Store the number of available garrison spots, so that units don't try to garrison in buildings that will be full
	let reserved = new Map();
	for (let unit of units)
	{
		let holder = cmpRangeManager.ExecuteQuery(unit, 0, this.searchRange, players, IID_GarrisonHolder).find(ent => {
			// Ignore moving garrison holders
			if (Engine.QueryInterface(ent, IID_UnitAI))
				return false;

			// Ensure that the garrison holder is within range of the alert raiser
			if (DistanceBetweenEntities(this.entity, ent) > +this.template.Range + this.bufferRange)
				return false;

			let cmpGarrisonHolder = Engine.QueryInterface(ent, IID_GarrisonHolder);
			if (!reserved.has(ent))
				reserved.set(ent, cmpGarrisonHolder.GetCapacity() - cmpGarrisonHolder.GetGarrisonedEntitiesCount());

			return cmpGarrisonHolder.IsAllowedToGarrison(unit) && reserved.get(ent);
		});

		let cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
		if (holder)
		{
			reserved.set(holder, reserved.get(holder) - 1);
			cmpUnitAI.ReplaceOrder("Garrison", { "target": holder, "force": true });
		}
		else
			// If no available spots, move to the alert raiser
			cmpUnitAI.ReplaceOrder("WalkToTarget", { "target": this.entity, "force": true });
	}
	return true;
};

AlertRaiser.prototype.EndOfAlert = function()
{
	this.SoundAlert();

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return false;

	let players = [cmpOwnership.GetOwner()];

	// Units that are not garrisoned should stop and go back to work
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	let units = cmpRangeManager.ExecuteQuery(this.entity, 0, +this.template.Range + this.bufferRange, players, IID_UnitAI).filter(ent =>
		Engine.QueryInterface(ent, IID_UnitAI).ReactsToAlert(this.level)
	);

	for (let unit of units)
	{
		let cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
		if (cmpUnitAI.HasWorkOrders() && (cmpUnitAI.HasGarrisonOrder() || cmpUnitAI.IsIdle()))
			cmpUnitAI.BackToWork();
		else if (cmpUnitAI.HasGarrisonOrder())
			// Stop rather than continue to try to garrison
			cmpUnitAI.ReplaceOrder("Stop", undefined);
	}

	// Units that are garrisoned should ungarrison and go back to work
	let holders = cmpRangeManager.ExecuteQuery(this.entity, 0, +this.template.Range + this.bufferRange, players, IID_GarrisonHolder);
	if (Engine.QueryInterface(this.entity, IID_GarrisonHolder))
		holders.push(this.entity);

	for (let holder of holders)
	{
		if (Engine.QueryInterface(holder, IID_UnitAI))
			continue;

		let cmpGarrisonHolder = Engine.QueryInterface(holder, IID_GarrisonHolder);
		let units = cmpGarrisonHolder.GetEntities().filter(ent =>
			Engine.QueryInterface(ent, IID_UnitAI).ReactsToAlert(this.level) &&
			Engine.QueryInterface(ent, IID_Ownership).GetOwner() == players[0]
		);

		for (let unit of units)
			if (cmpGarrisonHolder.PerformEject([unit], false))
			{
				let cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
				if (cmpUnitAI.HasWorkOrders())
					cmpUnitAI.BackToWork();
				else
					// Stop rather than walk to the rally point
					cmpUnitAI.ReplaceOrder("Stop", undefined);
			}
	}

	this.level = 0;
	return true;
};

Engine.RegisterComponentType(IID_AlertRaiser, "AlertRaiser", AlertRaiser);
