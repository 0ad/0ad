function BuildingAI() {}

BuildingAI.prototype.Schema = 
	"<element name='DefaultArrowCount'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<element name='GarrisonArrowMultiplier'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

/**
 * Initialize BuildingAI Component
 */
BuildingAI.prototype.Init = function()
{
	if (this.template.DefaultArrowCount > 0 || this.template.GarrisonArrowMultiplier > 0)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_BuildingAI, "FireArrows", 1000, {});
	}
};

BuildingAI.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to != -1)
		this.SetupRangeQuery(msg.to);
};

/**
 * Cleanup on destroy
 */
BuildingAI.prototype.OnDestroy = function()
{
	if (this.timer)
	{
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		this.timer = undefined;
	}

	// Clean up range queries
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.enemyUnitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.enemyUnitsQuery);
};

/**
 * Setup the Range Query to detect units coming in & out of range
 */
BuildingAI.prototype.SetupRangeQuery = function(owner)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	
	var players = [];
	
	var player = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(owner), IID_Player);

	// Get our diplomacy array
	var diplomacy = player.GetDiplomacy();
	var numPlayers = cmpPlayerManager.GetNumPlayers();
		
	for (var i = 1; i < numPlayers; ++i)
	{	// Exclude gaia, allies, and self
		// TODO: How to handle neutral players - Special query to attack military only?
		if ((i != owner) && (diplomacy[i - 1] < 0))
			players.push(i);
	}
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (cmpAttack)
	{
		var range = cmpAttack.GetRange("Ranged");
		this.enemyUnitsQuery = cmpRangeManager.CreateActiveQuery(this.entity, range.min, range.max, players, 0);
		cmpRangeManager.EnableActiveQuery(this.enemyUnitsQuery);
	}
};

/**
 * Called when units enter or leave range
 */
BuildingAI.prototype.OnRangeUpdate = function(msg)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var targetUnits = cmpRangeManager.ResetActiveQuery(this.enemyUnitsQuery);
	if (targetUnits.length > 0)
		this.targetUnit = targetUnits[0];
	else
		this.targetUnit = undefined;
};

/**
 * Returns the number of arrows which needs to be fired.
 * DefaultArrowCount + Garrisoned Archers(ie., any unit capable 
 * of shooting arrows from inside buildings)
 */
BuildingAI.prototype.GetArrowCount = function()
{
	var count = +this.template.DefaultArrowCount;
	var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
	{
		//Need to get garrisoned infantry count
		count += Math.round(cmpGarrisonHolder.GetGarrisonedArcherCount() * this.template.GarrisonArrowMultiplier);
	}
	return count;
};

/**
 * Fires arrows. Called every N times every 2 seconds
 * where N is the number of Arrows
 */
BuildingAI.prototype.FireArrows = function()
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (cmpAttack)
	{
		var timerInterval;
		var arrowCount = this.GetArrowCount();
		if (arrowCount > 0)
		{
			timerInterval = Math.round(2000 / arrowCount);
		}
		else
		{
			timerInterval = 1000;
		}
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_BuildingAI, "FireArrows", timerInterval, {});
		
		if ((this.targetUnit != undefined) && (arrowCount > 0))
		{
			cmpAttack.PerformAttack("Ranged", this.targetUnit);	
		}
		
	}
};

Engine.RegisterComponentType(IID_BuildingAI, "BuildingAI", BuildingAI);