function Health() {}

Health.prototype.Schema =
	"<a:help>Deals with hitpoints and death.</a:help>" +
	"<a:example>" +
		"<Max>100</Max>" +
		"<RegenRate>1.0</RegenRate>" +
		"<IdleRegenRate>0</IdleRegenRate>" +
		"<DeathType>corpse</DeathType>" +
	"</a:example>" +
	"<element name='Max' a:help='Maximum hitpoints'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Initial' a:help='Initial hitpoints. Default if unspecified is equal to Max'>" +
			"<ref name='nonNegativeDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='DamageVariants'>" +
			"<oneOrMore>" +
				"<element a:help='Name of the variant to select when health drops under the defined ratio'>" +
					"<anyName/>" +
					"<data type='decimal'>" +
						"<param name='minInclusive'>0</param>" +
						"<param name='maxInclusive'>1</param>" +
					"</data>" +
				"</element>" +
			"</oneOrMore>" +
		"</element>" +
	"</optional>" +
	"<element name='RegenRate' a:help='Hitpoint regeneration rate per second.'>" +
		"<data type='decimal'/>" +
	"</element>" +
	"<element name='IdleRegenRate' a:help='Hitpoint regeneration rate per second when idle or garrisoned.'>" +
		"<data type='decimal'/>" +
	"</element>" +
	"<element name='DeathType' a:help='Behaviour when the unit dies'>" +
		"<choice>" +
			"<value a:help='Disappear instantly'>vanish</value>" +
			"<value a:help='Turn into a corpse'>corpse</value>" +
			"<value a:help='Remain in the world with 0 health'>remain</value>" +
		"</choice>" +
	"</element>" +
	"<optional>" +
		"<element name='SpawnEntityOnDeath' a:help='Entity template to spawn when this entity dies. Note: this is different than the corpse, which retains the original entity&apos;s appearance'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<element name='Unhealable' a:help='Indicates that the entity can not be healed by healer units'>" +
		"<data type='boolean'/>" +
	"</element>";

Health.prototype.Init = function()
{
	// Cache this value so it allows techs to maintain previous health level
	this.maxHitpoints = +this.template.Max;
	// Default to <Initial>, but use <Max> if it's undefined or zero
	// (Allowing 0 initial HP would break our death detection code)
	this.hitpoints = +(this.template.Initial || this.GetMaxHitpoints());
	this.regenRate = ApplyValueModificationsToEntity("Health/RegenRate", +this.template.RegenRate, this.entity);
	this.idleRegenRate = ApplyValueModificationsToEntity("Health/IdleRegenRate", +this.template.IdleRegenRate, this.entity);
	this.CheckRegenTimer();
	this.UpdateActor();
};

/**
 * Returns the current hitpoint value.
 * This is 0 if (and only if) the unit is dead.
 */
Health.prototype.GetHitpoints = function()
{
	return this.hitpoints;
};

Health.prototype.GetMaxHitpoints = function()
{
	return this.maxHitpoints;
};

/**
 * @return {boolean} Whether the units are injured. Dead units are not considered injured.
 */
Health.prototype.IsInjured = function()
{
	return this.hitpoints > 0 && this.hitpoints < this.GetMaxHitpoints();
};

Health.prototype.SetHitpoints = function(value)
{
	// If we're already dead, don't allow resurrection
	if (this.hitpoints == 0)
		return;

	// Before changing the value, activate Fogging if necessary to hide changes
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	let old = this.hitpoints;
	this.hitpoints = Math.max(1, Math.min(this.GetMaxHitpoints(), value));

	let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (cmpRangeManager)
		cmpRangeManager.SetEntityFlag(this.entity, "injured", this.IsInjured());

	this.RegisterHealthChanged(old);
};

Health.prototype.IsRepairable = function()
{
	let cmpRepairable = Engine.QueryInterface(this.entity, IID_Repairable);
	return cmpRepairable && cmpRepairable.IsRepairable();
};

Health.prototype.IsUnhealable = function()
{
	return this.template.Unhealable == "true" ||
		this.hitpoints <= 0 || !this.IsInjured();
};

Health.prototype.GetIdleRegenRate = function()
{
	return this.idleRegenRate;
};

Health.prototype.GetRegenRate = function()
{
	return this.regenRate;
};

