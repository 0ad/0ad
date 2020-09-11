function ResourceSupply() {}

ResourceSupply.prototype.Schema =
	"<a:help>Provides a supply of one particular type of resource.</a:help>" +
	"<a:example>" +
		"<Amount>1000</Amount>" +
		"<Type>food.meat</Type>" +
		"<KillBeforeGather>false</KillBeforeGather>" +
		"<MaxGatherers>25</MaxGatherers>" +
		"<DiminishingReturns>0.8</DiminishingReturns>" +
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

	this.gatherers = [];

	let [type, subtype] = this.template.Type.split('.');
	this.cachedType = { "generic": type, "specific": subtype };
};

ResourceSupply.prototype.IsInfinite = function()
{
	return !isFinite(+this.template.Amount);
};

ResourceSupply.prototype.GetKillBeforeGather = function()
{
	return this.template.KillBeforeGather == "true";
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
	return this.gatherers.length;
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
	// Before changing the amount, activate Fogging if necessary to hide changes
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	if (this.IsInfinite())
		return { "amount": amount, "exhausted": false };

	let oldAmount = this.GetCurrentAmount();
	this.amount = Math.max(0, oldAmount - amount);

	let isExhausted = this.GetCurrentAmount() == 0;
	// Remove entities that have been exhausted
	if (isExhausted)
		Engine.DestroyEntity(this.entity);

	Engine.PostMessage(this.entity, MT_ResourceSupplyChanged, { "from": oldAmount, "to": this.GetCurrentAmount() });

	return { "amount": oldAmount - this.GetCurrentAmount(), "exhausted": isExhausted };
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
	Engine.PostMessage(this.entity, MT_ResourceSupplyNumGatherersChanged, { "to": this.GetNumGatherers() });

	return true;
};

/**
 * @param {number} gathererID - The gatherer's entity id.
 * @todo: Should this return false if the gatherer didn't gather from said resource?
 */
ResourceSupply.prototype.RemoveGatherer = function(gathererID)
{
	let index = this.gatherers.indexOf(gathererID);
	if (index == -1)
		return;

	this.gatherers.splice(index, 1);
	Engine.PostMessage(this.entity, MT_ResourceSupplyNumGatherersChanged, { "to": this.GetNumGatherers() });
};

Engine.RegisterComponentType(IID_ResourceSupply, "ResourceSupply", ResourceSupply);
