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
	"<element name='Undeletable' a:help='Prevent players from deleting this entity.'>" +
		"<data type='boolean'/>" +
	"</element>" +
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
	this.undeletable = this.template.Undeletable == "true";
	this.CheckRegenTimer();
	this.UpdateActor();
};

//// Interface functions ////

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

Health.prototype.SetHitpoints = function(value)
{
	// If we're already dead, don't allow resurrection
	if (this.hitpoints == 0)
		return;

	// Before changing the value, activate Fogging if necessary to hide changes
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	var old = this.hitpoints;
	this.hitpoints = Math.max(1, Math.min(this.GetMaxHitpoints(), value));

	var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
	if (cmpRangeManager)
	{
		if (this.hitpoints < this.GetMaxHitpoints())
			cmpRangeManager.SetEntityFlag(this.entity, "injured", true);
		else
			cmpRangeManager.SetEntityFlag(this.entity, "injured", false);
	}

	this.RegisterHealthChanged(old);
};

Health.prototype.IsRepairable = function()
{
	return Engine.QueryInterface(this.entity, IID_Repairable) != null;
};

Health.prototype.IsUnhealable = function()
{
	return (this.template.Unhealable == "true"
		|| this.GetHitpoints() <= 0
		|| this.GetHitpoints() >= this.GetMaxHitpoints());
};

Health.prototype.IsUndeletable = function()
{
	return this.undeletable;
};

Health.prototype.SetUndeletable = function(undeletable)
{
	this.undeletable = undeletable;
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
		if (cmpUnitAI && (cmpUnitAI.IsIdle() || cmpUnitAI.IsGarrisoned() && !cmpUnitAI.IsTurret()))
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
	    this.GetHitpoints() == this.GetMaxHitpoints() && this.GetRegenRate() >= 0 && this.GetIdleRegenRate() >= 0 ||
	    this.GetHitpoints() == 0)
	{
		// we don't need a timer, disable if one exists
		if (this.regenTimer)
		{
			var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
			cmpTimer.CancelTimer(this.regenTimer);
			this.regenTimer = undefined;
		}
		return;
	}

	// we need a timer, enable if one doesn't exist
	if (this.regenTimer)
		return;

	var cmpTimer = Engine.QueryInterface(SYSTEM_ENTITY, IID_Timer);
	this.regenTimer = cmpTimer.SetInterval(this.entity, IID_Health, "ExecuteRegeneration", 1000, 1000, null);
};

Health.prototype.Kill = function()
{
	this.Reduce(this.hitpoints);
};

/**
 * Reduces entity's health by amount HP.
 * Returns object of the form { "killed": false, "change": -12 }
 */
Health.prototype.Reduce = function(amount)
{
	// Before changing the value, activate Fogging if necessary to hide changes
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	var state = { "killed": false };
	if (amount >= 0 && this.hitpoints == this.GetMaxHitpoints())
	{
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetEntityFlag(this.entity, "injured", true);
	}
	var oldHitpoints = this.hitpoints;
	if (amount >= this.hitpoints)
	{
		// If this is the first time we reached 0, then die.
		// (The entity will exist a little while after calling DestroyEntity so this
		// might get called multiple times)
		if (this.hitpoints)
		{
			state.killed = true;

			PlaySound("death", this.entity);

			// If SpawnEntityOnDeath is set, spawn the entity
			if(this.template.SpawnEntityOnDeath)
				this.CreateDeathSpawnedEntity();

			if (this.template.DeathType == "corpse")
			{
				this.CreateCorpse();
				Engine.DestroyEntity(this.entity);
			}
			else if (this.template.DeathType == "vanish")
			{
				Engine.DestroyEntity(this.entity);
			}
			else if (this.template.DeathType == "remain")
			{
				var resource = this.CreateCorpse(true);
				if (resource != INVALID_ENTITY)
					Engine.BroadcastMessage(MT_EntityRenamed, { entity: this.entity, newentity: resource });
				Engine.DestroyEntity(this.entity);
			}

			this.hitpoints = 0;
			this.RegisterHealthChanged(oldHitpoints);
		}

	}
	else
	{
		this.hitpoints -= amount;
		this.RegisterHealthChanged(oldHitpoints);
	}
	state.change = this.hitpoints - oldHitpoints;
	return state;
};