Health.prototype.ExecuteRegeneration = function()
{
	let regen = this.GetRegenRate();
	if (this.GetIdleRegenRate() != 0)
	{
		let cmpUnitAI = Engine.QueryInterface(this.entity, IID_UnitAI);
		if (cmpUnitAI && cmpUnitAI.IsIdle())
			regen += this.GetIdleRegenRate();
	}

	if (regen > 0)
		this.Increase(regen);
	else
		this.Reduce(-regen);
};

/*
 * Check if the regeneration timer needs to be started or stopped
 */
Health.prototype.CheckRegenTimer = function()
{
	// check if we need a timer
	if (this.GetRegenRate() == 0 && this.GetIdleRegenRate() == 0 ||
	    !this.IsInjured() && this.GetRegenRate() >= 0 && this.GetIdleRegenRate() >= 0 ||
	    this.hitpoints == 0)
	{
		// we don't need a timer, disable if one exists
		if (this.regenTimer)
		{
			let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			cmpTimer.CancelTimer(this.regenTimer);
			this.regenTimer = undefined;
		}
		return;
	}

	// we need a timer, enable if one doesn't exist
	if (this.regenTimer)
		return;

	let cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.regenTimer = cmpTimer.SetInterval(this.entity, IID_Health, "ExecuteRegeneration", 1000, 1000, null);
};

Health.prototype.Kill = function()
{
	this.Reduce(this.hitpoints);
};

/**
 * @param {number} amount - The amount of damage to be taken.
 * @param {number} attacker - The entityID of the attacker.
 * @param {number} attackerOwner - The playerID of the owner of the attacker.
 *
 * @eturn {Object} - Object of the form { "healthChange": number }.
 */
Health.prototype.TakeDamage = function(amount, attacker, attackerOwner)
{
	if (!amount || !this.hitpoints)
		return { "healthChange": 0 };

	let change = this.Reduce(amount);

	let cmpLoot = Engine.QueryInterface(this.entity, IID_Loot);
	if (cmpLoot && cmpLoot.GetXp() > 0 && change.healthChange < 0)
		change.xp = cmpLoot.GetXp() * -change.healthChange / this.GetMaxHitpoints();

	if (!this.hitpoints)
		this.KilledBy(attacker, attackerOwner);

	return change;
};

/**
 * Called when an entity kills us.
 * @param {number} attacker - The entityID of the killer.
 * @param {number} attackerOwner - The playerID of the attacker.
 */
Health.prototype.KilledBy = function(attacker, attackerOwner)
{
	let cmpAttackerOwnership = Engine.QueryInterface(attacker, IID_Ownership);
	if (cmpAttackerOwnership)
	{
		let currentAttackerOwner = cmpAttackerOwnership.GetOwner();
		if (currentAttackerOwner != INVALID_PLAYER)
			attackerOwner = currentAttackerOwner;
	}

	// Add to killer statistics.
	let cmpKillerPlayerStatisticsTracker = QueryPlayerIDInterface(attackerOwner, IID_StatisticsTracker);
	if (cmpKillerPlayerStatisticsTracker)
		cmpKillerPlayerStatisticsTracker.KilledEntity(this.entity);

	// Add to loser statistics.
	let cmpTargetPlayerStatisticsTracker = QueryOwnerInterface(this.entity, IID_StatisticsTracker);
	if (cmpTargetPlayerStatisticsTracker)
		cmpTargetPlayerStatisticsTracker.LostEntity(this.entity);

	let cmpLooter = Engine.QueryInterface(attacker, IID_Looter);
	if (cmpLooter)
		cmpLooter.Collect(this.entity);
};

/**
 * @param {number} amount - The amount of hitpoints to substract. Kills the entity if required.
 * @return {{ healthChange:number }} -  Number of health points lost.
 */
Health.prototype.Reduce = function(amount)
{
	// If we are dead, do not do anything
	// (The entity will exist a little while after calling DestroyEntity so this
	// might get called multiple times)
	// Likewise if the amount is 0.
	if (!amount || !this.hitpoints)
		return { "healthChange": 0 };

	// Before changing the value, activate Fogging if necessary to hide changes
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	let oldHitpoints = this.hitpoints;
	// If we reached 0, then die.
	if (amount >= this.hitpoints)
	{
		this.hitpoints = 0;
		this.RegisterHealthChanged(oldHitpoints);
		this.HandleDeath();
		return { "healthChange": -oldHitpoints };
	}

	// If we are not marked as injured, do it now
	if (!this.IsInjured())
	{
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetEntityFlag(this.entity, "injured", true);
	}

	this.hitpoints -= amount;
	this.RegisterHealthChanged(oldHitpoints);
	return { "healthChange": this.hitpoints - oldHitpoints };
};

