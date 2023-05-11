function ResourceTrickle() {}

ResourceTrickle.prototype.Schema =
	"<a:help>Controls the resource trickle ability of the unit.</a:help>" +
	"<element name='Rates' a:help='Trickle Rates'>" +
		Resources.BuildSchema("nonNegativeDecimal") +
	"</element>" +
	"<element name='Interval' a:help='Number of milliseconds must pass for the player to gain the next trickle.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

ResourceTrickle.prototype.Init = function()
{
	this.trickleInterval = +this.template.Interval;
	this.CheckTimer();
};

ResourceTrickle.prototype.GetInterval = function()
{
	return this.trickleInterval;
};

ResourceTrickle.prototype.GetRates = function()
{
	return this.rates;
};

/**
 * @return {boolean} - Whether this entity has at least one non-zero trickle rate.
 */
ResourceTrickle.prototype.ComputeRates = function()
{
	this.rates = {};
	let hasTrickle = false;
	for (let resource in this.template.Rates)
	{
		let rate = ApplyValueModificationsToEntity("ResourceTrickle/Rates/" + resource, +this.template.Rates[resource], this.entity);
		if (rate)
		{
			this.rates[resource] = rate;
			hasTrickle = true;
		}
	}

	return hasTrickle;
};

ResourceTrickle.prototype.Trickle = function(data, lateness)
{
	// The player entity may also have a ResourceTrickle component
	let cmpPlayer = QueryOwnerInterface(this.entity) || Engine.QueryInterface(this.entity, IID_Player);
	if (!cmpPlayer)
		return;

	cmpPlayer.AddResources(this.rates);
};

ResourceTrickle.prototype.OnValueModification = function(msg)
{
	if (msg.component != "ResourceTrickle")
		return;

	this.CheckTimer();
};

ResourceTrickle.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to != INVALID_PLAYER)
		this.CheckTimer();
};

ResourceTrickle.prototype.CheckTimer = function()
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

	let oldTrickleInterval = this.trickleInterval;
	this.trickleInterval = ApplyValueModificationsToEntity("ResourceTrickle/Interval", +this.template.Interval, this.entity);
	if (this.trickleInterval < 0)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		delete this.timer;
		return;
	}

	if (this.timer)
	{
		if (this.trickleInterval == oldTrickleInterval)
			return;

		// If the timer wasn't invalidated before (interval <= 0), just update it.
		if (oldTrickleInterval > 0)
		{
			let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			cmpTimer.UpdateRepeatTime(this.timer, this.trickleInterval);
			return;
		}
	}

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_ResourceTrickle, "Trickle", this.trickleInterval, this.trickleInterval, undefined);
};

Engine.RegisterComponentType(IID_ResourceTrickle, "ResourceTrickle", ResourceTrickle);
