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
		Resources.BuildSchema("positiveDecimal", [], true) +
	"</element>" +
	"<element name='Capacities' a:help='Per-resource-type maximum carrying capacity'>" +
		Resources.BuildSchema("positiveDecimal") +
	"</element>";

/*
 * Call interval will be determined by gather rate,
 * so always gather integer amount.
 */
ResourceGatherer.prototype.GATHER_AMOUNT = 1;

ResourceGatherer.prototype.Init = function()
{
	this.capacities = {};
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
	for (let resource of resources)
		this.carrying[resource.type] = +resource.amount;
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

	return undefined;
};

ResourceGatherer.prototype.SetLastCarriedType = function(lastCarriedType)
{
	this.lastCarriedType = lastCarriedType;
};

// Since this code is very performancecritical and applying technologies quite slow, cache it.
ResourceGatherer.prototype.RecalculateGatherRates = function()
{
	this.baseSpeed = ApplyValueModificationsToEntity("ResourceGatherer/BaseSpeed", +this.template.BaseSpeed, this.entity);

	this.rates = {};
	for (let r in this.template.Rates)
	{
		let type = r.split(".");

		if (!Resources.GetResource(type[0]).subtypes[type[1]])
		{
			error("Resource subtype not found: " + type[0] + "." + type[1]);
			continue;
		}

		let rate = ApplyValueModificationsToEntity("ResourceGatherer/Rates/" + r, +this.template.Rates[r], this.entity);
		this.rates[r] = rate * this.baseSpeed;
	}
};

ResourceGatherer.prototype.RecalculateCapacities = function()
{
	this.capacities = {};
	for (let r in this.template.Capacities)
		this.capacities[r] = ApplyValueModificationsToEntity("ResourceGatherer/Capacities/" + r, +this.template.Capacities[r], this.entity);
};

ResourceGatherer.prototype.RecalculateCapacity = function(type)
{
	if (type in this.capacities)
		this.capacities[type] = ApplyValueModificationsToEntity("ResourceGatherer/Capacities/" + type, +this.template.Capacities[type], this.entity);
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
	if (!this.template.Capacities[resourceType])
		return 0;
	return this.capacities[resourceType];
};

ResourceGatherer.prototype.GetRange = function()
{
	return { "max": +this.template.MaxDistance, "min": 0 };
};

/**
 * @param {number} target - The target to gather from.
 * @param {number} callerIID - The IID to notify on specific events.
 * @return {boolean} - Whether we started gathering.
 */
ResourceGatherer.prototype.StartGathering = function(target, callerIID)
{
	if (this.target)
		this.StopGathering();

	let rate = this.GetTargetGatherRate(target);
	if (!rate)
		return false;

	let cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	if (!cmpResourceSupply || !cmpResourceSupply.AddActiveGatherer(this.entity))
		return false;

	let resourceType = cmpResourceSupply.GetType();

	// If we've already got some resources but they're the wrong type,
	// drop them first to ensure we're only ever carrying one type.
	if (this.IsCarryingAnythingExcept(resourceType.generic))
		this.DropResources();

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("gather_" + resourceType.specific, false, 1.0);

	// Calculate timing based on gather rates.
	// This allows the gather rate to control how often we gather, instead of how much.
	let timing = 1000 / rate;

	this.target = target;
	this.callerIID = callerIID;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetInterval(this.entity, IID_ResourceGatherer, "PerformGather", timing, timing, null);

	return true;
};

/**
 * @param {string} reason - The reason why we stopped gathering used to notify the caller.
 */
ResourceGatherer.prototype.StopGathering = function(reason)
{
	if (!this.target)
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	delete this.timer;

	let cmpResourceSupply = Engine.QueryInterface(this.target, IID_ResourceSupply);
	if (cmpResourceSupply)
		cmpResourceSupply.RemoveGatherer(this.entity);

	delete this.target;

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("idle", false, 1.0);

	// The callerIID component may start again,
	// replacing the callerIID, hence save that.
	let callerIID = this.callerIID;
	delete this.callerIID;

	if (reason && callerIID)
	{
		let component = Engine.QueryInterface(this.entity, callerIID);
		if (component)
			component.ProcessMessage(reason, null);
	}
};

