function Health() {}

Health.prototype.Schema =
	"<a:help>Deals with hitpoints and death.</a:help>" +
	"<a:example>" +
		"<Max>100</Max>" +
		"<RegenRate>1.0</RegenRate>" +
		"<DeathType>corpse</DeathType>" +
	"</a:example>" +
	"<element name='Max' a:help='Maximum hitpoints'>" +
		"<data type='positiveInteger'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Initial' a:help='Initial hitpoints. Default if unspecified is equal to Max'>" +
			"<data type='positiveInteger'/>" +
		"</element>" +
	"</optional>" +
	"<element name='RegenRate' a:help='Hitpoint regeneration rate per second. Not yet implemented'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='DeathType' a:help='Behaviour when the unit dies'>" +
		"<choice>" +
			"<value a:help='Disappear instantly'>vanish</value>" +
			"<value a:help='Turn into a corpse'>corpse</value>" +
			"<value a:help='Remain in the world with 0 health'>remain</value>" +
		"</choice>" +
	"</element>" +
	"<element name='Healable' a:help='Indicates that the entity can be healed by healer units'>" +
		"<data type='boolean'/>" +
	"</element>" +
	"<element name='Repairable' a:help='Indicates that the entity can be repaired by builder units'>" +
		"<data type='boolean'/>" +
	"</element>";

Health.prototype.Init = function()
{
	// Default to <Initial>, but use <Max> if it's undefined or zero
	// (Allowing 0 initial HP would break our death detection code)
	this.hitpoints = +(this.template.Initial || this.GetMaxHitpoints());
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
	return +this.template.Max;
};

Health.prototype.SetHitpoints = function(value)
{
	// If we're already dead, don't allow resurrection
	if (this.hitpoints == 0)
		return;

	var old = this.hitpoints;
	this.hitpoints = Math.max(1, Math.min(this.GetMaxHitpoints(), value));

	Engine.PostMessage(this.entity, MT_HealthChanged, { "from": old, "to": this.hitpoints });
};

Health.prototype.IsRepairable = function()
{
	return (this.template.Repairable == "true");
};

Health.prototype.Kill = function()
{
	this.Reduce(this.hitpoints);
};

Health.prototype.Reduce = function(amount)
{
	var state = { "killed": false };
	if (amount >= this.hitpoints)
	{
		// If this is the first time we reached 0, then die.
		// (The entity will exist a little while after calling DestroyEntity so this
		// might get called multiple times)
		if (this.hitpoints)
		{
			state.killed = true;
			
			PlaySound("death", this.entity);

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
				// Don't destroy the entity

				// Make it fall over
				var cmpVisual = Engine.QueryInterface(this.entity, IID_Visual);
				if (cmpVisual)
					cmpVisual.SelectAnimation("death", true, 1.0, "");
			}

			var old = this.hitpoints;
			this.hitpoints = 0;
			
			Engine.PostMessage(this.entity, MT_HealthChanged, { "from": old, "to": this.hitpoints });
		}

	}
	else
	{
		var old = this.hitpoints;
		this.hitpoints -= amount;

		Engine.PostMessage(this.entity, MT_HealthChanged, { "from": old, "to": this.hitpoints });
	}
	return state;
};

Health.prototype.Increase = function(amount)
{
	// If we're already dead, don't allow resurrection
	if (this.hitpoints == 0)
		return;

	var old = this.hitpoints;
	this.hitpoints = Math.min(this.hitpoints + amount, this.GetMaxHitpoints());

	Engine.PostMessage(this.entity, MT_HealthChanged, { "from": old, "to": this.hitpoints });
};

//// Private functions ////

Health.prototype.CreateCorpse = function()
{
	// Create a static local version of the current entity
	var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var templateName = cmpTempMan.GetCurrentTemplateName(this.entity);
	var corpse = Engine.AddLocalEntity("corpse|" + templateName);

	// Copy various parameters so it looks just like us

	var cmpPosition = Engine.QueryInterface(this.entity, IID_Position);
	var cmpCorpsePosition = Engine.QueryInterface(corpse, IID_Position);
	var pos = cmpPosition.GetPosition();
	cmpCorpsePosition.JumpTo(pos.x, pos.z);
	var rot = cmpPosition.GetRotation();
	cmpCorpsePosition.SetYRotation(rot.y);
	cmpCorpsePosition.SetXZRotation(rot.x, rot.z);

	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpCorpseOwnership = Engine.QueryInterface(corpse, IID_Ownership);
	cmpCorpseOwnership.SetOwner(cmpOwnership.GetOwner());

	// Make it fall over
	var cmpCorpseVisual = Engine.QueryInterface(corpse, IID_Visual);
	cmpCorpseVisual.SelectAnimation("death", true, 1.0, "");
};

Health.prototype.Repair = function(builderEnt, work)
{
	var damage = this.GetMaxHitpoints() - this.GetHitpoints();

	// Do nothing if we're already at full hitpoints
	if (damage <= 0)
		return;

	// Calculate the amount of hitpoints that will be added
	// TODO: what computation should we use?
	// TODO: should we do some diminishing returns thing? (see Foundation.Build)
	var amount = Math.min(damage, work);

	// TODO: resource costs?

	// Add hitpoints
	this.Increase(amount);

	// If we repaired all the damage, send a message to entities
	// to stop repairing this building
	if (amount >= damage)
	{
		Engine.PostMessage(this.entity, MT_ConstructionFinished,
			{ "entity": this.entity, "newentity": this.entity });
	}
};

Engine.RegisterComponentType(IID_Health, "Health", Health);