Health.prototype.Increase = function(amount)
{
	// Before changing the value, activate Fogging if necessary to hide changes
	let cmpFogging = Engine.QueryInterface(this.entity, IID_Fogging);
	if (cmpFogging)
		cmpFogging.Activate();

	if (this.hitpoints == this.GetMaxHitpoints())
		return {"old": this.hitpoints, "new":this.hitpoints};

	// If we're already dead, don't allow resurrection
	if (this.hitpoints == 0)
		return undefined;

	var old = this.hitpoints;
	this.hitpoints = Math.min(this.hitpoints + amount, this.GetMaxHitpoints());

	if (this.hitpoints == this.GetMaxHitpoints())
	{
		var cmpRangeManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_RangeManager);
		if (cmpRangeManager)
			cmpRangeManager.SetEntityFlag(this.entity, "injured", false);
	}

	this.RegisterHealthChanged(old);

	return { "old": old, "new": this.hitpoints};
};

//// Private functions ////

Health.prototype.CreateCorpse = function(leaveResources)
{
	// If the unit died while not in the world, don't create any corpse for it
	// since there's nowhere for the corpse to be placed
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition.IsInWorld())
		return INVALID_ENTITY;

	// Either creates a static local version of the current entity, or a
	// persistent corpse retaining the ResourceSupply element of the parent.
	var cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var templateName = cmpTemplateManager.GetCurrentTemplateName(this.entity);
	var corpse;
	if (leaveResources)
		corpse = Engine.AddEntity("resource|" + templateName);
	else
		corpse = Engine.AddLocalEntity("corpse|" + templateName);

	// Copy various parameters so it looks just like us

	var cmpCorpsePosition = Engine.QueryInterface(corpse, IID_Position);
	var pos = cmpPosition.GetPosition();
	cmpCorpsePosition.JumpTo(pos.x, pos.z);
	var rot = cmpPosition.GetRotation();
	cmpCorpsePosition.SetYRotation(rot.y);
	cmpCorpsePosition.SetXZRotation(rot.x, rot.z);

	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpCorpseOwnership = Engine.QueryInterface(corpse, IID_Ownership);
	cmpCorpseOwnership.SetOwner(cmpOwnership.GetOwner());

	var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
	var cmpCorpseVisual = Engine.QueryInterface(corpse, IID_Visual);
	cmpCorpseVisual.SetActorSeed(cmpVisual.GetActorSeed());

	// Make it fall over
	cmpCorpseVisual.SelectAnimation("death", true, 1.0, "");

	return corpse;
};

Health.prototype.CreateDeathSpawnedEntity = function()
{
	// If the unit died while not in the world, don't spawn a death entity for it
	// since there's nowhere for it to be placed
	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	if (!cmpPosition.IsInWorld())
		return INVALID_ENTITY;

	// Create SpawnEntityOnDeath entity
	var spawnedEntity = Engine.AddLocalEntity(this.template.SpawnEntityOnDeath);

	// Move to same position
	var cmpSpawnedPosition = Engine.QueryInterface(spawnedEntity, IID_Position);
	var pos = cmpPosition.GetPosition();
	cmpSpawnedPosition.JumpTo(pos.x, pos.z);
	var rot = cmpPosition.GetRotation();
	cmpSpawnedPosition.SetYRotation(rot.y);
	cmpSpawnedPosition.SetXZRotation(rot.x, rot.z);

	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpSpawnedOwnership = Engine.QueryInterface(spawnedEntity, IID_Ownership);
	if (cmpOwnership && cmpSpawnedOwnership)
		cmpSpawnedOwnership.SetOwner(cmpOwnership.GetOwner());

	return spawnedEntity;
};

Health.prototype.UpdateActor = function()
{
	if (!this.template.DamageVariants)
		return;
	let ratio = this.GetHitpoints() / this.GetMaxHitpoints();
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

Health.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Health")
		return;

	let oldMaxHitpoints = this.GetMaxHitpoints();
	let newMaxHitpoints = ApplyValueModificationsToEntity("Health/Max", +this.template.Max, this.entity);
	if (oldMaxHitpoints != newMaxHitpoints)
	{
		let newHitpoints = this.GetHitpoints() * newMaxHitpoints/oldMaxHitpoints;
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

Health.prototype.RegisterHealthChanged = function(from)
{
	this.CheckRegenTimer();
	this.UpdateActor();
	Engine.PostMessage(this.entity, MT_HealthChanged, { "from": from, "to": this.hitpoints });
};

Engine.RegisterComponentType(IID_Health, "Health", Health);
