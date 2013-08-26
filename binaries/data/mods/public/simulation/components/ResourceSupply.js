function ResourceSupply() {}

ResourceSupply.prototype.Schema =
	"<a:help>Provides a supply of one particular type of resource.</a:help>" +
	"<a:example>" +
		"<Amount>1000</Amount>" +
		"<Type>food.meat</Type>" +
	"</a:example>" +
	"<element name='KillBeforeGather' a:help='Whether this entity must be killed (health reduced to 0) before its resources can be gathered'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='Amount' a:help='Amount of resources available from this entity'>" +
		"<choice><data type='nonNegativeInteger'/><value>Infinity</value></choice>" +
	"</element>" +
	"<element name='Type' a:help='Type of resources'>" +
		"<choice>" +
			"<value>wood.tree</value>" +
			"<value>wood.ruins</value>" +
			"<value>stone.rock</value>" +
			"<value>stone.ruins</value>" +
			"<value>metal.ore</value>" +
			"<value>food.fish</value>" +
			"<value>food.fruit</value>" +
			"<value>food.grain</value>" +
			"<value>food.meat</value>" +
			"<value>food.milk</value>" +
			"<value>treasure.wood</value>" +
			"<value>treasure.stone</value>" +
			"<value>treasure.metal</value>" +
			"<value>treasure.food</value>" +
		"</choice>" +
	"</element>" +
	"<optional>" +
		"<element name='Regeneration' a:help='Controls whether this resource can regenerate its remaining amount.'>" +
			"<interleave>" +
				"<element name='Rate' a:help='Optional regeneration rate. Resources/second if the growth is linear or the rate of quadratic growth if the growth is quadratic.'>" +
					"<ref name='nonNegativeDecimal'/>" +
				"</element>" +
				"<optional>" +
					"<element name='Acceleration' a:help='Controls the curve the regeneration rate will follow for quadratic growth; does nothing for linear growth.'>" +
						"<ref name='nonNegativeDecimal'/>" +
					"</element>" +
				"</optional>" +
				"<element name='Delay' a:help='Seconds between when the number of gatherers hit 0 and the resource starts regenerating.'>" +
					"<data type='nonNegativeInteger'/>" +
				"</element>" +
				"<element name='Growth' a:help='Growth formula for regeneration. Either linear or quadratic. Linear continues at a steady rate, while quadratic gets faster over time.'>" +
					"<choice>" +
						"<value>linear</value>" +
						"<value>quadratic</value>" +
					"</choice>" +
				"</element>" +
			"</interleave>" +
		"</element>" +
	"</optional>" +
	"<element name='MaxGatherers' a:help='Amount of gatherers who can gather resources from this entity at the same time'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<optional>" +
		"<element name='DiminishingReturns' a:help='The rate at which adding more gatherers decreases overall efficiency. Lower numbers = faster dropoff. Leave the element out for no diminishing returns.'>" +
			"<ref name='positiveDecimal'/>" +
		"</element>" +
	"</optional>";

ResourceSupply.prototype.Init = function()
{
	// Current resource amount (non-negative)
	this.amount = this.GetMaxAmount();
    this.gatherers = [];	// list of IDs
    this.infinite = !isFinite(+this.template.Amount);
	if (this.template.Regeneration) {
		this.regenRate = +this.template.Regeneration.Rate;
		if (this.template.Regeneration.Acceleration)
			this.regenAccel = +this.template.Regeneration.Acceleration;
		this.regenDelay = +this.template.Regeneration.Delay;
	}
	if (this.IsRegenerative())
		this.RegenerateResources();
};

ResourceSupply.prototype.IsInfinite = function()
{
	return this.infinite;
};

ResourceSupply.prototype.GetKillBeforeGather = function()
{
	return (this.template.KillBeforeGather == "true");
};

ResourceSupply.prototype.GetMaxAmount = function()
{
	return +this.template.Amount;
};

ResourceSupply.prototype.GetCurrentAmount = function()
{
	return this.amount;
};

ResourceSupply.prototype.GetMaxGatherers = function()
{
	return +this.template.MaxGatherers;
};

ResourceSupply.prototype.GetGatherers = function()
{
	return this.gatherers;
};

ResourceSupply.prototype.GetDiminishingReturns = function()
{
	if ("DiminishingReturns" in this.template)
		return ApplyTechModificationsToEntity("ResourceSupply/DiminishingReturns", +this.template.DiminishingReturns, this.entity);
	return null;
};

ResourceSupply.prototype.TakeResources = function(rate)
{
	if (this.infinite)
		return { "amount": rate, "exhausted": false };

	// 'rate' should be a non-negative integer

	var old = this.amount;
	this.amount = Math.max(0, old - rate);
	var change = old - this.amount;

	// Remove entities that have been exhausted
	if (this.amount == 0 && !this.IsRegenerative())
		Engine.DestroyEntity(this.entity);

	Engine.PostMessage(this.entity, MT_ResourceSupplyChanged, { "from": old, "to": this.amount });

	return { "amount": change, "exhausted": this.amount == 0 };
};