/**
 * Gather from our target entity.
 * @params - data and lateness are unused.
 */
ResourceGatherer.prototype.PerformGather = function(data, lateness)
{
	let cmpResourceSupply = Engine.QueryInterface(this.target, IID_ResourceSupply);
	if (!cmpResourceSupply || cmpResourceSupply.GetCurrentAmount() <= 0)
	{
		this.StopGathering("TargetInvalidated");
		return;
	}

	if (!this.IsTargetInRange(this.target))
	{
		this.StopGathering("OutOfRange");
		return;
	}

	// ToDo: Enable entities to keep facing a target.
	Engine.QueryInterface(this.entity, IID_UnitAI)?.FaceTowardsTarget(this.target);

	let type = cmpResourceSupply.GetType();
	if (!this.carrying[type.generic])
		this.carrying[type.generic] = 0;

	let maxGathered = this.GetCapacity(type.generic) - this.carrying[type.generic];
	let status = cmpResourceSupply.TakeResources(Math.min(this.GATHER_AMOUNT, maxGathered));
	this.carrying[type.generic] += status.amount;
	this.lastCarriedType = type;

	// Update stats of how much the player collected.
	// (We have to do it here rather than at the dropsite, because we
	// need to know what subtype it was.)
	let cmpStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseResourceGatheredCounter(type.generic, status.amount, type.specific);

	if (!this.CanCarryMore(type.generic))
		this.StopGathering("InventoryFilled");
	else if (status.exhausted)
		this.StopGathering("TargetInvalidated");
};

/**
 * Compute the amount of resources collected per second from the target.
 * Returns 0 if resources cannot be collected (e.g. the target doesn't
 * exist, or is the wrong type).
 */
ResourceGatherer.prototype.GetTargetGatherRate = function(target)
{
	let cmpResourceSupply = QueryMiragedInterface(target, IID_ResourceSupply);
	if (!cmpResourceSupply || cmpResourceSupply.GetCurrentAmount() <= 0)
		return 0;

	let type = cmpResourceSupply.GetType();

	let rate = 0;
	if (type.specific)
		rate = this.GetGatherRate(type.generic + "." + type.specific);
	if (rate == 0 && type.generic)
		rate = this.GetGatherRate(type.generic);

	let diminishingReturns = cmpResourceSupply.GetDiminishingReturns();
	if (diminishingReturns)
		rate *= diminishingReturns;

	return rate;
};

/**
 * @param {number} target - The entity ID of the target to check.
 * @return {boolean} - Whether we can gather from the target.
 */
ResourceGatherer.prototype.CanGather = function(target)
{
	return this.GetTargetGatherRate(target) > 0;
};

/**
 * Returns whether this unit can carry more of the given type of resource.
 * (This ignores whether the unit is actually able to gather that
 * resource type or not.)
 */
ResourceGatherer.prototype.CanCarryMore = function(type)
{
	let amount = this.carrying[type] || 0;
	return amount < this.GetCapacity(type);
};


ResourceGatherer.prototype.IsCarrying = function(type)
{
	let amount = this.carrying[type] || 0;
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
 * @param {number} target - The entity to check.
 * @param {boolean} checkCarriedResource - Whether we need to check the resource we are carrying.
 * @return {boolean} - Whether we can return carried resources.
 */
ResourceGatherer.prototype.CanReturnResource = function(target, checkCarriedResource)
{
	let cmpResourceDropsite = Engine.QueryInterface(target, IID_ResourceDropsite);
	if (!cmpResourceDropsite)
		return false;

	if (checkCarriedResource)
	{
		let type = this.GetMainCarryingType();
		if (!type || !cmpResourceDropsite.AcceptsType(type))
			return false;
	}

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership && IsOwnedByPlayer(cmpOwnership.GetOwner(), target))
		return true;
	let cmpPlayer = QueryOwnerInterface(this.entity);
	return cmpPlayer && cmpPlayer.HasSharedDropsites() && cmpResourceDropsite.IsShared() &&
	       cmpOwnership && IsOwnedByMutualAllyOfPlayer(cmpOwnership.GetOwner(), target);
};

