function Gate() {}

Gate.prototype.Schema =
	"<element name='PassRange'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

/**
 * Initialize Gate Component
 */
Gate.prototype.Init = function()
{
	this.units = [];
	this.opened = false;
	this.locked = false;
};

Gate.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to != -1)
	{
		this.SetupRangeQuery(msg.to);
		if (!this.locked)
			this.UnlockGate();
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
};

/**
 * Setup the Range Query to detect units coming in & out of range
 */
Gate.prototype.SetupRangeQuery = function(owner)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (this.unitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.unitsQuery);
	var players = []
	
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(owner), IID_Player);
	var numPlayers = cmpPlayerManager.GetNumPlayers();
		
	for (var i = 1; i < numPlayers; ++i)
	{
		players.push(i);
	}
	
	if (this.GetPassRange() > 0)
	{
		var range = this.GetPassRange();
		this.unitsQuery = cmpRangeManager.CreateActiveQuery(this.entity, 0, range, players, 0, cmpRangeManager.GetEntityFlagMask("normal"));
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
	{
		for each (var entity in msg.added)
		{
			var cmpIdentity = Engine.QueryInterface(entity, IID_Identity);
			if(cmpIdentity)
			{
				var classes = cmpIdentity.GetClassesList();
				if(classes.indexOf("Unit") != -1)
					this.units.push(entity);
			}
		}
	}
	if (msg.removed.length > 0)
	{
		for each (var entity in msg.removed)
		{
			this.units.splice(this.units.indexOf(entity), 1);
		}
	}

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
 * Open or close the gate
 */
Gate.prototype.OperateGate = function()
{
	if (this.opened == true )
	{
		// If no units are in range, close the gate
		if (this.units.length == 0)
		{
			this.CloseGate();
		}
	}
	else
	{
		// If one units in range is owned by an ally, open the gate
		for each (var ent in this.units)
		{
			if (IsOwnedByAllyOfEntity(this.entity, ent))
			{
				this.OpenGate();
				break;
			}
		}
	}
};

Gate.prototype.IsLocked = function()
{
	return this.locked;
};

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
};

Gate.prototype.UnlockGate = function()
{
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return;

	// Disable 'block pathfinding'
	cmpObstruction.SetDisableBlockMovementPathfinding(false, true, 0);
	this.locked = false;

	// If the gate is closed, open it if necessary
	if (!this.opened)
		this.OperateGate();
};

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
};

Gate.prototype.CloseGate = function()
{
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return;

	// If we ordered the gate to be locked, enable 'block movement' and 'block pathfinding'
	if (this.locked)
		cmpObstruction.SetDisableBlockMovementPathfinding(false, false, 0);
	// Else just enable 'block movement'
	else
		cmpObstruction.SetDisableBlockMovementPathfinding(false, true, 0);
	this.opened = false;
};

Engine.RegisterComponentType(IID_Gate, "Gate", Gate);