ResourceSupply.prototype.RegenerateResources = function(data, lateness)
{
	var max = this.GetMaxAmount();
	if (this.gatherers.length == 0 && !this.regenDelayTimer && this.amount < max)
	{
		var old = this.amount;
		if (this.regenGrowth == "linear")
			this.amount = Math.min(max, this.amount + data.rate);
		else
			this.amount = Math.min(max, this.amount + Math.max(1, data.rate * max * (data.acceleration * this.amount / max -Math.pow(this.amount / max, 2)) / 100));
		Engine.PostMessage(this.entity, MT_ResourceSupplyChanged, { "from": old, "to": this.amount });
	}
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	var regenRate = this.GetRegenerationRate();
	var absRegen = Math.abs(regenRate);
	if (Math.floor(regenRate) == regenRate || this.regenGrowth == "quadratic")
		cmpTimer.SetTimeout(this.entity, IID_ResourceSupply, "RegenerateResources", 1000, { "rate": regenRate, "acceleration": this.GetRegenerationAcceleration() });
	else
		cmpTimer.SetTimeout(this.entity, IID_ResourceSupply, "RegenerateResources", 1000 / absRegen,
				{ "rate": absRegen == regenRate ? 1 : -1 });
};

ResourceSupply.prototype.StartRegenerationDelayTimer = function()
{
	if (!this.regenDelayTimer) {
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.regenDelayTimer = cmpTimer.SetTimeout(this.entity, IID_ResourceSupply, "CancelRegenerationDelayTimer", this.GetRegenerationDelay() * 1000, null);
	}
};

ResourceSupply.prototype.CancelRegenerationDelayTimer = function()
{
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.regenDelayTimer);
	this.regenDelayTimer = null;
};

ResourceSupply.prototype.GetType = function()
{
	// All resources must have both type and subtype

	var [type, subtype] = this.template.Type.split('.');
	return { "generic": type, "specific": subtype };
};

ResourceSupply.prototype.IsAvailable = function(gathererID)
{
	if (this.gatherers.length < this.GetMaxGatherers() || this.gatherers.indexOf(gathererID) !== -1)
		return true;
	return false;
};

ResourceSupply.prototype.IsRegenerative = function()
{
	return this.GetRegenerationRate() != 0;
};

ResourceSupply.prototype.GetTerritoryOwner = function ()
{
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpTerritoryManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TerritoryManager);
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!(cmpPosition && cmpPosition.IsInWorld()))
		return 0;  // Something's wrong, just say we're in neutral territory.
	var pos = cmpPosition.GetPosition2D();
	return cmpPlayerManager.GetPlayerByID(cmpTerritoryManager.GetOwner(pos.x, pos.y));
};

ResourceSupply.prototype.GetRegenerationRate = function()
{
	return ApplyTechModificationsToPlayer("ResourceSupply/Regeneration/Rate", this.regenRate, this.GetTerritoryOwner());
};

ResourceSupply.prototype.GetRegenerationAcceleration = function()
{
	return ApplyTechModificationsToPlayer("ResourceSupply/Regeneration/Acceleration", this.regenAccel, this.GetTerritoryOwner());
};

ResourceSupply.prototype.GetRegenerationDelay = function()
{
	return ApplyTechModificationsToPlayer("ResourcesSupply/Regeneration/Delay", this.regenDelay, this.GetTerritoryOwner());
};

ResourceSupply.prototype.AddGatherer = function(gathererID)
{
	if (!this.IsAvailable(gathererID))
		return false;

	if (this.gatherers.indexOf(gathererID) === -1)
	{
		this.gatherers.push(gathererID);
		this.CancelRegenerationDelayTimer();
		// broadcast message, mainly useful for the AIs.
		Engine.PostMessage(this.entity, MT_ResourceSupplyGatherersChanged, { "to": this.gatherers });
	}

	return true;
};

// should this return false if the gatherer didn't gather from said resource?
ResourceSupply.prototype.RemoveGatherer = function(gathererID)
{
	if (this.gatherers.indexOf(gathererID) !== -1)
	{
		this.gatherers.splice(this.gatherers.indexOf(gathererID),1);
		// broadcast message, mainly useful for the AIs.
		Engine.PostMessage(this.entity, MT_ResourceSupplyGatherersChanged, { "to": this.gatherers });
	}
	if (this.gatherers.length == 0)
		this.StartRegenerationDelayTimer();
};

Engine.RegisterComponentType(IID_ResourceSupply, "ResourceSupply", ResourceSupply);
