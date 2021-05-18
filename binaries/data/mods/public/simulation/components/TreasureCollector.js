function TreasureCollector() {}

TreasureCollector.prototype.Schema =
	"<a:help>Defines the treasure collecting abilities.</a:help>" +
	"<a:example>" +
		"<MaxDistance>2.0</MaxDistance>" +
	"</a:example>" +
	"<element name='MaxDistance' a:help='The maximum treasure taking distance in m.'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>";

TreasureCollector.prototype.Init = function()
{
};

/**
 * @return {Object} - Min/Max range at which this entity can claim a treasure.
 */
TreasureCollector.prototype.GetRange = function()
{
	return { "min": 0, "max": +this.template.MaxDistance };
};

/**
 * @param {number} target - Entity ID of the target.
 * @return {boolean} - Whether we can collect from the target.
 */
TreasureCollector.prototype.CanCollect = function(target)
{
	let cmpTreasure = Engine.QueryInterface(target, IID_Treasure);
	return cmpTreasure && cmpTreasure.IsAvailable();
};

/**
 * @param {number} target - The target to collect.
 * @param {number} callerIID - The IID to notify on specific events.
 *
 * @return {boolean} - Whether we started collecting.
 */
TreasureCollector.prototype.StartCollecting = function(target, callerIID)
{
	if (this.target)
		this.StopCollecting();

	let cmpTreasure = Engine.QueryInterface(target, IID_Treasure);
	if (!cmpTreasure || !cmpTreasure.IsAvailable())
		return false;

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("collecting_treasure", false, 1.0);

	this.target = target;
	this.callerIID = callerIID;

	// ToDo: Implement rate modifiers.
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetTimeout(this.entity, IID_TreasureCollector, "CollectTreasure", cmpTreasure.CollectionTime(), null);

	return true;
};

/**
 * @param {string} reason - The reason why we stopped collecting, used to notify the caller.
 */
TreasureCollector.prototype.StopCollecting = function(reason)
{
	if (!this.target)
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	cmpTimer.CancelTimer(this.timer);
	delete this.timer;

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
 * @params - Data and lateness are unused.
 */
TreasureCollector.prototype.CollectTreasure = function(data, lateness)
{
	let cmpTreasure = Engine.QueryInterface(this.target, IID_Treasure);
	if (!cmpTreasure || !cmpTreasure.IsAvailable())
	{
		this.StopCollecting("TargetInvalidated");
		return;
	}

	if (!this.IsTargetInRange(this.target))
	{
		this.StopCollecting("OutOfRange");
		return;
	}

	cmpTreasure.Reward(this.entity);
	this.StopCollecting("TargetInvalidated");
};

/**
 * @param {number} - The entity ID of the target to check.
 * @return {boolean} - Whether this entity is in range of its target.
 */
TreasureCollector.prototype.IsTargetInRange = function(target)
{
	let range = this.GetRange();
	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	return cmpObstructionManager.IsInTargetRange(this.entity, target, range.min, range.max, false);
};

Engine.RegisterComponentType(IID_TreasureCollector, "TreasureCollector", TreasureCollector);
