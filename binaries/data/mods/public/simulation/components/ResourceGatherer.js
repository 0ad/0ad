function ResourceGatherer() {}

ResourceGatherer.prototype.Schema =
	"<a:help>Lets the unit gather resources from entities that have the ResourceSupply component.</a:help>" +
	"<a:example>" +
		"<MaxDistance>2.0</MaxDistance>" +
		"<BaseSpeed>1.0</BaseSpeed>" +
		"<Rates>" +
			"<food.fish>1</food.fish>" +
			"<metal.ore>3</metal.ore>" +
			"<stone.rock>3</stone.rock>" +
			"<wood.tree>2</wood.tree>" +
		"</Rates>" +
		"<Capacities>" +
			"<food>10</food>" +
			"<metal>10</metal>" +
			"<stone>10</stone>" +
			"<wood>10</wood>" +
		"</Capacities>" +
	"</a:example>" +
	"<element name='MaxDistance' a:help='Max resource-gathering distance'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='BaseSpeed' a:help='Base resource-gathering rate (in resource units per second)'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='Rates' a:help='Per-resource-type gather rate multipliers. If a resource type is not specified then it cannot be gathered by this unit'>" +
		"<interleave>" +
			"<optional><element name='food' a:help='Food gather rate (may be overridden by \"food.*\" subtypes)'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='wood' a:help='Wood gather rate'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='stone' a:help='Stone gather rate'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='metal' a:help='Metal gather rate'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='treasure' a:help='Treasure gather rate (only presense on value makes sense, size is ignored)'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.fish' a:help='Fish gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.fruit' a:help='Fruit gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.grain' a:help='Grain gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.meat' a:help='Meat gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.milk' a:help='Milk gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='wood.tree' a:help='Tree gather rate (overrides \"wood\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='wood.ruins' a:help='Tree gather rate (overrides \"wood\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='stone.rock' a:help='Rock gather rate (overrides \"stone\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='stone.ruins' a:help='Rock gather rate (overrides \"stone\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='metal.ore' a:help='Ore gather rate (overrides \"metal\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='treasure.food' a:help='Food treasure gather rate (overrides \"treasure\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='treasure.wood' a:help='Wood treasure gather rate (overrides \"treasure\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='treasure.stone' a:help='Stone treasure gather rate (overrides \"treasure\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='treasure.metal' a:help='Metal treasure gather rate (overrides \"treasure\")'><ref name='positiveDecimal'/></element></optional>" +
		"</interleave>" +
	"</element>" +
	"<element name='Capacities' a:help='Per-resource-type maximum carrying capacity'>" +
		"<interleave>" +
			"<element name='food' a:help='Food capacity'><ref name='positiveDecimal'/></element>" +
			"<element name='wood' a:help='Wood capacity'><ref name='positiveDecimal'/></element>" +
			"<element name='stone' a:help='Stone capacity'><ref name='positiveDecimal'/></element>" +
			"<element name='metal' a:help='Metal capacity'><ref name='positiveDecimal'/></element>" +
		"</interleave>" +
	"</element>";

ResourceGatherer.prototype.Init = function()
{
	this.carrying = {}; // { generic type: integer amount currently carried }
	// (Note that this component supports carrying multiple types of resources,
	// each with an independent capacity, but the rest of the game currently
	// ensures and assumes we'll only be carrying one type at once)

	// The last exact type gathered, so we can render appropriate props
	this.lastCarriedType = undefined; // { generic, specific }
};

/**
 * Returns data about what resources the unit is currently carrying,
 * in the form [ {"type":"wood", "amount":7, "max":10} ]
 */
ResourceGatherer.prototype.GetCarryingStatus = function()
{
	var ret = [];
	for (var type in this.carrying)
	{
		ret.push({
			"type": type,
			"amount": this.carrying[type],
			"max": +this.template.Capacities[type]
		});
	}
	return ret;
};

/**
 * Used to instantly give resources to unit
 * @param resources The same structure as returned form GetCarryingStatus
 */
ResourceGatherer.prototype.GiveResources = function(resources)
{
	for each (var resource in resources)
	{
		this.carrying[resource.type] = +(resource.amount);
	}
};

/**
 * Returns the generic type of one particular resource this unit is
 * currently carrying, or undefined if none.
 */
ResourceGatherer.prototype.GetMainCarryingType = function()
{
	// Return the first key, if any
	for (var type in this.carrying)
		return type;

	return undefined;
};

/**
 * Returns the exact resource type we last picked up, as long as
 * we're still carrying something similar enough, in the form
 * { generic, specific }
 */
