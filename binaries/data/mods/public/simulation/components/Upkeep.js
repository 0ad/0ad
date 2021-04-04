function Upkeep() {}

Upkeep.prototype.Schema =
	"<a:help>Controls the resource upkeep of an entity.</a:help>" +
	"<element name='Rates' a:help='Upkeep Rates'>" +
		Resources.BuildSchema("nonNegativeDecimal") +
	"</element>" +
	"<element name='Interval' a:help='Number of milliseconds must pass for the player to pay the next upkeep.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

Upkeep.prototype.Init = function()
{
	this.upkeepInterval = +this.template.Interval;
	this.CheckTimer();
};

/**
 * @return {number} - The interval between resource subtractions, in ms.
 */
Upkeep.prototype.GetInterval = function()
{
	return this.upkeepInterval;
};

/**
 * @return {Object} - The upkeep rates in the form of { "resourceName": {number} }.
 */
Upkeep.prototype.GetRates = function()
{
	return this.rates;
};

/**
 * @return {boolean} - Whether this entity has at least one non-zero amount of resources to pay.
 */
Upkeep.prototype.ComputeRates = function()
{
	this.rates = {};
	let hasUpkeep = false;
	for (let resource in this.template.Rates)
	{
		let rate = ApplyValueModificationsToEntity("Upkeep/Rates/" + resource, +this.template.Rates[resource], this.entity);
		if (rate)
		{
			this.rates[resource] = rate;
			hasUpkeep = true;
		}
	}

	return hasUpkeep;
};

/**
 * Try to subtract the needed resources.
 * Data and lateness are unused.
 */
Upkeep.prototype.Pay = function(data, lateness)
{
	let cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return;

	if (!cmpPlayer.TrySubtractResources(this.rates))
		this.HandleInsufficientUpkeep();
	else
		this.HandleSufficientUpkeep();
};

/**
 * E.g. take a hitpoint, reduce CP.
 */
Upkeep.prototype.HandleInsufficientUpkeep = function()
{
	if (this.unpayed)
		return;

	let cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
		cmpIdentity.SetControllable(false);
	this.unpayed = true;
};

/**
 * Reset to the previous stage.
 */
Upkeep.prototype.HandleSufficientUpkeep = function()
{
	if (!this.unpayed)
		return;

	let cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
		cmpIdentity.SetControllable(true);
	delete this.unpayed;
};

Upkeep.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Upkeep")
		return;

	this.CheckTimer();
};

/**
 * Recalculate the interval and update the timer accordingly.
 */
Upkeep.prototype.CheckTimer = function()
{
	if (!this.ComputeRates())
	{
		if (!this.timer)
			return;

		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		delete this.timer;
		return;
	}

	let oldUpkeepInterval = this.upkeepInterval;
	this.upkeepInterval = ApplyValueModificationsToEntity("Upkeep/Interval", +this.template.Interval, this.entity);
	if (this.upkeepInterval < 0)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		delete this.timer;
		return;
	}

	if (this.timer)
	{
		if (this.upkeepInterval == oldUpkeepInterval)
			return;

		// If the timer wasn't invalidated before (interval <= 0), just update it.
		if (oldUpkeepInterval > 0)
		{
			let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			cmpTimer.UpdateRepeatTime(this.timer, this.upkeepInterval);
			return;
		}
	}

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_Upkeep, "Pay", this.upkeepInterval, this.upkeepInterval, undefined);
};

Engine.RegisterComponentType(IID_Upkeep, "Upkeep", Upkeep);
