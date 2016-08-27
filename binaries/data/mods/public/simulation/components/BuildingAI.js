//Number of rounds of firing per 2 seconds
const roundCount = 10;
const attackType = "Ranged";

function BuildingAI() {}

BuildingAI.prototype.Schema =
	"<element name='DefaultArrowCount'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<optional>" +
		"<element name='MaxArrowCount' a:help='Limit the number of arrows to a certain amount'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<element name='GarrisonArrowMultiplier'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='GarrisonArrowClasses' a:help='Add extra arrows for this class list'>" +
		"<text/>" +
	"</element>";

BuildingAI.prototype.MAX_PREFERENCE_BONUS = 2;

BuildingAI.prototype.Init = function()
{
	this.currentRound = 0;
	this.archersGarrisoned = 0;
	this.arrowsLeft = 0;
	this.targetUnits = [];
};

BuildingAI.prototype.OnGarrisonedUnitsChanged = function(msg)
{
	let classes = this.template.GarrisonArrowClasses;

	for (let ent of msg.added)
	{
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (!cmpIdentity)
			continue;
		if (MatchesClassList(cmpIdentity.GetClassesList(), classes))
			++this.archersGarrisoned;
	}

	for (let ent of msg.removed)
	{
		let cmpIdentity = Engine.QueryInterface(ent, IID_Identity);
		if (!cmpIdentity)
			continue;
		if (MatchesClassList(cmpIdentity.GetClassesList(), classes))
			--this.archersGarrisoned;
	}
};

BuildingAI.prototype.OnOwnershipChanged = function(msg)
{
	this.targetUnits = [];
	this.SetupRangeQuery();
	this.SetupGaiaRangeQuery();
};

BuildingAI.prototype.OnDiplomacyChanged = function(msg)
{
	if (!IsOwnedByPlayer(msg.player, this.entity))
		return;

	// Remove maybe now allied/neutral units
	this.targetUnits = [];
	this.SetupRangeQuery();
	this.SetupGaiaRangeQuery();
};

BuildingAI.prototype.OnDestroy = function()
{
	if (this.timer)
	{
		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		this.timer = undefined;
	}

	// Clean up range queries
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.enemyUnitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.enemyUnitsQuery);
	if (this.gaiaUnitsQuery)
		cmpRangeManager.DestroyActiveQuery(this.gaiaUnitsQuery);
};

/**
 * React on Attack value modifications, as it might influence the range
 */
BuildingAI.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Attack")
		return;

	this.targetUnits = [];
	this.SetupRangeQuery();
	this.SetupGaiaRangeQuery();
};

/**
 * Setup the Range Query to detect units coming in & out of range
 */
BuildingAI.prototype.SetupRangeQuery = function()
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.enemyUnitsQuery)
	{
		cmpRangeManager.DestroyActiveQuery(this.enemyUnitsQuery);
		this.enemyUnitsQuery = undefined;
	}

	var cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return;

	var enemies = cmpPlayer.GetEnemies();
	if (enemies.length && enemies[0] == 0)
		enemies.shift(); // remove gaia

	if (!enemies.length)
		return;

	var range = cmpAttack.GetRange(attackType);
	this.enemyUnitsQuery = cmpRangeManager.CreateActiveParabolicQuery(
			this.entity, range.min, range.max, range.elevationBonus,
			enemies, IID_DamageReceiver, cmpRangeManager.GetEntityFlagMask("normal"));

	cmpRangeManager.EnableActiveQuery(this.enemyUnitsQuery);
};

// Set up a range query for Gaia units within LOS range which can be attacked.
// This should be called whenever our ownership changes.
BuildingAI.prototype.SetupGaiaRangeQuery = function()
{
	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (this.gaiaUnitsQuery)
	{
		cmpRangeManager.DestroyActiveQuery(this.gaiaUnitsQuery);
		this.gaiaUnitsQuery = undefined;
	}

	var cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer || !cmpPlayer.IsEnemy(0))
		return;

	var range = cmpAttack.GetRange(attackType);

	// This query is only interested in Gaia entities that can attack.
	this.gaiaUnitsQuery = cmpRangeManager.CreateActiveParabolicQuery(
			this.entity, range.min, range.max, range.elevationBonus,
			[0], IID_Attack, cmpRangeManager.GetEntityFlagMask("normal"));

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

	// Target enemy units except non-dangerous animals
	if (msg.tag == this.gaiaUnitsQuery)
	{
		msg.added = msg.added.filter(e => {
			let cmpUnitAI = Engine.QueryInterface(e, IID_UnitAI);
			return cmpUnitAI && (!cmpUnitAI.IsAnimal() || cmpUnitAI.IsDangerousAnimal());
		});
	}
	else if (msg.tag != this.enemyUnitsQuery)
		return;

	// Add new targets
	for (let entity of msg.added)
		if (cmpAttack.CanAttack(entity))
			this.targetUnits.push(entity);

	// Remove targets outside of vision-range
	for (let entity of msg.removed)
	{
		let index = this.targetUnits.indexOf(entity);
		if (index > -1)
			this.targetUnits.splice(index, 1);
	}

	if (this.targetUnits.length)
		this.StartTimer();
};

