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
			"<optional><element name='treasure' a:help='Treasure gather rate (only presense on value makes sense, size is only used to determine the delay before gathering, so it should be set to 1)'><ref name='positiveDecimal'/></element></optional>" +
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
	let ret = [];
	for (let type in this.carrying)
	{
		ret.push({
			"type": type,
			"amount": this.carrying[type],
			"max": +this.GetCapacity(type)
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
	for each (let resource in resources)
	{
		this.carrying[resource.type] = +(resource.amount);
	}

	Engine.PostMessage(this.entity, MT_ResourceCarryingChanged, { "to": this.GetCarryingStatus() });
};

/**
 * Returns the generic type of one particular resource this unit is
 * currently carrying, or undefined if none.
 */
ResourceGatherer.prototype.GetMainCarryingType = function()
{
	// Return the first key, if any
	for (let type in this.carrying)
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

ResourceGatherer.prototype.GetBaseSpeed = function()
{
	let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	let multiplier = cmpPlayer ? cmpPlayer.GetGatherRateMultiplier() : 1;
	return multiplier * ApplyValueModificationsToEntity("ResourceGatherer/BaseSpeed", +this.template.BaseSpeed, this.entity);
};

ResourceGatherer.prototype.GetGatherRates = function()
{
	let ret = {};
	let baseSpeed = this.GetBaseSpeed();

	for (let r in this.template.Rates)
	{
		let rate = ApplyValueModificationsToEntity("ResourceGatherer/Rates/" + r, +this.template.Rates[r], this.entity);
		ret[r] = rate * baseSpeed;
	}

	return ret;
};

ResourceGatherer.prototype.GetGatherRate = function(resourceType)
{
	if (!this.template.Rates[resourceType])
		return 0;

	let baseSpeed = this.GetBaseSpeed();
	let rate = ApplyValueModificationsToEntity("ResourceGatherer/Rates/" + resourceType, +this.template.Rates[resourceType], this.entity);
	return rate * baseSpeed;
};

ResourceGatherer.prototype.GetCapacity = function(resourceType)
{
	if(!this.template.Capacities[resourceType])
		return 0;
	return ApplyValueModificationsToEntity("ResourceGatherer/Capacities/" + resourceType, +this.template.Capacities[resourceType], this.entity);
};

ResourceGatherer.prototype.GetRange = function()
{
	return { "max": +this.template.MaxDistance, "min": 0 };
	// maybe this should depend on the unit or target or something?
};

/**
 * Try to gather treasure
 * @return 'true' if treasure is successfully gathered and 'false' in the other case
 */
ResourceGatherer.prototype.TryInstantGather = function(target)
{
	let cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	let type = cmpResourceSupply.GetType();

	if (type.generic != "treasure")
		return false;

	let status = cmpResourceSupply.TakeResources(cmpResourceSupply.GetCurrentAmount());

	let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	if (cmpPlayer)
		cmpPlayer.AddResource(type.specific, status.amount);

	let cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseTreasuresCollectedCounter();

	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);  
	if (cmpTrigger && cmpPlayer) 
		cmpTrigger.CallEvent("TreasureCollected", {"player": cmpPlayer.GetPlayerID(), "type": type.specific, "amount": status.amount });  

	return true;
};

/**
 * Gather from the target entity. This should only be called after a successful range check,
 * and if the target has a compatible ResourceSupply.
 * Call interval will be determined by gather rate, so always gather 1 amount when called.
 */
ResourceGatherer.prototype.PerformGather = function(target)
{
	if (!this.GetTargetGatherRate(target))
		return { "exhausted": true };

	let gatherAmount = 1;

	let cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	let type = cmpResourceSupply.GetType();

	// Initialise the carried count if necessary
	if (!this.carrying[type.generic])
		this.carrying[type.generic] = 0;

	// Find the maximum so we won't exceed our capacity
	let maxGathered = this.GetCapacity(type.generic) - this.carrying[type.generic];

	let status = cmpResourceSupply.TakeResources(Math.min(gatherAmount, maxGathered));

	this.carrying[type.generic] += status.amount;

	this.lastCarriedType = type;

	// Update stats of how much the player collected.
	// (We have to do it here rather than at the dropsite, because we
	// need to know what subtype it was)
	let cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseResourceGatheredCounter(type.generic, status.amount, type.specific);

	Engine.PostMessage(this.entity, MT_ResourceCarryingChanged, { "to": this.GetCarryingStatus() });

	// Tell the target we're gathering from it
	Engine.PostMessage(target, MT_ResourceGather,
		{ "entity": target, "gatherer": this.entity });

	return {
		"amount": status.amount,
		"exhausted": status.exhausted,
		"filled": (this.carrying[type.generic] >= this.GetCapacity(type.generic))
	};
};

/**
 * Compute the amount of resources collected per second from the target.
 * Returns 0 if resources cannot be collected (e.g. the target doesn't
 * exist, or is the wrong type).
 */
ResourceGatherer.prototype.GetTargetGatherRate = function(target)
{
	let cmpResourceSupply = QueryMiragedInterface(target, IID_ResourceSupply);
	if (!cmpResourceSupply)
		return 0;

	let type = cmpResourceSupply.GetType();

	let rate = 0;
	if (type.specific)
		rate = this.GetGatherRate(type.generic+"."+type.specific);
	if (rate == 0 && type.generic)
		rate = this.GetGatherRate(type.generic);

	let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	let cheatMultiplier = cmpPlayer ? cmpPlayer.GetCheatTimeMultiplier() : 1;
	rate = rate / cheatMultiplier;

	if ("Mirages" in cmpResourceSupply)
		return rate;

	// Apply diminishing returns with more gatherers, for e.g. infinite farms. For most resources this has no effect. (GetDiminishingReturns will return null.)
	// We can assume that for resources that are miraged this is the case. (else just add the diminishing returns data to the mirage data and remove the
	// early return above)
	// Note to people looking to change <DiminishingReturns> in a template: This is a bit complicated. Basically, the lower that number is
	// the steeper diminishing returns will be. I suggest playing around with Wolfram Alpha or a graphing calculator a bit.
	// In each of the following links, replace 0.65 with the gather rate of your worker for the resource with diminishing returns and
	// 14 with the constant you wish to use to control the diminishing returns.
	// (In this case 0.65 is the women farming rate, in resources/second, and 14 is a good constant for farming.)
	// This is the gather rate in resources/second of each individual worker as the total number of workers goes up:
	// http://www.wolframalpha.com/input/?i=plot+%281%2F2+cos%28%28x-1%29*pi%2F14%29+%2B+1%2F2%29+*+0.65+from+1+to+5
	// This is the total output of the resource in resources/second:
	// http://www.wolframalpha.com/input/?i=plot+x%281%2F2+cos%28%28x-1%29*pi%2F14%29+%2B+1%2F2%29+*+0.65+from+1+to+5
	// This is the fraction of a worker each new worker is worth (the 5th worker in this example is only producing about half as much as the first one):
	// http://www.wolframalpha.com/input/?i=plot+x%281%2F2+cos%28%28x-1%29*pi%2F14%29+%2B+1%2F2%29+-++%28x-1%29%281%2F2+cos%28%28x-2%29*pi%2F14%29+%2B+1%2F2%29+from+x%3D1+to+5+and+y%3D0+to+1
	// Here's how this technically works:
	// The cosine function is an oscillating curve, normally between -1 and 1. Multiplying by 0.5 squishes that down to
	// between -0.5 and 0.5. Adding 0.5 to that changes the range to 0 to 1. The diminishingReturns constant
	// adjusts the period of the curve.
	let diminishingReturns = cmpResourceSupply.GetDiminishingReturns();
	if (diminishingReturns)
		rate = (0.5 * Math.cos((cmpResourceSupply.GetNumGatherers() - 1) * Math.PI / diminishingReturns) + 0.5) * rate;

	return rate;
};

/**
 * Returns whether this unit can carry more of the given type of resource.
 * (This ignores whether the unit is actually able to gather that
 * resource type or not.)
 */
ResourceGatherer.prototype.CanCarryMore = function(type)
{
	let amount = (this.carrying[type] || 0);
	return (amount < this.GetCapacity(type));
};

/**
 * Returns whether this unit is carrying any resources of a type that is
 * not the requested type. (This is to support cases where the unit is
 * only meant to be able to carry one type at once.)
 */
ResourceGatherer.prototype.IsCarryingAnythingExcept = function(exceptedType)
{
	for (let type in this.carrying)
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
	let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);

	if (cmpPlayer)
		for each (let type in types)
			if (type in this.carrying)
			{
				cmpPlayer.AddResource(type, this.carrying[type]);
				delete this.carrying[type];
			}

	Engine.PostMessage(this.entity, MT_ResourceCarryingChanged, { "to": this.GetCarryingStatus() });
};

/**
 * Drop all currently-carried resources.
 * (Currently they just vanish after being dropped - we don't bother depositing
 * them onto the ground.)
 */
ResourceGatherer.prototype.DropResources = function()
{
	this.carrying = {};

	Engine.PostMessage(this.entity, MT_ResourceCarryingChanged, { "to": this.GetCarryingStatus() });
};

Engine.RegisterComponentType(IID_ResourceGatherer, "ResourceGatherer", ResourceGatherer);
