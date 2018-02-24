function Gate() {}

Gate.prototype.Schema =
	"<a:help>Controls behavior of wall gates</a:help>" +
	"<a:example>" +
		"<PassRange>20</PassRange>" +
	"</a:example>" +
	"<element name='PassRange' a:help='Units must be within this distance (in meters) of the gate for it to open'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

/**
 * Initialize Gate component
 */
Gate.prototype.Init = function()
{
	this.allies = [];
	this.opened = false;
	this.locked = false;
};

Gate.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to != INVALID_PLAYER)
	{
		this.SetupRangeQuery(msg.to);
		// Set the initial state, but don't play unlocking sound
		if (!this.locked)
			this.UnlockGate(true);
	}
};

Gate.prototype.OnDiplomacyChanged = function(msg)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership && cmpOwnership.GetOwner() == msg.player)
	{
		this.allies = [];
		this.SetupRangeQuery(msg.player);
	}
};

/**
 * Cleanup on destroy
 */
Gate.prototype.OnDestroy = function()
{
	// Clean up range query
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.unitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.unitsQuery);

	// Cancel the closing-blocked timer if it's running.
	if (this.timer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		this.timer = undefined;
	}
};

/**
 * Setup the range query to detect units coming in & out of range
 */
Gate.prototype.SetupRangeQuery = function(owner)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	if (this.unitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.unitsQuery);

	// Only allied units can make the gate open.
	var players = QueryPlayerIDInterface(owner).GetAllies();

	var range = this.GetPassRange();
	if (range > 0)
	{
		// Only find entities with IID_UnitAI interface
		this.unitsQuery = cmpRangeManager.CreateActiveQuery(this.entity, 0, range, players, IID_UnitAI, cmpRangeManager.GetEntityFlagMask("normal"));
		cmpRangeManager.EnableActiveQuery(this.unitsQuery);
	}
};

/**
 * Called when units enter or leave range
 */
Gate.prototype.OnRangeUpdate = function(msg)
{
	if (msg.tag != this.unitsQuery)
		return;

	if (msg.added.length > 0)
		for (let entity of msg.added)
			this.allies.push(entity);

	if (msg.removed.length > 0)
		for (let entity of msg.removed)
			this.allies.splice(this.allies.indexOf(entity), 1);

	this.OperateGate();
};

/**
 * Get the range in which units are detected
 */
Gate.prototype.GetPassRange = function()
{
	return +this.template.PassRange;
};

/**
 * Attempt to open or close the gate.
 * An ally must be in range to open the gate, but an unlocked gate will only close
 * if there are no allies in range and no units are inside the gate's obstruction.
 */
Gate.prototype.OperateGate = function()
{
	// Cancel the closing-blocked timer if it's running.
	if (this.timer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		this.timer = undefined;
	}

	if (this.opened && (this.allies.length == 0 || this.locked))
		this.CloseGate();
	else if (!this.opened && this.allies.length)
		this.OpenGate();
};

Gate.prototype.IsLocked = function()
{
	return this.locked;
};

/**
 * Lock the gate, with sound. It will close at the next opportunity.
 */
Gate.prototype.LockGate = function()
{
	this.locked = true;
	// If the door is closed, enable 'block pathfinding'
	// Else 'block pathfinding' will be enabled the next time the gate close
	if (!this.opened)
	{
		var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
		if (!cmpObstruction)
			return;
		cmpObstruction.SetDisableBlockMovementPathfinding(false, false, 0);
	}
	else
		this.OperateGate();

	// TODO: Possibly move the lock/unlock sounds to UI? Needs testing
	PlaySound("gate_locked", this.entity);
};

/**
 * Unlock the gate, with sound. May open the gate if allied units are within range.
 * If quiet is true, no sound will be played (used for initial setup).
 */
Gate.prototype.UnlockGate = function(quiet)
{
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return;

	// Disable 'block pathfinding'
	cmpObstruction.SetDisableBlockMovementPathfinding(this.opened, true, 0);
	this.locked = false;

	// TODO: Possibly move the lock/unlock sounds to UI? Needs testing
	if (!quiet)
		PlaySound("gate_unlocked", this.entity);

	// If the gate is closed, open it if necessary
	if (!this.opened)
		this.OperateGate();
};

/**
 * Open the gate if unlocked, with sound and animation.
 */
Gate.prototype.OpenGate = function()
{
	// Do not open the gate if it has been locked
	if (this.locked)
		return;

	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return;

	// Disable 'block movement'
	cmpObstruction.SetDisableBlockMovementPathfinding(true, true, 0);
	this.opened = true;

	PlaySound("gate_opening", this.entity);
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("gate_opening", true, 1.0);
};

/**
 * Close the gate, with sound and animation.
 *
 * The gate may fail to close due to unit obstruction. If this occurs, the
 * gate will start a timer and attempt to close on each simulation update.
 */
Gate.prototype.CloseGate = function()
{
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return;

	// The gate can't be closed if there are entities colliding with it.
	var collisions = cmpObstruction.GetUnitCollisions();
	if (collisions.length)
	{
		if (!this.timer)
		{
			// Set an "instant" timer which will run on the next simulation turn.
			var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			this.timer = cmpTimer.SetTimeout(this.entity, IID_Gate, "OperateGate", 0, {});
		}
		return;
	}

	// If we ordered the gate to be locked, enable 'block movement' and 'block pathfinding'
	if (this.locked)
		cmpObstruction.SetDisableBlockMovementPathfinding(false, false, 0);
	// Else just enable 'block movement'
	else
		cmpObstruction.SetDisableBlockMovementPathfinding(false, true, 0);
	this.opened = false;

	PlaySound("gate_closing", this.entity);
	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SelectAnimation("gate_closing", true, 1.0);
};

Engine.RegisterComponentType(IID_Gate, "Gate", Gate);