/**
 * Handle what happens when the entity dies.
 */
Health.prototype.HandleDeath = function()
{
	let cmpDeathDamage = Engine.QueryInterface(this.entity, IID_DeathDamage);
	if (cmpDeathDamage)
		cmpDeathDamage.CauseDeathDamage();
	PlaySound("death", this.entity);

	if (this.template.SpawnEntityOnDeath)
		this.CreateDeathSpawnedEntity();

	switch (this.template.DeathType)
	{
	case "corpse":
		this.CreateCorpse();
		break;

	case "remain":
		return;

	case "vanish":
		break;

	default:
		error("Invalid template.DeathType: " + this.template.DeathType);
		break;
	}

	Engine.DestroyEntity(this.entity);
};

Health.prototype.Increase = function(amount)
{
	// Before changing the value, activate Fogging if necessary to hide changes
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	if (!this.IsInjured())
		return { "old": this.hitpoints, "new": this.hitpoints };

	// If we're already dead, don't allow resurrection
	if (this.hitpoints == 0)
		return undefined;

	let old = this.hitpoints;
	this.hitpoints = Math.min(this.hitpoints + amount, this.GetMaxHitpoints());

	if (!this.IsInjured())
	{
		let cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetEntityFlag(this.entity, "injured", false);
	}

	this.RegisterHealthChanged(old);

	return { "old": old, "new": this.hitpoints };
};

Health.prototype.CreateCorpse = function()
{
	// If the unit died while not in the world, don't create any corpse for it
	// since there's nowhere for the corpse to be placed.
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition || !cmpPosition.IsInWorld())
		return;

	// Either creates a static local version of the current entity, or a
	// persistent corpse retaining the ResourceSupply element of the parent.
	let templateName = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager).GetCurrentTemplateName(this.entity);

	let entCorpse;
	let cmpResourceSupply = Engine.QueryInterface(this.entity, IID_ResourceSupply);
	let resource = cmpResourceSupply && cmpResourceSupply.GetKillBeforeGather();
	if (resource)
		entCorpse = Engine.AddEntity("resource|" + templateName);
	else
		entCorpse = Engine.AddLocalEntity("corpse|" + templateName);

	// Copy various parameters so it looks just like us.
	let cmpPositionCorpse = Engine.QueryInterface(entCorpse, IID_Position);
	let pos = cmpPosition.GetPosition();
	cmpPositionCorpse.JumpTo(pos.x, pos.z);
	let rot = cmpPosition.GetRotation();
	cmpPositionCorpse.SetYRotation(rot.y);
	cmpPositionCorpse.SetXZRotation(rot.x, rot.z);

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let cmpOwnershipCorpse = Engine.QueryInterface(entCorpse, IID_Ownership);
	if (cmpOwnership && cmpOwnershipCorpse)
		cmpOwnershipCorpse.SetOwner(cmpOwnership.GetOwner());

	let cmpVisualCorpse = Engine.QueryInterface(entCorpse, IID_Visual);
	if (cmpVisualCorpse)
	{
		let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
		if (cmpVisual)
			cmpVisualCorpse.SetActorSeed(cmpVisual.GetActorSeed());

		cmpVisualCorpse.SelectAnimation("death", true, 1);
	}

	const cmpIdentityCorpse = Engine.QueryInterface(entCorpse, IID_Identity);
	if (cmpIdentityCorpse)
	{
		const cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
		if (cmpIdentity)
		{
			const oldPhenotype = cmpIdentity.GetPhenotype();
			if (cmpIdentityCorpse.GetPhenotype() !== oldPhenotype)
			{
				cmpIdentityCorpse.SetPhenotype(oldPhenotype);
				if (cmpVisualCorpse)
					cmpVisualCorpse.RecomputeActorName();
			}
		}
	}

	if (resource)
		Engine.PostMessage(this.entity, MT_EntityRenamed, {
			"entity": this.entity,
			"newentity": entCorpse
		});
};

