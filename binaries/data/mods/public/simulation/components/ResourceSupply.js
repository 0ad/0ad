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

	this.gatherers = [];	// list of IDs for each players
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);	// system component so that's safe.
	var numPlayers = cmpPlayerManager.GetNumPlayers();
	for (var i = 0; i <= numPlayers; ++i)	// use "<=" because we want Gaia too.
		this.gatherers.push([]);

	this.infinite = !isFinite(+this.template.Amount);
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
	var numPlayers = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager).GetNumPlayers();
	var total = [];
	for (var playerid = 0; playerid <= numPlayers; playerid++)
		for (var gatherer = 0; gatherer < this.gatherers[playerid].length; gatherer++)
			total.push(this.gatherers[playerid][gatherer]);
	return total;
};

ResourceSupply.prototype.GetDiminishingReturns = function()
{
	if ("DiminishingReturns" in this.template)
		return ApplyValueModificationsToEntity("ResourceSupply/DiminishingReturns", +this.template.DiminishingReturns, this.entity);
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
	if (this.amount == 0)
		Engine.DestroyEntity(this.entity);

	Engine.PostMessage(this.entity, MT_ResourceSupplyChanged, { "from": old, "to": this.amount });

	return { "amount": change, "exhausted": (this.amount == 0) };
};

ResourceSupply.prototype.GetType = function()
{
	// All resources must have both type and subtype

	var [type, subtype] = this.template.Type.split('.');
	return { "generic": type, "specific": subtype };
};

ResourceSupply.prototype.IsAvailable = function(player, gathererID)
{
	if (this.GetGatherers().length < this.GetMaxGatherers() || this.gatherers[player].indexOf(gathererID) !== -1)
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
	    Engine.PostMessage(this.entity, MT_ResourceSupplyGatherersChanged, { "to": this.GetGatherers() });
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
			Engine.PostMessage(this.entity, MT_ResourceSupplyGatherersChanged, { "to": this.GetGatherers() });
			return;
		}
	}
};

Engine.RegisterComponentType(IID_ResourceSupply, "ResourceSupply", ResourceSupply);
