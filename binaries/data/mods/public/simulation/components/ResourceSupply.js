function ResourceSupply() {}

ResourceSupply.prototype.Schema =
	"<a:help>Provides a supply of one particular type of resource.</a:help>" +
	"<a:example>" +
		"<Max>1000</Max>" +
		"<Initial>1000</Initial>" +
		"<Type>food.meat</Type>" +
		"<KillBeforeGather>false</KillBeforeGather>" +
		"<MaxGatherers>25</MaxGatherers>" +
		"<DiminishingReturns>0.8</DiminishingReturns>" +
		"<Change>" +
			"<AnyName>" +
				"<Value>2</Value>" +
				"<Interval>1000</Interval>" +
			"</AnyName>" +
			"<Growth>" +
				"<State>alive</State>" +
				"<Value>2</Value>" +
				"<Interval>1000</Interval>" +
				"<UpperLimit>500</UpperLimit>" +
			"</Growth>" +
			"<Rotting>" +
				"<State>dead notGathered</State>" +
				"<Value>-2</Value>" +
				"<Interval>1000</Interval>" +
			"</Rotting>" +
			"<Decay>" +
				"<State>dead</State>" +
				"<Value>-1</Value>" +
				"<Interval>1000</Interval>" +
				"<LowerLimit>500</LowerLimit>" +
			"</Decay>" +
		"</Change>" +
	"</a:example>" +
	"<element name='KillBeforeGather' a:help='Whether this entity must be killed (health reduced to 0) before its resources can be gathered'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='Max' a:help='Max amount of resources available from this entity.'>" +
		"<choice><data type='nonNegativeInteger'/><value>Infinity</value></choice>" +
	"</element>" +
	"<optional>" +
		"<element name='Initial' a:help='Initial amount of resources available from this entity, if this is not specified, Max is used.'>" +
			"<choice><data type='nonNegativeInteger'/><value>Infinity</value></choice>" +
		"</element>" +
	"</optional>" +
	"<element name='Type' a:help='Type and Subtype of resource available from this entity'>" +
		Resources.BuildChoicesSchema(true) +
	"</element>" +
	"<element name='MaxGatherers' a:help='Amount of gatherers who can gather resources from this entity at the same time'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<optional>" +
		"<element name='DiminishingReturns' a:help='The relative rate of any new gatherer compared to the previous one (geometric sequence). Leave the element out for no diminishing returns.'>" +
			"<ref name='positiveDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Change' a:help='Optional element containing all the modifications that affects a resource supply'>" +
			"<oneOrMore>" +
				"<element a:help='Element defining whether and how a resource supply regenerates or decays'>" +
					"<anyName/>" +
					"<interleave>" +
						"<element name='Value' a:help='The amount of resource added per interval.'>" +
							"<data type='integer'/>" +
						"</element>" +
						"<element name='Interval' a:help='The interval in milliseconds.'>" +
							"<data type='positiveInteger'/>" +
						"</element>" +
						"<optional>" +
							"<element name='UpperLimit' a:help='The upper limit of the value after which the Change has no effect.'>" +
								"<data type='nonNegativeInteger'/>" +
							"</element>" +
						"</optional>" +
						"<optional>" +
							"<element name='LowerLimit' a:help='The bottom limit of the value after which the Change has no effect.'>" +
								"<data type='nonNegativeInteger'/>" +
							"</element>" +
						"</optional>" +
						"<optional>" +
							"<element name='State' a:help='What state the entity must be in for the effect to occur.'>" +
								"<list>" +
									"<oneOrMore>" +
										"<choice>" +
											"<value>alive</value>" +
											"<value>dead</value>" +
											"<value>gathered</value>" +
											"<value>notGathered</value>" +
										"</choice>" +
									"</oneOrMore>" +
								"</list>" +
							"</element>" +
						"</optional>" +
					"</interleave>" +
				"</element>" +
			"</oneOrMore>" +
		"</element>" +
	"</optional>";