BuildingAI.prototype.StartTimer = function()
{
	if (this.timer)
		return;

	var cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	var attackTimers = cmpAttack.GetTimers(attackType);

	this.timer = cmpTimer.SetInterval(this.entity, IID_BuildingAI, "FireArrows",
		attackTimers.prepare, attackTimers.repeat / roundCount, null);
};

BuildingAI.prototype.GetDefaultArrowCount = function()
{
	var arrowCount = +this.template.DefaultArrowCount;
	return ApplyValueModificationsToEntity("BuildingAI/DefaultArrowCount", arrowCount, this.entity);
};

BuildingAI.prototype.GetMaxArrowCount = function()
{
	if (!this.template.MaxArrowCount)
		return Infinity;

	let maxArrowCount = +this.template.MaxArrowCount;
	return Math.round(ApplyValueModificationsToEntity("BuildingAI/MaxArrowCount", maxArrowCount, this.entity));
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
	let count = this.GetDefaultArrowCount() +
		Math.round(this.archersGarrisoned * this.GetGarrisonArrowMultiplier());

	return Math.min(count, this.GetMaxArrowCount());
};

BuildingAI.prototype.SetUnitAITarget = function(ent)
{
	this.unitAITarget = ent;
	if (ent)
		this.StartTimer();
};

/**
 * Fire arrows with random temporal distribution on prefered targets.
 * Called 'roundCount' times every 'RepeatTime' seconds when there are units in the range.
 */
BuildingAI.prototype.FireArrows = function()
{
	if (!this.targetUnits.length && !this.unitAITarget)
	{
		if (!this.timer)
			return;

		let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
		cmpTimer.CancelTimer(this.timer);
		this.timer = undefined;
		return;
	}

	let cmpAttack = Engine.QueryInterface(this.entity, IID_Attack);
	if (!cmpAttack)
		return;

	if (this.currentRound > roundCount - 1)
		this.currentRound = 0;

	if (this.currentRound == 0)
		this.arrowsLeft = this.GetArrowCount();

	let arrowsToFire = 0;
	if (this.currentRound == roundCount - 1)
		arrowsToFire = this.arrowsLeft;
	else
		arrowsToFire = Math.min(
		    Math.round(2 * Math.random() * this.GetArrowCount() / roundCount),
		    this.arrowsLeft
		);

	if (arrowsToFire <= 0)
	{
		++this.currentRound;
		return;
	}

	// Add targets to a weighted list, to allow preferences
	let targets = new WeightedList();
	let maxPreference = this.MAX_PREFERENCE_BONUS;
	let addTarget = function(target)
	{
		let preference = cmpAttack.GetPreference(target);
		let weight = 1;

		if (preference !== null && preference !== undefined)
			weight += maxPreference / (1 + preference);

		targets.push(target, weight);
	};

	// Add the UnitAI target separately, as the UnitMotion and RangeManager implementations differ
	if (this.unitAITarget && this.targetUnits.indexOf(this.unitAITarget) == -1)
		addTarget(this.unitAITarget);
	for (let target of this.targetUnits)
		addTarget(target);

	for (let i = 0; i < arrowsToFire; ++i)
	{
		let selectedIndex = targets.randomIndex();
		let selectedTarget = targets.itemAt(selectedIndex);

		if (selectedTarget && this.CheckTargetVisible(selectedTarget))
		{
			cmpAttack.PerformAttack(attackType, selectedTarget);
			PlaySound("attack", this.entity);
			continue;
		}

		// Could not attack target, retry
		targets.removeAt(selectedIndex);
		--i;

		if (!targets.length())
		{
			this.arrowsLeft += arrowsToFire;
			break;
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

	// Entities that are hidden and miraged are considered visible
	var cmpFogging = Engine.QueryInterface(target, IID_Fogging);
	if (cmpFogging && cmpFogging.IsMiraged(cmpOwnership.GetOwner()))
		return true;

	// Either visible directly, or visible in fog
	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	return cmpRangeManager.GetLosVisibility(target, cmpOwnership.GetOwner()) != "hidden";
};

Engine.RegisterComponentType(IID_BuildingAI, "BuildingAI", BuildingAI);
