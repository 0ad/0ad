function TreasureCollecter() {}

TreasureCollecter.prototype.Schema =
	"<a:help>Defines the treasure collecting abilities.</a:help>" +
	"<a:example>" +
		"<MaxDistance>2.0</MaxDistance>" +
	"</a:example>" +
	"<element name='MaxDistance' a:help='The maximum treasure taking distance in m.'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>";

TreasureCollecter.prototype.Init = function()
{
};

/**
 * @return {Object} - Min/Max range at which this entity can claim a treasure.
 */
TreasureCollecter.prototype.GetRange = function()
{
	return { "min": 0, "max": +this.template.MaxDistance };
};

/**
 * @param {number} target - Entity ID of the target.
 * @return {boolean} - Whether we can collect from the target.
 */
TreasureCollecter.prototype.CanCollect = function(target)
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
TreasureCollecter.prototype.StartCollecting = function(target, callerIID)
{
	if (this.target)
		this.StopCollecting();

	let cmpTreasure = Engine.QueryInterface(target, IID_Treasure);
	if (!cmpTreasure || !cmpTreasure.IsAvailable())
		return false;

	this.target = target;
	this.callerIID = callerIID;

	// ToDo: Implement rate modifiers.
	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.timer = cmpTimer.SetTimeout(this.entity, IID_TreasureCollecter, "CollectTreasure", cmpTreasure.CollectionTime(), null);

	return true;
};

/**
 * @param {string} reason - The reason why we stopped collecting, used to notify the caller.
 */
TreasureCollecter.prototype.StopCollecting = function(reason)
{
	if (this.timer)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		delete this.timer;
	}
	delete this.target;

	// The callerIID component may start gathering again,
	// replacing the callerIID, which gets deleted after
	// the callerIID has finished. Hence save the data.
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
TreasureCollecter.prototype.CollectTreasure = function(data, lateness)
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
TreasureCollecter.prototype.IsTargetInRange = function(target)
{
	let range = this.GetRange();
	let cmpObstructionManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_ObstructionManager);
	return cmpObstructionManager.IsInTargetRange(this.entity, target, range.min, range.max, false);
};

Engine.RegisterComponentType(IID_TreasureCollecter, "TreasureCollecter", TreasureCollecter);
