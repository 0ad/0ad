function Health() {}

Health.prototype.Init = function()
{
	this.hitpoints = this.GetMaxHitpoints();
};

Health.prototype.GetHitpoints = function()
{
	return this.hitpoints;
};

Health.prototype.GetMaxHitpoints = function()
{
	return +this.template.Max;
};

Health.prototype.Reduce = function(amount)
{
	if (amount >= this.hitpoints)
	{
		this.hitpoints = 0;
		// TODO: need to destroy this entity, set up a corpse, etc
	}
	else
	{
		this.hitpoints -= amount;
	}
}

Engine.RegisterComponentType(IID_Health, "Health", Health);