Health.prototype.CreateDeathSpawnedEntity = function()
{
	// If the unit died while not in the world, don't spawn a death entity for it
	// since there's nowhere for it to be placed
	let cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition.IsInWorld())
		return INVALID_ENTITY;

	// Create SpawnEntityOnDeath entity
	let spawnedEntity = Engine.AddLocalEntity(this.template.SpawnEntityOnDeath);

	// Move to same position
	let cmpSpawnedPosition = Engine.QueryInterface(spawnedEntity, IID_Position);
	let pos = cmpPosition.GetPosition();
	cmpSpawnedPosition.JumpTo(pos.x, pos.z);
	let rot = cmpPosition.GetRotation();
	cmpSpawnedPosition.SetYRotation(rot.y);
	cmpSpawnedPosition.SetXZRotation(rot.x, rot.z);

	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	let cmpSpawnedOwnership = Engine.QueryInterface(spawnedEntity, IID_Ownership);
	if (cmpOwnership && cmpSpawnedOwnership)
		cmpSpawnedOwnership.SetOwner(cmpOwnership.GetOwner());

	return spawnedEntity;
};

Health.prototype.UpdateActor = function()
{
	if (!this.template.DamageVariants)
		return;
	let ratio = this.hitpoints / this.GetMaxHitpoints();
	let newDamageVariant = "alive";
	if (ratio > 0)
	{
		let minTreshold = 1;
		for (let key in this.template.DamageVariants)
		{
			let treshold = +this.template.DamageVariants[key];
			if (treshold < ratio || treshold > minTreshold)
				continue;
			newDamageVariant = key;
			minTreshold = treshold;
		}
	}
	else
		newDamageVariant = "death";

	if (this.damageVariant && this.damageVariant == newDamageVariant)
		return;

	this.damageVariant = newDamageVariant;

	let cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	if (cmpVisual)
		cmpVisual.SetVariant("health", newDamageVariant);
};

Health.prototype.RecalculateValues = function()
{
	let oldMaxHitpoints = this.GetMaxHitpoints();
	let newMaxHitpoints = ApplyValueModificationsToEntity("Health/Max", +this.template.Max, this.entity);
	if (oldMaxHitpoints != newMaxHitpoints)
	{
		// Don't recalculate hitpoints when full health due to float imprecision: #6657.
		const newHitpoints = (this.hitpoints === oldMaxHitpoints) ? newMaxHitpoints :
			this.hitpoints * newMaxHitpoints/oldMaxHitpoints;
		this.maxHitpoints = newMaxHitpoints;
		this.SetHitpoints(newHitpoints);
	}

	let oldRegenRate = this.regenRate;
	this.regenRate = ApplyValueModificationsToEntity("Health/RegenRate", +this.template.RegenRate, this.entity);

	let oldIdleRegenRate = this.idleRegenRate;
	this.idleRegenRate = ApplyValueModificationsToEntity("Health/IdleRegenRate", +this.template.IdleRegenRate, this.entity);

	if (this.regenRate != oldRegenRate || this.idleRegenRate != oldIdleRegenRate)
		this.CheckRegenTimer();
};

Health.prototype.OnValueModification = function(msg)
{
	if (msg.component == "Health")
		this.RecalculateValues();
};

Health.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.to != INVALID_PLAYER)
		this.RecalculateValues();
};

Health.prototype.RegisterHealthChanged = function(from)
{
	this.CheckRegenTimer();
	this.UpdateActor();
	Engine.PostMessage(this.entity, MT_HealthChanged, { "from": from, "to": this.hitpoints });
};

function HealthMirage() {}
HealthMirage.prototype.Init = function(cmpHealth)
{
	this.maxHitpoints = cmpHealth.GetMaxHitpoints();
	this.hitpoints = cmpHealth.GetHitpoints();
	this.repairable = cmpHealth.IsRepairable();
	this.injured = cmpHealth.IsInjured();
	this.unhealable = cmpHealth.IsUnhealable();
};
HealthMirage.prototype.GetMaxHitpoints = function() { return this.maxHitpoints; };
HealthMirage.prototype.GetHitpoints = function() { return this.hitpoints; };
HealthMirage.prototype.IsRepairable = function() { return this.repairable; };
HealthMirage.prototype.IsInjured = function() { return this.injured; };
HealthMirage.prototype.IsUnhealable = function() { return this.unhealable; };

Engine.RegisterGlobal("HealthMirage", HealthMirage);

Health.prototype.Mirage = function()
{
	let mirage = new HealthMirage();
	mirage.Init(this);
	return mirage;
};

Engine.RegisterComponentType(IID_Health, "Health", Health);