ResourceSupply.prototype.Init = function()
{
	this.amount = +(this.template.Initial || this.template.Max);

	// Includes the ones that are tasked but not here yet, i.e. approaching.
	this.gatherers = [];
	this.activeGatherers = [];

	let [type, subtype] = this.template.Type.split('.');
	this.cachedType = { "generic": type, "specific": subtype };

	if (this.template.Change)
	{
		this.timers = {};
		this.cachedChanges = {};
	}
};

ResourceSupply.prototype.IsInfinite = function()
{
	return !isFinite(+this.template.Max);
};

ResourceSupply.prototype.GetKillBeforeGather = function()
{
	return this.template.KillBeforeGather == "true";
};

ResourceSupply.prototype.GetMaxAmount = function()
{
	return this.maxAmount;
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
	return this.gatherers.length;
};

/**
 * @return {number} - The number of currently active gatherers.
 */
ResourceSupply.prototype.GetNumActiveGatherers = function()
{
	return this.activeGatherers.length;
};

/**
 * @return {{ "generic": string, "specific": string }} An object containing the subtype and the generic type. All resources must have both.
 */
ResourceSupply.prototype.GetType = function()
{
	return this.cachedType;
};

/**
 * @param {number} gathererID - The gatherer's entity id.
 * @return {boolean} - Whether the ResourceSupply can have this additional gatherer or it is already gathering.
 */
ResourceSupply.prototype.IsAvailableTo = function(gathererID)
{
	return this.IsAvailable() || this.IsGatheringUs(gathererID);
};

/**
 * @return {boolean} - Whether this entity can have an additional gatherer.
 */
ResourceSupply.prototype.IsAvailable = function()
{
	return this.amount && this.gatherers.length < this.GetMaxGatherers();
};

/**
 * @param {number} entity - The entityID to check for.
 * @return {boolean} - Whether the given entity is already gathering at us.
 */
ResourceSupply.prototype.IsGatheringUs = function(entity)
{
	return this.gatherers.indexOf(entity) !== -1;
};

/**
 * Each additional gatherer decreases the rate following a geometric sequence, with diminishingReturns as ratio.
 * @return {number} The diminishing return if any, null otherwise.
 */
ResourceSupply.prototype.GetDiminishingReturns = function()
{
	if (!this.template.DiminishingReturns)
		return null;

	let diminishingReturns = ApplyValueModificationsToEntity("ResourceSupply/DiminishingReturns", +this.template.DiminishingReturns, this.entity);
	if (!diminishingReturns)
		return null;

	let numGatherers = this.GetNumGatherers();
	if (numGatherers > 1)
		return diminishingReturns == 1 ? 1 : (1 - Math.pow(diminishingReturns, numGatherers)) / (1 - diminishingReturns) / numGatherers;

	return null;
};

/**
 * @param {number} amount The amount of resources that should be taken from the resource supply. The amount must be positive.
 * @return {{ "amount": number, "exhausted": boolean }} The current resource amount in the entity and whether it's exhausted or not.
 */
ResourceSupply.prototype.TakeResources = function(amount)
{
	if (this.IsInfinite())
		return { "amount": amount, "exhausted": false };

	return {
		"amount": Math.abs(this.Change(-amount)),
		"exhausted": this.amount == 0
	};
};

/**
 * @param {number} change - The amount to change the resources with (can be negative).
 * @return {number} - The actual change in resourceSupply.
 */
ResourceSupply.prototype.Change = function(change)
{
	// Before changing the amount, activate Fogging if necessary to hide changes
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	let oldAmount = this.amount;
	this.amount = Math.min(Math.max(oldAmount + change, 0), this.maxAmount);

	// Remove entities that have been exhausted.
	if (this.amount == 0)
		Engine.DestroyEntity(this.entity);

	let actualChange = this.amount - oldAmount;
	if (actualChange != 0)
	{
		Engine.PostMessage(this.entity, MT_ResourceSupplyChanged, {
			"from": oldAmount,
			"to": this.amount
		});
		this.CheckTimers();
	}
	return actualChange;
};

/**
 * @param {number} newValue - The value to set the current amount to.
 */
