function ResourceGatherer() {}

ResourceGatherer.prototype.Init = function()
{
};

ResourceGatherer.prototype.GetGatherRates = function()
{
	var ret = {};
	for (var r in this.template.Rates)
		ret[r] = this.template.Rates[r] * this.template.BaseSpeed;
	return ret;
};

ResourceGatherer.prototype.GetRange = function()
{
	return { "max": 4, "min": 0 };
	// maybe this should depend on the unit or target or something?
}

/**
 * Gather from the target entity. This should only be called after a successful range check,
 * and if the target has a compatible ResourceSupply.
 * It should be called at a rate of once per second.
 */
ResourceGatherer.prototype.PerformGather = function(target)
{
	var cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	var type = cmpResourceSupply.GetType();

	var rate;
	if (type.specific && this.template.Rates[type.generic+"."+type.specific])
		rate = this.template.Rates[type.generic+"."+type.specific] * this.template.BaseSpeed;
	else
		rate = this.template.Rates[type.generic] * this.template.BaseSpeed;

	var status = cmpResourceSupply.TakeResources(rate);

	// Give the gathered resources to the player
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(cmpOwnership.GetOwner()), IID_Player);
	cmpPlayer.AddResource(type.generic, status.amount);
	
	return status;
};


Engine.RegisterComponentType(IID_ResourceGatherer, "ResourceGatherer", ResourceGatherer);