/**
 * Transfer our carried resources to our owner immediately.
 * Only resources of the appropriate types will be transferred.
 * (This should typically be called after reaching a dropsite.)
 *
 * @param {number} target - The target entity ID to drop resources at.
 */
ResourceGatherer.prototype.CommitResources = function(target)
{
	let cmpResourceDropsite = Engine.QueryInterface(target, IID_ResourceDropsite);
	if (!cmpResourceDropsite)
		return;

	let change = cmpResourceDropsite.ReceiveResources(this.carrying, this.entity);
	for (let type in change)
	{
		this.carrying[type] -= change[type];
		if (this.carrying[type] == 0)
			delete this.carrying[type];
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

/**
 * @return {string} - A generic resource type if we were tasked to gather.
 */
ResourceGatherer.prototype.GetTaskedResourceType = function()
{
	return this.taskedResourceType;
};

/**
 * @param {string} type - A generic resource type.
 */
ResourceGatherer.prototype.AddToPlayerCounter = function(type)
{
	// We need to be removed from the player counter first.
	if (this.taskedResourceType)
		return;

	let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	if (cmpPlayer)
		cmpPlayer.AddResourceGatherer(type);

	this.taskedResourceType = type;
};

/**
 * @param {number} playerid - Optionally a player ID.
 */
ResourceGatherer.prototype.RemoveFromPlayerCounter = function(playerid)
{
	if (!this.taskedResourceType)
		return;

	let cmpPlayer = playerid != undefined ?
		QueryPlayerIDInterface(playerid) :
		QueryOwnerInterface(this.entity, IID_Player);

	if (cmpPlayer)
		cmpPlayer.RemoveResourceGatherer(this.taskedResourceType);

	delete this.taskedResourceType;
};

/**
 * @param {number} - The entity ID of the target to check.
 * @return {boolean} - Whether this entity is in range of its target.
 */
ResourceGatherer.prototype.IsTargetInRange = function(target)
{
	return Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager).
		IsInTargetRange(this.entity, target, 0, +this.template.MaxDistance, false);
};

// Since we cache gather rates, we need to make sure we update them when tech changes.
// and when our owner change because owners can had different techs.
ResourceGatherer.prototype.OnValueModification = function(msg)
{
	if (msg.component != "ResourceGatherer")
		return;

	// NB: at the moment, 0 A.D. always uses the fast path, the other is mod support.
	if (msg.valueNames.length === 1)
	{
		if (msg.valueNames[0].indexOf("Capacities") !== -1)
			this.RecalculateCapacity(msg.valueNames[0].substr(28));
		else
			this.RecalculateGatherRates();
	}
	else
	{
		this.RecalculateGatherRates();
		this.RecalculateCapacities();
	}
};

ResourceGatherer.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to == INVALID_PLAYER)
	{
		this.RemoveFromPlayerCounter(msg.from);
		return;
	}
	if (this.lastGathered && msg.from !== INVALID_PLAYER)
	{
		const resource = this.taskedResourceType;
		this.RemoveFromPlayerCounter(msg.from);
		this.AddToPlayerCounter(resource);
	}

	this.RecalculateGatherRates();
	this.RecalculateCapacities();
};

ResourceGatherer.prototype.OnGlobalInitGame = function(msg)
{
	this.RecalculateGatherRates();
	this.RecalculateCapacities();
};

ResourceGatherer.prototype.OnMultiplierChanged = function(msg)
{
	let cmpPlayer = QueryOwnerInterface(this.entity, IID_Player);
	if (cmpPlayer && msg.player == cmpPlayer.GetPlayerID())
		this.RecalculateGatherRates();
};

Engine.RegisterComponentType(IID_ResourceGatherer, "ResourceGatherer", ResourceGatherer);
