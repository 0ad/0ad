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
		"<element name='DiminishingReturns' a:help='The rate at which adding more gatherers decreases overall efficiency. Lower numbers = faster dropoff. Leave the element out for no diminishing returns.'>" +
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
	let resData = type === "treasure" ?
		{ "subtypes": Resources.GetNames() } :
		Resources.GetResource(type);

	if (!resData || !resData.subtypes[subtype])
	{
		error("ResourceSupply with invalid resource: " + uneval(resData));
		Engine.DestroyEntity(this.entity);
	}

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

/** Note to people looking to change <DiminishingReturns> in a template: This is a bit complicated. Basically, the lower that number is
 * the steeper diminishing returns will be. I suggest playing around with Wolfram Alpha or a graphing calculator a bit.
 * In each of the following links, replace 0.65 with the gather rate of your worker for the resource with diminishing returns and
 * 14 with the constant you wish to use to control the diminishing returns.
 * (In this case 0.65 is the women farming rate, in resources/second, and 14 is a good constant for farming.)
 * This is the gather rate in resources/second of each individual worker as the total number of workers goes up:
 * http://www.wolframalpha.com/input/?i=plot+%281%2F2+cos%28%28x-1%29*pi%2F14%29+%2B+1%2F2%29+*+0.65+from+1+to+5
 * This is the total output of the resource in resources/second:
 * http://www.wolframalpha.com/input/?i=plot+x%281%2F2+cos%28%28x-1%29*pi%2F14%29+%2B+1%2F2%29+*+0.65+from+1+to+5
 * This is the fraction of a worker each new worker is worth (the 5th worker in this example is only producing about half as much as the first one):
 * http://www.wolframalpha.com/input/?i=plot+x%281%2F2+cos%28%28x-1%29*pi%2F14%29+%2B+1%2F2%29+-++%28x-1%29%281%2F2+cos%28%28x-2%29*pi%2F14%29+%2B+1%2F2%29+from+x%3D1+to+5+and+y%3D0+to+1
 * Here's how this technically works:
 * The cosine function is an oscillating curve, normally between -1 and 1. Multiplying by 0.5 squishes that down to
 * between -0.5 and 0.5. Adding 0.5 to that changes the range to 0 to 1. The diminishingReturns constant
 * adjusts the period of the curve.
 */
ResourceSupply.prototype.GetDiminishingReturns = function()
{
	if ("DiminishingReturns" in this.template)
	{
		let diminishingReturns = ApplyValueModificationsToEntity("ResourceSupply/DiminishingReturns", +this.template.DiminishingReturns, this.entity);
		if (diminishingReturns)
			return (0.5 * Math.cos((this.GetNumGatherers() - 1) * Math.PI / diminishingReturns) + 0.5);
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
