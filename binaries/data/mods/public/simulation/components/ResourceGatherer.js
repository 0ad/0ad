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
		Resources.BuildSchema("positiveDecimal", ["treasure"], true) +
	"</element>" +
	"<element name='Capacities' a:help='Per-resource-type maximum carrying capacity'>" +
		Resources.BuildSchema("positiveDecimal") +
	"</element>";

ResourceGatherer.prototype.Init = function()
{
	this.carrying = {}; // { generic type: integer amount currently carried }
	// (Note that this component supports carrying multiple types of resources,
	// each with an independent capacity, but the rest of the game currently
	// ensures and assumes we'll only be carrying one type at once)

	// The last exact type gathered, so we can render appropriate props
	this.lastCarriedType = undefined; // { generic, specific }

	this.RecalculateGatherRatesAndCapacities();
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
	for (let resource of resources)
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

// Since this code is very performancecritical and applying technologies quite slow, cache it.
ResourceGatherer.prototype.RecalculateGatherRatesAndCapacities = function()
{
	let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	let multiplier = cmpPlayer ? cmpPlayer.GetGatherRateMultiplier() : 1;
	this.baseSpeed = multiplier * ApplyValueModificationsToEntity("ResourceGatherer/BaseSpeed", +this.template.BaseSpeed, this.entity);

	this.rates = {};
	for (let r in this.template.Rates)
	{
		let type = r.split(".");

		if (type[0] != "treasure" && type.length > 1 && !Resources.GetResource(type[0]).subtypes[type[1]])
		{
			error("Resource subtype not found: " + type[0] + "." + type[1]);
			continue;
		}

		let rate = ApplyValueModificationsToEntity("ResourceGatherer/Rates/" + r, +this.template.Rates[r], this.entity);
		this.rates[r] = rate * this.baseSpeed;
	}

	this.capacities = {};
	for (let r in this.template.Capacities)
		this.capacities[r] = ApplyValueModificationsToEntity("ResourceGatherer/Capacities/" + r, +this.template.Capacities[r], this.entity);
};

ResourceGatherer.prototype.GetGatherRates = function()
{
	return this.rates;
};

ResourceGatherer.prototype.GetGatherRate = function(resourceType)
{
	if (!this.template.Rates[resourceType])
		return 0;

	return this.rates[resourceType];
};

ResourceGatherer.prototype.GetCapacity = function(resourceType)
{
	if(!this.template.Capacities[resourceType])
		return 0;
	return this.capacities[resourceType];
};

ResourceGatherer.prototype.GetRange = function()
{
	return { "max": +this.template.MaxDistance, "min": 0 };
	// maybe this should depend on the unit or target or something?
};

/**
 * Try to gather treasure
 * @return 'true' if treasure is successfully gathered, otherwise 'false'
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

	// Apply diminishing returns with more gatherers, for e.g. infinite farms. For most resources this has no effect
	// (GetDiminishingReturns will return null). We can assume that for resources that are miraged this is the case
	// (else just add the diminishing returns data to the mirage data and remove the early return above)
	let diminishingReturns = cmpResourceSupply.GetDiminishingReturns();
	if (diminishingReturns)
		rate *= diminishingReturns;

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
	return amount < this.GetCapacity(type);
};


ResourceGatherer.prototype.IsCarrying = function(type)
{
	let amount = (this.carrying[type] || 0);
	return amount > 0;
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
	let cmpPlayer = QueryOwnerInterface(this.entity);

	if (cmpPlayer)
		for (let type of types)
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

// Since we cache gather rates, we need to make sure we update them when tech changes.
// and when our owner change because owners can had different techs.
ResourceGatherer.prototype.OnValueModification = function(msg)
{
	if (msg.component != "ResourceGatherer")
		return;

	this.RecalculateGatherRatesAndCapacities();
};

ResourceGatherer.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to === -1)
		return;
	this.RecalculateGatherRatesAndCapacities();
};

ResourceGatherer.prototype.OnGlobalInitGame = function(msg)
{
	this.RecalculateGatherRatesAndCapacities();
};

Engine.RegisterComponentType(IID_ResourceGatherer, "ResourceGatherer", ResourceGatherer);