ResourceSupply.prototype.SetAmount = function(newValue)
{
	// We currently don't support changing to or fro Infinity.
	if (this.IsInfinite() || newValue === Infinity)
		return;
	this.Change(newValue - this.amount);
};

/**
 * @param {number} gathererID - The gatherer to add.
 * @return {boolean} - Whether the gatherer was successfully added to the entity's gatherers list
 *			or the entity was already gathering us.
 */
ResourceSupply.prototype.AddGatherer = function(gathererID)
{
	if (!this.IsAvailable())
		return false;

	if (this.IsGatheringUs(gathererID))
		return true;

	this.gatherers.push(gathererID);

	return true;
};

/**
 * @param {number} player - The playerID owning the gatherer.
 * @param {number} entity - The entityID gathering.
 *
 * @return {boolean} - Whether the gatherer was successfully added to the active-gatherers list
 *			or the entity was already in that list.
 */
ResourceSupply.prototype.AddActiveGatherer = function(entity)
{
	if (!this.AddGatherer(entity))
		return false;

	if (this.activeGatherers.indexOf(entity) == -1)
	{
		this.activeGatherers.push(entity);
		this.CheckTimers();
	}
	return true;
};

/**
 * @param {number} gathererID - The gatherer's entity id.
 * @todo: Should this return false if the gatherer didn't gather from said resource?
 */
ResourceSupply.prototype.RemoveGatherer = function(gathererID)
{
	let index = this.gatherers.indexOf(gathererID);
	if (index != -1)
		this.gatherers.splice(index, 1);

	index = this.activeGatherers.indexOf(gathererID);
	if (index == -1)
		return;
	this.activeGatherers.splice(index, 1);
	this.CheckTimers();
};

/**
 * Checks whether a timer ought to be added or removed.
 */
ResourceSupply.prototype.CheckTimers = function()
{
	if (!this.template.Change || this.IsInfinite())
		return;

	for (let changeKey in this.template.Change)
	{
		if (!this.CheckState(changeKey))
		{
			this.StopTimer(changeKey);
			continue;
		}
		let template = this.template.Change[changeKey];
		if (this.amount < +(template.LowerLimit || -1) ||
			this.amount > +(template.UpperLimit || this.GetMaxAmount()))
		{
			this.StopTimer(changeKey);
			continue;
		}

		if (this.cachedChanges[changeKey] == 0)
		{
			this.StopTimer(changeKey);
			continue;
		}

		if (!this.timers[changeKey])
			this.StartTimer(changeKey);
	}
};

/**
 * This verifies whether the current state of the supply matches the ones needed
 * for the specific timer to run.
 *
 * @param {string} changeKey - The name of the Change to verify the state for.
 * @return {boolean} - Whether the timer may run.
 */
ResourceSupply.prototype.CheckState = function(changeKey)
{
	let template = this.template.Change[changeKey];
	if (!template.State)
		return true;

	let states = template.State;
	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);
	if (states.indexOf("alive") != -1 && !cmpHealth && states.indexOf("dead") == -1 ||
		states.indexOf("dead") != -1 && cmpHealth && states.indexOf("alive") == -1)
		return false;

	let activeGatherers = this.GetNumActiveGatherers();
	if (states.indexOf("gathered") != -1 && activeGatherers == 0 && states.indexOf("notGathered") == -1 ||
		states.indexOf("notGathered") != -1 && activeGatherers > 0 && states.indexOf("gathered") == -1)
		return false;

	return true;
};

/**
 * @param {string} changeKey - The name of the Change to apply to the entity.
 */
ResourceSupply.prototype.StartTimer = function(changeKey)
{
	if (this.timers[changeKey])
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	let interval = ApplyValueModificationsToEntity("ResourceSupply/Change/" + changeKey + "/Interval", +(this.template.Change[changeKey].Interval || 1000), this.entity);
	this.timers[changeKey] = cmpTimer.SetInterval(this.entity, IID_ResourceSupply, "TimerTick", interval, interval, changeKey);
};

/**
 * @param {string} changeKey - The name of the change to stop the timer for.
 */
