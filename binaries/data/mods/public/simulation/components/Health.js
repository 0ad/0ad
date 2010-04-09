function Health() {}

Health.prototype.Schema =
	"<element name='Max'>" +
		"<data type='positiveInteger'/>" +
	"</element>" +
	"<optional>" +
		"<element name='Initial'>" +
			"<data type='positiveInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='RegenRate'>" +
			"<ref name='positiveDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='DeathType'>" +
			"<value>corpse</value>" +
		"</element>" +
	"</optional>";

Health.prototype.Init = function()
{
	// Default to <Initial>, but use <Max> if it's undefined or zero
	// (Allowing 0 initial HP would break our death detection code)
	this.hitpoints = +(this.template.Initial || this.GetMaxHitpoints());
};

//// Interface functions ////

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

	this.hitpoints = Math.max(1, Math.min(this.GetMaxHitpoints(), value));
}

Health.prototype.Reduce = function(amount)
{
	if (amount >= this.hitpoints)
	{
		// If this is the first time we reached 0, then die.
		// (The entity will exist a little while after calling DestroyEntity so this
		// might get called multiple times)
		if (this.hitpoints)
		{
			PlaySound("death", this.entity);

			if (this.template.DeathType == "corpse")
				this.CreateCorpse();

			Engine.DestroyEntity(this.entity);
		}

		this.hitpoints = 0;
	}
	else
	{
		this.hitpoints -= amount;
	}
}

Health.prototype.Increase = function(amount)
{
	// If we're already dead, don't allow resurrection
	if (this.hitpoints == 0)
		return;

	this.hitpoints = Math.min(this.hitpoints + amount, this.GetMaxHitpoints());
}

//// Private functions ////

Health.prototype.CreateCorpse = function()
{
	// Create a static local version of the current entity
	var cmpTempMan = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	var templateName = cmpTempMan.GetCurrentTemplateName(this.entity);
	var corpse = Engine.AddLocalEntity("preview|" + templateName);
	// (Maybe this should be some kind of "corpse|" instead of "preview|", if we want
	// to add things like corpse-removal timers and change the terrain conformance mode)

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
	cmpCorpseVisual.SelectAnimation("death", true);
};


Engine.RegisterComponentType(IID_Health, "Health", Health);
