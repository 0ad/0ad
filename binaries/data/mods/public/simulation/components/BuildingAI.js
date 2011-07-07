//Number of rounds of firing per 2 seconds
const roundCount = 10;
const timerInterval = 2000 / roundCount;

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
		this.currentRound = 0;
		//Arrows left to fire
		this.arrowsLeft = 0;
		this.timer = cmpTimer.SetTimeout(this.entity, IID_BuildingAI, "FireArrows", timerInterval, {});
		this.targetUnits = [];
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
	if (this.enemyUnitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.enemyUnitsQuery);
	var players = [];
	
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(owner), IID_Player);
	var numPlayers = cmpPlayerManager.GetNumPlayers();
		
	for (var i = 1; i < numPlayers; ++i)
	{	// Exclude gaia, allies, and self
		// TODO: How to handle neutral players - Special query to attack military only?
		if (cmpPlayer.IsEnemy(i))
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
	if (msg.tag != this.enemyUnitsQuery)
		return;

	if (msg.added.length > 0)
	{
		for each (var entity in msg.added)
		{
			this.targetUnits.push(entity);
		}
	}
	if (msg.removed.length > 0)
	{
		for each (var entity in msg.removed)
		{
			this.targetUnits.splice(this.targetUnits.indexOf(entity), 1);
		}
	}
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
		count += Math.round(cmpGarrisonHolder.GetGarrisonedArcherCount() * +this.template.GarrisonArrowMultiplier);
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
		var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		this.timer = cmpTimer.SetTimeout(this.entity, IID_BuildingAI, "FireArrows", timerInterval, {});
		var arrowsToFire = 0;
		if (this.currentRound > (roundCount - 1))
		{
			//Reached end of rounds. Reset count
			this.currentRound = 0;
		}
		
		if (this.currentRound == 0)
		{
			//First round. Calculate arrows to fire
			this.arrowsLeft = this.GetArrowCount();
		}
		
		if (this.currentRound == (roundCount - 1))
		{
			//Last round. Need to fire all left-over arrows
			arrowsToFire = this.arrowsLeft;
		}
		else
		{
			//Fire N arrows, 0 <= N <= Number of arrows left
			arrowsToFire = Math.floor(Math.random() * this.arrowsLeft);
		}
		if (this.targetUnits.length > 0)
		{
			for (var i = 0;i < arrowsToFire;i++)
			{
				cmpAttack.PerformAttack("Ranged", this.targetUnits[Math.floor(Math.random() * this.targetUnits.length)]);
				PlaySound("arrowfly", this.entity);
			}
			this.arrowsLeft -= arrowsToFire;
		}
		this.currentRound++;
	}
};

Engine.RegisterComponentType(IID_BuildingAI, "BuildingAI", BuildingAI);
