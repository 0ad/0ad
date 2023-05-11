function Treasure() {}

Treasure.prototype.Schema =
	"<a:help>Provides a bonus when taken. E.g. a supply of resources.</a:help>" +
	"<a:example>" +
		"<CollectTime>1000</CollectTime>" +
		"<Resources>" +
			"<Food>1000</Food>" +
		"</Resources>" +
	"</a:example>" +
	"<element name='CollectTime' a:help='Amount of milliseconds that it takes to collect this treasure.'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Resources' a:help='Amount of resources that are in this.'>" +
			Resources.BuildSchema("positiveDecimal") +
		"</element>" +
	"</optional>";

Treasure.prototype.Init = function()
{
};

Treasure.prototype.ComputeReward = function()
{
	for (let resource in this.template.Resources)
	{
		let amount = ApplyValueModificationsToEntity("Treasure/Resources/" + resource, this.template.Resources[resource], this.entity);
		if (!amount)
			continue;
		if (!this.resources)
			this.resources = {};
		this.resources[resource] = amount;
	}
};

/**
 * @return {Object} - The resources given by this treasure.
 */
Treasure.prototype.Resources = function()
{
	return this.resources || {};
};

/**
 * @return {number} - The time in milliseconds it takes to collect this treasure.
 */
Treasure.prototype.CollectionTime = function()
{
	return +this.template.CollectTime;
};

/**
 * @param {number} entity - The entity collecting us.
 * @return {boolean} - Whether the reward was granted.
 */
Treasure.prototype.Reward = function(entity)
{
	if (this.isTaken)
		return false;

	let cmpPlayer = QueryOwnerInterface(entity);
	if (!cmpPlayer)
		return false;

	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	if (this.resources)
		cmpPlayer.AddResources(this.resources);

	let cmpStatisticsTracker = QueryOwnerInterface(entity, IID_StatisticsTracker);
	if (cmpStatisticsTracker)
		cmpStatisticsTracker.IncreaseTreasuresCollectedCounter();

	let cmpTrigger = Engine.QueryInterface(SYSTEM_ENTITY, IID_Trigger);
	cmpTrigger.CallEvent("OnTreasureCollected", {
		"player": cmpPlayer.GetPlayerID(),
		"treasure": this.entity
	});

	this.isTaken = true;
	Engine.DestroyEntity(this.entity);
	return true;
};

/**
 * We might live long enough for a collecting entity
 * to find us again after taking us.
 * @return {boolean} - Whether we are taken already.
 */
Treasure.prototype.IsAvailable = function()
{
	return !this.isTaken;
};

Treasure.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to != INVALID_PLAYER)
		this.ComputeReward();
};

Treasure.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Treasure")
		return;
	this.ComputeReward();
};

Engine.RegisterComponentType(IID_Treasure, "Treasure", Treasure);
