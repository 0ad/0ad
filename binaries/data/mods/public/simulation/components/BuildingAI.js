//Number of rounds of firing per 2 seconds
const roundCount = 10;
const attackType = "Ranged";

function BuildingAI() {}

BuildingAI.prototype.Schema = 
	"<element name='DefaultArrowCount'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<element name='GarrisonArrowMultiplier'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='GarrisonArrowClasses'>" +
		"<text/>" +
	"</element>";

BuildingAI.prototype.MAX_PREFERENCE_BONUS = 2;

/**
 * Initialize BuildingAI Component
 */
BuildingAI.prototype.Init = function()
{
	if (this.GetDefaultArrowCount() > 0 || this.GetGarrisonArrowMultiplier() > 0)
	{
		this.currentRound = 0;
		//Arrows left to fire
		this.arrowsLeft = 0;
		this.targetUnits = [];
	}
};

BuildingAI.prototype.OnOwnershipChanged = function(msg)
{
	// Remove current targets, to prevent them from being added twice
	this.targetUnits = [];

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
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

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

	var range = cmpAttack.GetRange(attackType);
	this.enemyUnitsQuery = cmpRangeManager.CreateActiveParabolicQuery(this.entity, range.min, range.max, range.elevationBonus, players, IID_DamageReceiver, cmpRangeManager.GetEntityFlagMask("normal"));
	cmpRangeManager.EnableActiveQuery(this.enemyUnitsQuery);
};

// Set up a range query for Gaia units within LOS range which can be attacked.
// This should be called whenever our ownership changes.
BuildingAI.prototype.SetupGaiaRangeQuery = function()
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var owner = cmpOwnership.GetOwner();

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	var playerMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

	if (this.gaiaUnitsQuery)
	{
		cmpRangeManager.DestroyActiveQuery(this.gaiaUnitsQuery);
		this.gaiaUnitsQuery = undefined;
	}

	if (owner == -1)
		return;

	var cmpPlayer = Engine.QueryInterface(playerMan.GetPlayerByID(owner), IID_Player);
	if (!cmpPlayer.IsEnemy(0))
		return;

	var range = cmpAttack.GetRange(attackType);

	// This query is only interested in Gaia entities that can attack.
	this.gaiaUnitsQuery = cmpRangeManager.CreateActiveParabolicQuery(this.entity, range.min, range.max, range.elevationBonus, [0], IID_Attack, cmpRangeManager.GetEntityFlagMask("normal"));
	cmpRangeManager.EnableActiveQuery(this.gaiaUnitsQuery);
};

/**
 * Called when units enter or leave range
 */
BuildingAI.prototype.OnRangeUpdate = function(msg)
{

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

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
			if (cmpAttack.CanAttack(entity))
				this.targetUnits.push(entity);
		}
	}
	if (msg.removed.length > 0)
	{
		for each (var entity in msg.removed)
		{	
			var index = this.targetUnits.indexOf(entity);
			if (index > -1)
				this.targetUnits.splice(index, 1);
		}
	}

	if (!this.targetUnits.length || this.timer)
		return;

	// units entered the range, prepare to shoot
	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	var attackTimers = cmpAttack.GetTimers(attackType);
	this.timer = cmpTimer.SetInterval(this.entity, IID_BuildingAI, "FireArrows", attackTimers.prepare, attackTimers.repeat / roundCount, null);
};

BuildingAI.prototype.GetDefaultArrowCount = function()
{
	var arrowCount = +this.template.DefaultArrowCount;
	return ApplyValueModificationsToEntity("BuildingAI/DefaultArrowCount", arrowCount, this.entity);
};

BuildingAI.prototype.GetGarrisonArrowMultiplier = function()
{
	var arrowMult = +this.template.GarrisonArrowMultiplier;
	return ApplyValueModificationsToEntity("BuildingAI/GarrisonArrowMultiplier", arrowMult, this.entity);
};

BuildingAI.prototype.GetGarrisonArrowClasses = function()
{
	var string = this.template.GarrisonArrowClasses;
	if (string)
		return string.split(/\s+/);
	return [];
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
		count += Math.round(cmpGarrisonHolder.GetGarrisonedArcherCount(this.GetGarrisonArrowClasses()) * this.GetGarrisonArrowMultiplier());
	}
	return count;
};

/**
 * Fires arrows. Called 'roundCount' times every 'RepeatTime' seconds when there are units in the range
 */
BuildingAI.prototype.FireArrows = function()
{
	if (!this.targetUnits.length)
	{
		if (this.timer)
		{
			// stop the timer
			var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			cmpTimer.CancelTimer(this.timer);
			this.timer = undefined;
		}
		return;
	}

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

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
		arrowsToFire = Math.min( 
		    Math.round(2*Math.random() * this.GetArrowCount()/roundCount),  
		    this.arrowsLeft 
		);
	}
	if (arrowsToFire <= 0)
	{
		this.currentRound++;
		return;
	}
	var targets = new WeightedList();
	for (var i = 0; i < this.targetUnits.length; i++)
	{
		var target = this.targetUnits[i];
		var preference = cmpAttack.GetPreference(target);
		var weight = 1;
		if (preference !== null && preference !== undefined)
		{
			// Lower preference scores indicate a higher preference so they
			// should result in a higher weight.
			weight = 1 + this.MAX_PREFERENCE_BONUS / (1 + preference);
		}
		targets.push(target, weight);
	}
	for (var i = 0;i < arrowsToFire;i++)
	{
		var selectedIndex = targets.randomIndex();
		var selectedTarget = targets.itemAt(selectedIndex);
		if (selectedTarget && this.CheckTargetVisible(selectedTarget))
		{
			cmpAttack.PerformAttack(attackType, selectedTarget);
			PlaySound("attack", this.entity);
		}
		else 
		{
			targets.removeAt(selectedIndex);
			i--; // one extra arrow left to fire
			if(targets.length() < 1)
			{
				this.arrowsLeft += arrowsToFire;
				// no targets found in this round, save arrows and go to next round
				break;
			}
		}
	}

	this.arrowsLeft -= arrowsToFire;
	this.currentRound++;
};

/**
 * Returns true if the target entity is visible through the FoW/SoD.
 */
BuildingAI.prototype.CheckTargetVisible = function(target)
{
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
		return false;

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);

	// Entities that are hidden and miraged are considered visible
	var cmpFogging = Engine.QueryInterface(target, IID_Fogging);
	if (cmpFogging && cmpFogging.IsMiraged(cmpOwnership.GetOwner()))
		return true;

	if (cmpRangeManager.GetLosVisibility(target, cmpOwnership.GetOwner(), false) == "hidden")
		return false;

	// Either visible directly, or visible in fog
	return true;
};

Engine.RegisterComponentType(IID_BuildingAI, "BuildingAI", BuildingAI);
