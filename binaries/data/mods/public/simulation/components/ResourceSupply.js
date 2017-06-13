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
	"<element name='Type' a:help='Type and Subtype of resource available from this entity'>" +
		Resources.BuildChoicesSchema(true, true) +
	"</element>" +
	"<element name='MaxGatherers' a:help='Amount of gatherers who can gather resources from this entity at the same time'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<optional>" +
		"<element name='DiminishingReturns' a:help='The relative rate of any new gatherer compared to the previous one (geometric sequence). Leave the element out for no diminishing returns.'>" +
			"<ref name='positiveDecimal'/>" +
		"</element>" +
	"</optional>";

ResourceSupply.prototype.Init = function()
{
	// Current resource amount (non-negative)
	this.amount = this.GetMaxAmount();

	this.gatherers = [];	// list of IDs for each players
	let cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);	// system component so that's safe.
	let numPlayers = cmpPlayerManager.GetNumPlayers();
	for (let i = 0; i <= numPlayers; ++i)	// use "<=" because we want Gaia too.
		this.gatherers.push([]);

	this.infinite = !isFinite(+this.template.Amount);

	let [type, subtype] = this.template.Type.split('.');
	this.cachedType = { "generic": type, "specific": subtype };
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

ResourceSupply.prototype.GetNumGatherers = function()
{
	return this.gatherers.reduce((a, b) => a + b.length, 0);
};

/* The rate of each additionnal gatherer rate follow a geometric sequence, with diminishingReturns as common ratio. */
ResourceSupply.prototype.GetDiminishingReturns = function()
{
	if ("DiminishingReturns" in this.template)
	{
		let diminishingReturns = ApplyValueModificationsToEntity("ResourceSupply/DiminishingReturns", +this.template.DiminishingReturns, this.entity);
		if (diminishingReturns)
		{
			let numGatherers = this.GetNumGatherers();
			if (numGatherers > 1)
				return diminishingReturns == 1 ? 1 : (1. - Math.pow(diminishingReturns, numGatherers)) / (1. - diminishingReturns) / numGatherers;
		}
	}
	return null;
};

ResourceSupply.prototype.TakeResources = function(rate)
{
	// Before changing the amount, activate Fogging if necessary to hide changes
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	if (this.infinite)
		return { "amount": rate, "exhausted": false };

	// 'rate' should be a non-negative integer

	var old = this.amount;
	this.amount = Math.max(0, old - rate);
	var change = old - this.amount;

	// Remove entities that have been exhausted
	if (this.amount === 0)
		Engine.DestroyEntity(this.entity);

	Engine.PostMessage(this.entity, MT_ResourceSupplyChanged, { "from": old, "to": this.amount });

	return { "amount": change, "exhausted": (this.amount === 0) };
};

ResourceSupply.prototype.GetType = function()
{
	// All resources must have both type and subtype
	return this.cachedType;
};

ResourceSupply.prototype.IsAvailable = function(player, gathererID)
{
	if (this.GetNumGatherers() < this.GetMaxGatherers() || this.gatherers[player].indexOf(gathererID) !== -1)
		return true;
	return false;
};

ResourceSupply.prototype.AddGatherer = function(player, gathererID)
{
	if (!this.IsAvailable(player, gathererID))
		return false;

	if (this.gatherers[player].indexOf(gathererID) === -1)
	{
		this.gatherers[player].push(gathererID);
		// broadcast message, mainly useful for the AIs.
		Engine.PostMessage(this.entity, MT_ResourceSupplyNumGatherersChanged, { "to": this.GetNumGatherers() });
	}

	return true;
};

// should this return false if the gatherer didn't gather from said resource?
ResourceSupply.prototype.RemoveGatherer = function(gathererID, player)
{
	// this can happen if the unit is dead
	if (player === undefined || player === -1)
	{
		for (var i = 0; i < this.gatherers.length; ++i)
			this.RemoveGatherer(gathererID, i);
	}
	else
	{
		var index = this.gatherers[player].indexOf(gathererID);
		if (index !== -1)
		{
			this.gatherers[player].splice(index,1);
			// broadcast message, mainly useful for the AIs.
			Engine.PostMessage(this.entity, MT_ResourceSupplyNumGatherersChanged, { "to": this.GetNumGatherers() });
			return;
		}
	}
};

Engine.RegisterComponentType(IID_ResourceSupply, "ResourceSupply", ResourceSupply);