ResourceGatherer.prototype.GetLastCarriedType = function()
{
	if (this.lastCarriedType && this.lastCarriedType.generic in this.carrying)
		return this.lastCarriedType;
	else
		return undefined;
};

ResourceGatherer.prototype.GetGatherRates = function()
{
	var ret = {};
	for (var r in this.template.Rates)
		ret[r] = this.template.Rates[r] * this.template.BaseSpeed;
	return ret;
};

ResourceGatherer.prototype.GetRange = function()
{
	return { "max": +this.template.MaxDistance, "min": 0 };
	// maybe this should depend on the unit or target or something?
}

/**
 * Try to gather treasure
 * @return 'true' if treasure is successfully gathered and 'false' in the other case
 */
ResourceGatherer.prototype.TryInstantGather = function(target)
{
	var cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	var type = cmpResourceSupply.GetType();
	
	if (type.generic != "treasure") return false;
	
	var status = cmpResourceSupply.TakeResources(cmpResourceSupply.GetCurrentAmount());

	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	cmpPlayer.AddResource(type.specific, status.amount);
	
	var cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseTreasuresCollectedCounter();

	return true;
};

/**
 * Gather from the target entity. This should only be called after a successful range check,
 * and if the target has a compatible ResourceSupply.
 * It should be called at a rate of once per second.
 */
ResourceGatherer.prototype.PerformGather = function(target)
{
	var rate = this.GetTargetGatherRate(target);
	if (!rate)
		return { "exhausted": true };

	var cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	var type = cmpResourceSupply.GetType();

	// Initialise the carried count if necessary
	if (!this.carrying[type.generic])
		this.carrying[type.generic] = 0;

	// Find the maximum so we won't exceed our capacity
	var maxGathered = this.template.Capacities[type.generic] - this.carrying[type.generic];

	var status = cmpResourceSupply.TakeResources(Math.min(rate, maxGathered));

	this.carrying[type.generic] += status.amount;

	this.lastCarriedType = type;

	// Update stats of how much the player collected.
	// (We have to do it here rather than at the dropsite, because we
	// need to know what subtype it was)
	var cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseResourceGatheredCounter(type.generic, status.amount, type.specific);

	// Tell the target we're gathering from it
	Engine.PostMessage(target, MT_ResourceGather,
		{ "entity": target, "gatherer": this.entity });

	return {
		"amount": status.amount,
		"exhausted": status.exhausted,
		"filled": (this.carrying[type.generic] >= this.template.Capacities[type.generic])
	};
};

/**
 * Compute the amount of resources collected per second from the target.
 * Returns 0 if resources cannot be collected (e.g. the target doesn't
 * exist, or is the wrong type).
 */
ResourceGatherer.prototype.GetTargetGatherRate = function(target)
{
	var cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	if (!cmpResourceSupply)
		return 0;

	var type = cmpResourceSupply.GetType();

	var rate;
	if (type.specific && this.template.Rates[type.generic+"."+type.specific])
		rate = this.template.Rates[type.generic+"."+type.specific];
	else
		rate = this.template.Rates[type.generic];

	return (rate || 0) * this.template.BaseSpeed;
}

/**
 * Returns whether this unit can carry more of the given type of resource.
 * (This ignores whether the unit is actually able to gather that
 * resource type or not.)
 */
ResourceGatherer.prototype.CanCarryMore = function(type)
{
	var amount = (this.carrying[type] || 0);
	return (amount < this.template.Capacities[type]);
};

/**
 * Returns whether this unit is carrying any resources of a type that is
 * not the requested type. (This is to support cases where the unit is
 * only meant to be able to carry one type at once.)
 */
ResourceGatherer.prototype.IsCarryingAnythingExcept = function(exceptedType)
{
	for (var type in this.carrying)
		if (type != exceptedType)
			return true;

	return false;
};

/**
 * Transfer our carried resources to our owner immediately.
 * Only resources of the given types will be transferred.
 * (This should typically be called after reaching a dropsite).
 */
ResourceGatherer.prototype.CommitResources = function(types)
{
	var cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);

	for each (var type in types)
	{
		if (type in this.carrying)
		{
			cmpPlayer.AddResource(type, this.carrying[type]);
			delete this.carrying[type];
		}
	}
};

/**
 * Drop all currently-carried resources.
 * (Currently they just vanish after being dropped - we don't bother depositing
 * them onto the ground.)
 */
ResourceGatherer.prototype.DropResources = function()
{
	this.carrying = {};
};

Engine.RegisterComponentType(IID_ResourceGatherer, "ResourceGatherer", ResourceGatherer);