ResourceSupply.prototype.StopTimer = function(changeKey)
{
	if (!this.timers[changeKey])
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timers[changeKey]);
	delete this.timers[changeKey];
};

/**
 * @param {string} changeKey - The name of the change to apply to the entity.
 */
ResourceSupply.prototype.TimerTick = function(changeKey)
{
	let template = this.template.Change[changeKey];
	if (!template || !this.Change(this.cachedChanges[changeKey]))
		this.StopTimer(changeKey);
};

/**
 * Since the supposed changes can be affected by modifications, and applying those
 * are slow, do not calculate them every timer tick.
 */
ResourceSupply.prototype.RecalculateValues = function()
{
	this.maxAmount = ApplyValueModificationsToEntity("ResourceSupply/Max", +this.template.Max, this.entity);
	if (!this.template.Change || this.IsInfinite())
		return;

	for (let changeKey in this.template.Change)
		this.cachedChanges[changeKey] = ApplyValueModificationsToEntity("ResourceSupply/Change/" + changeKey + "/Value", +this.template.Change[changeKey].Value, this.entity);

	this.CheckTimers();
};

/**
 * @param {{ "component": string, "valueNames": string[] }} msg - Message containing a list of values that were changed.
 */
ResourceSupply.prototype.OnValueModification = function(msg)
{
	if (msg.component != "ResourceSupply")
		return;

	this.RecalculateValues();
};

/**
 * @param {{ "from": number, "to": number }} msg - Message containing the old new owner.
 */
ResourceSupply.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to == INVALID_PLAYER)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		for (let changeKey in this.timers)
			cmpTimer.CancelTimer(this.timers[changeKey]);
	}
	else
		this.RecalculateValues();
};

/**
 * @param {{ "entity": number, "newentity": number }} msg - Message to what the entity has been renamed.
 */
ResourceSupply.prototype.OnEntityRenamed = function(msg)
{
	let cmpResourceSupplyNew = Engine.QueryInterface(msg.newentity, IID_ResourceSupply);
	if (cmpResourceSupplyNew)
		cmpResourceSupplyNew.SetAmount(this.GetCurrentAmount());
};

function ResourceSupplyMirage() {}
ResourceSupplyMirage.prototype.Init = function(cmpResourceSupply)
{
	this.maxAmount = cmpResourceSupply.GetMaxAmount();
	this.amount = cmpResourceSupply.GetCurrentAmount();
	this.type = cmpResourceSupply.GetType();
	this.isInfinite = cmpResourceSupply.IsInfinite();
	this.killBeforeGather = cmpResourceSupply.GetKillBeforeGather();
	this.maxGatherers = cmpResourceSupply.GetMaxGatherers();
	this.numGatherers = cmpResourceSupply.GetNumGatherers();
};

ResourceSupplyMirage.prototype.GetMaxAmount = function() { return this.maxAmount; };
ResourceSupplyMirage.prototype.GetCurrentAmount = function() { return this.amount; };
ResourceSupplyMirage.prototype.GetType = function() { return this.type; };
ResourceSupplyMirage.prototype.IsInfinite = function() { return this.isInfinite; };
ResourceSupplyMirage.prototype.GetKillBeforeGather = function() { return this.killBeforeGather; };
ResourceSupplyMirage.prototype.GetMaxGatherers = function() { return this.maxGatherers; };
ResourceSupplyMirage.prototype.GetNumGatherers = function() { return this.numGatherers; };

// Apply diminishing returns with more gatherers, for e.g. infinite farms. For most resources this has no effect
// (GetDiminishingReturns will return null). We can assume that for resources that are miraged this is the case.
ResourceSupplyMirage.prototype.GetDiminishingReturns = function() { return null; };

Engine.RegisterGlobal("ResourceSupplyMirage", ResourceSupplyMirage);

ResourceSupply.prototype.Mirage = function()
{
	let mirage = new ResourceSupplyMirage();
	mirage.Init(this);
	return mirage;
};

Engine.RegisterComponentType(IID_ResourceSupply, "ResourceSupply", ResourceSupply);
