function AlertRaiser() {}

AlertRaiser.prototype.Schema =
	"<element name='List' a:help='Classes of entities which are affected by this alert raiser'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>" +
	"<element name='RaiseAlertRange'><data type='integer'/></element>" +
	"<element name='EndOfAlertRange'><data type='integer'/></element>" +
	"<element name='SearchRange'><data type='integer'/></element>";

AlertRaiser.prototype.Init = function()
{
	// Store the last time the alert was used so players can't lag the game by raising alerts repeatedly.
	this.lastTime = 0;
};

AlertRaiser.prototype.UnitFilter = function(unit)
{
	let cmpIdentity = Engine.QueryInterface(unit, IID_Identity);
	return cmpIdentity && MatchesClassList(cmpIdentity.GetClassesList(), this.template.List._string);
};

AlertRaiser.prototype.RaiseAlert = function()
{
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	if (cmpTimer.GetTime() == this.lastTime)
		return;

	this.lastTime = cmpTimer.GetTime();
	PlaySound("alert_raise", this.entity);

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return;

	let owner = cmpOwnership.GetOwner();
	let cmpPlayer = QueryOwnerInterface(this.entity);
	let mutualAllies = cmpPlayer ? cmpPlayer.GetMutualAllies() : [owner];
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	// Store the number of available garrison spots so that units don't try to garrison in buildings that will be full
	let reserved = new Map();

	let units = cmpRangeManager.ExecuteQuery(this.entity, 0, +this.template.RaiseAlertRange, [owner], IID_UnitAI).filter(ent => this.UnitFilter(ent));
	for (let unit of units)
	{
		let cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);

		let holder = cmpRangeManager.ExecuteQuery(unit, 0, +this.template.SearchRange, mutualAllies, IID_GarrisonHolder).find(ent => {
			// Ignore moving garrison holders
			if (Engine.QueryInterface(ent, IID_UnitAI))
				return false;

			// Ensure that the garrison holder is within range of the alert raiser
			if (+this.template.EndOfAlertRange > 0 && DistanceBetweenEntities(this.entity, ent) > +this.template.EndOfAlertRange)
				return false;

			if (!cmpUnitAI.CheckTargetVisible(ent))
				return false;

			let cmpGarrisonHolder = Engine.QueryInterface(ent, IID_GarrisonHolder);
			if (!reserved.has(ent))
				reserved.set(ent, cmpGarrisonHolder.GetCapacity() - cmpGarrisonHolder.GetGarrisonedEntitiesCount());

			return cmpGarrisonHolder.IsAllowedToGarrison(unit) && reserved.get(ent);
		});

		if (holder)
		{
			reserved.set(holder, reserved.get(holder) - 1);
			cmpUnitAI.ReplaceOrder("Garrison", { "target": holder, "force": true });
		}
		else
			// If no available spots, stop moving
			cmpUnitAI.ReplaceOrder("Stop", { "force": true });
	}
};

AlertRaiser.prototype.EndOfAlert = function()
{
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	if (cmpTimer.GetTime() == this.lastTime)
		return;

	this.lastTime = cmpTimer.GetTime();
	PlaySound("alert_end", this.entity);

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership || cmpOwnership.GetOwner() == INVALID_PLAYER)
		return;

	let owner = cmpOwnership.GetOwner();
	let cmpPlayer = QueryOwnerInterface(this.entity);
	let mutualAllies = cmpPlayer ? cmpPlayer.GetMutualAllies() : [owner];
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	// Units that are not garrisoned should go back to work
	let units = cmpRangeManager.ExecuteQuery(this.entity, 0, +this.template.EndOfAlertRange, [owner], IID_UnitAI).filter(ent => this.UnitFilter(ent));
	for (let unit of units)
	{
		let cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
		if (cmpUnitAI.HasWorkOrders() && cmpUnitAI.ShouldRespondToEndOfAlert())
			cmpUnitAI.BackToWork();
		else if (cmpUnitAI.ShouldRespondToEndOfAlert())
			// Stop rather than continue to try to garrison
			cmpUnitAI.ReplaceOrder("Stop", { "force": true });
	}

	// Units that are garrisoned should ungarrison and go back to work
	let holders = cmpRangeManager.ExecuteQuery(this.entity, 0, +this.template.EndOfAlertRange, mutualAllies, IID_GarrisonHolder);
	if (Engine.QueryInterface(this.entity, IID_GarrisonHolder))
		holders.push(this.entity);

	for (let holder of holders)
	{
		if (Engine.QueryInterface(holder, IID_UnitAI))
			continue;

		let cmpGarrisonHolder = Engine.QueryInterface(holder, IID_GarrisonHolder);
		let units = cmpGarrisonHolder.GetEntities().filter(ent => {
			let cmpOwner = Engine.QueryInterface(ent, IID_Ownership);
			return cmpOwner && cmpOwner.GetOwner() == owner && this.UnitFilter(ent);
		});

		for (let unit of units)
			if (cmpGarrisonHolder.PerformEject([unit], false))
			{
				let cmpUnitAI = Engine.QueryInterface(unit, IID_UnitAI);
				if (cmpUnitAI.HasWorkOrders())
					cmpUnitAI.BackToWork();
				else
					// Stop rather than walk to the rally point
					cmpUnitAI.ReplaceOrder("Stop", { "force": true });
			}
	}
};

Engine.RegisterComponentType(IID_AlertRaiser, "AlertRaiser", AlertRaiser);
