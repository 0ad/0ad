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
	if (this.GetDefaultArrowCount() > 0 || this.GetGarrisonArrowMultiplier() > 0)
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

	// Non-Gaia buildings should attack certain Gaia units.
	if (msg.to != 0 || this.gaiaUnitsQuery)
		this.SetupGaiaRangeQuery(msg.to);
};

BuildingAI.prototype.OnDiplomacyChanged = function(msg)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (cmpOwnership && cmpOwnership.GetOwner() == msg.player)
	{
		// Remove maybe now allied/neutral units
		this.targetUnits = [];
		this.SetupRangeQuery(msg.player);
	}
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
	if (this.gaiaUnitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.gaiaUnitsQuery);
};

/**
 * Setup the Range Query to detect units coming in & out of range
 */
BuildingAI.prototype.SetupRangeQuery = function(owner)
{
	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	if (this.enemyUnitsQuery)
	{
		cmpRangeManager.DestroyActiveQuery(this.enemyUnitsQuery);
		this.enemyUnitsQuery = undefined;
	}
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
		this.enemyUnitsQuery = cmpRangeManager.CreateActiveQuery(this.entity, range.min, range.max, players, IID_DamageReceiver, cmpRangeManager.GetEntityFlagMask("normal"));
		cmpRangeManager.EnableActiveQuery(this.enemyUnitsQuery);
	}
};

// Set up a range query for Gaia units within LOS range which can be attacked.
// This should be called whenever our ownership changes.
BuildingAI.prototype.SetupGaiaRangeQuery = function()
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var owner = cmpOwnership.GetOwner();

	var rangeMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var playerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);

	if (this.gaiaUnitsQuery)
	{
		rangeMan.DestroyActiveQuery(this.gaiaUnitsQuery);
		this.gaiaUnitsQuery = undefined;
	}

	if (owner == -1)
		return;

	var cmpPlayer = Engine.QueryInterface(playerMan.GetPlayerByID(owner), IID_Player);
	if (!cmpPlayer.IsEnemy(0))
		return;

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (cmpAttack)
	{
		var range = cmpAttack.GetRange("Ranged");

		// This query is only interested in Gaia entities that can attack.
		this.gaiaUnitsQuery = rangeMan.CreateActiveQuery(this.entity, range.min, range.max, [0], IID_Attack, rangeMan.GetEntityFlagMask("normal"));
		rangeMan.EnableActiveQuery(this.gaiaUnitsQuery);
	}
};

/**
 * Called when units enter or leave range
 */
BuildingAI.prototype.OnRangeUpdate = function(msg)
{
	if (msg.tag == this.gaiaUnitsQuery)
	{
		const filter = function(e) {
			var cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
			return (cmpUnitAI && (!cmpUnitAI.IsAnimal() || cmpUnitAI.IsDangerousAnimal()));
		};

		if (msg.added.length)
			msg.added = msg.added.filter(filter);

		// Removed entities may not have cmpUnitAI.
		for (var i = 0; i < msg.removed.length; ++i)
			if (this.targetUnits.indexOf(msg.removed[i]) == -1)
				msg.removed.splice(i--, 1);
	}
	else if (msg.tag != this.enemyUnitsQuery)
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

BuildingAI.prototype.GetDefaultArrowCount = function()
{
	var arrowCount = +this.template.DefaultArrowCount;
	var cmpTechMan = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	if (cmpTechMan)
		arrowCount = cmpTechMan.ApplyModifications("BuildingAI/DefaultArrowCount", arrowCount, this.entity);
	return arrowCount;
};

BuildingAI.prototype.GetGarrisonArrowMultiplier = function()
{
	var arrowMult = +this.template.GarrisonArrowMultiplier;
	var cmpTechMan = QueryOwnerInterface(this.entity, IID_TechnologyManager);
	if (cmpTechMan)
		arrowMult = cmpTechMan.ApplyModifications("BuildingAI/GarrisonArrowMultiplier", arrowMult, this.entity);
	return arrowMult;
};

/**
 * Returns the number of arrows which needs to be fired.
 * DefaultArrowCount + Garrisoned Archers(ie., any unit capable 
 * of shooting arrows from inside buildings)
 */
BuildingAI.prototype.GetArrowCount = function()
{
	var count = this.GetDefaultArrowCount();
	var cmpGarrisonHolder = Engine.QueryInterface(this.entity, IID_GarrisonHolder);
	if (cmpGarrisonHolder)
	{
		count += Math.round(cmpGarrisonHolder.GetGarrisonedArcherCount() * this.GetGarrisonArrowMultiplier());
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
