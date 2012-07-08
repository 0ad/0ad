function Gate() {}

Gate.prototype.Schema =
	"<element name='Radius'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

/**
 * Initialize Gate Component
 */
Gate.prototype.Init = function()
{
		this.allyUnits = [];
};

Gate.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to != -1)
	{
		this.SetupRangeQuery(msg.to);
		this.UnlockGate();
		this.CloseGate();
	}
};

/**
 * Cleanup on destroy
 */
Gate.prototype.OnDestroy = function()
{
	// Clean up range query
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.allyUnitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.allyUnitsQuery);
};

/**
 * Setup the Range Query to detect units coming in & out of range
 */
Gate.prototype.SetupRangeQuery = function(owner)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (this.allyUnitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.allyUnitsQuery);
	var allyPlayers = []
	
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(owner), IID_Player);
	var numPlayers = cmpPlayerManager.GetNumPlayers();
		
	for (var i = 1; i < numPlayers; ++i)
	{	// Exclude gaia, allies, and self
		// TODO: How to handle neutral players - Special query to attack military only?
		if (cmpPlayer.IsAlly(i))
			allyPlayers.push(i);
	}
	
	if (this.GetRadius() > 0)
	{
		var range = this.GetRadius();
		this.allyUnitsQuery = cmpRangeManager.CreateActiveQuery(this.entity, 0, range, allyPlayers, 0, cmpRangeManager.GetEntityFlagMask("normal"));
		cmpRangeManager.EnableActiveQuery(this.allyUnitsQuery);
	}
};

/**
 * Called when units enter or leave range
 */
Gate.prototype.OnRangeUpdate = function(msg)
{
	if (msg.tag != this.allyUnitsQuery)
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
				this.allyUnits.push(entity);
			}
		}
	}
	if (msg.removed.length > 0)
	{
		for each (var entity in msg.removed)
		{
			this.allyUnits.splice(this.allyUnits.indexOf(entity), 1);
		}
	}

	this.ManeuverGate();
};

/**
 * Get the range query radius
 */
Gate.prototype.GetRadius = function()
{
	return +this.template.Radius;
};

/**
 * Open or close the gate
 */
Gate.prototype.ManeuverGate = function()
{
	if (this.opened == true )
	{
		if (this.allyUnits.length == 0)
		{
			this.CloseGate();
		}
	}
	else
	{
		if (this.allyUnits.length > 0)
		{
			this.OpenGate();
		}
	}
};

Gate.prototype.IsLocked = function()
{
	return this.locked;
};

Gate.prototype.LockGate = function()
{
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return;
	cmpObstruction.SetDisableBlockMovementPathfinding(false, false, 0);
	this.locked = true;
	this.opened = false;
};

Gate.prototype.UnlockGate = function()
{
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return;
	cmpObstruction.SetDisableBlockMovementPathfinding(false, true, 0);
	this.locked = false;
	this.opened = false;
	if (this.allyUnits.length > 0)
		this.OpenGate();
};

Gate.prototype.OpenGate = function()
{
	if (this.locked)
		return;
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return;
	cmpObstruction.SetDisableBlockMovementPathfinding(true, true, 0);
	this.opened = true;
};

Gate.prototype.CloseGate = function()
{
	if (this.locked)
		return;
	var cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (!cmpObstruction)
		return;
	cmpObstruction.SetDisableBlockMovementPathfinding(false, true, 0);
	this.opened = false;
};

Engine.RegisterComponentType(IID_Gate, "Gate", Gate);
