function ResourceGatherer() {}

ResourceGatherer.prototype.Schema =
	"<a:help>Lets the unit gather resources from entities that have the ResourceSupply component.</a:help>" +
	"<a:example>" +
		"<MaxDistance>2.0</MaxDistance>" +
		"<BaseSpeed>1.0</BaseSpeed>" +
		"<Rates>" +
			"<food.fish>1</food.fish>" +
			"<metal.ore>3</metal.ore>" +
			"<stone.rock>3</stone.rock>" +
			"<wood.tree>2</wood.tree>" +
		"</Rates>" +
	"</a:example>" +
	"<element name='MaxDistance' a:help='Max resource-gathering distance'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='BaseSpeed' a:help='Base resource-gathering rate (in resource units per second)'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='Rates' a:help='Per-resource-type gather rate multipliers. If a resource type is not specified then it cannot be gathered by this unit'>" +
		"<interleave>" +
			"<optional><element name='food' a:help='Food gather rate (may be overridden by \"food.*\" subtypes)'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='wood' a:help='Wood gather rate'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='stone' a:help='Stone gather rate'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='metal' a:help='Metal gather rate'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.fish' a:help='Fish gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.fruit' a:help='Fruit gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.grain' a:help='Grain gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.meat' a:help='Meat gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='food.milk' a:help='Milk gather rate (overrides \"food\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='wood.tree' a:help='Tree gather rate (overrides \"wood\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='stone.rock' a:help='Rock gather rate (overrides \"stone\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='metal.ore' a:help='Ore gather rate(overrides \"metal\")'><ref name='positiveDecimal'/></element></optional>" +
			"<optional><element name='metal.treasure' a:help='Treasure gather rate(overrides \"metal\")'><ref name='positiveDecimal'/></element></optional>" +
		"</interleave>" +
	"</element>";

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
	return { "max": +this.template.MaxDistance, "min": 0 };
	// maybe this should depend on the unit or target or something?
}

/**
 * Gather from the target entity. This should only be called after a successful range check,
 * and if the target has a compatible ResourceSupply.
 * It should be called at a rate of once per second.
 */
ResourceGatherer.prototype.PerformGather = function(target)
{
	var rate = this.GetTargetGatherRate(target);
	if (!rate)
		return { "exhausted": true };

	var cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	var type = cmpResourceSupply.GetType();

	var status = cmpResourceSupply.TakeResources(rate);

	// Give the gathered resources to the player
	var cmpPlayerManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_PlayerManager);
	var cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	var cmpPlayer = Engine.QueryInterface(cmpPlayerManager.GetPlayerByID(cmpOwnership.GetOwner()), IID_Player);
	cmpPlayer.AddResource(type.generic, status.amount);

	// Tell the target we're gathering from it
	Engine.PostMessage(target, MT_ResourceGather,
		{ "entity": target, "gatherer": this.entity });

	return status;
};

/**
 * Compute the amount of resources collected per second from the target.
 * Returns 0 if resources cannot be collected (e.g. the target doesn't
 * exist, or is the wrong type).
 */
ResourceGatherer.prototype.GetTargetGatherRate = function(target)
{
	var cmpResourceSupply = Engine.QueryInterface(target, IID_ResourceSupply);
	if (!cmpResourceSupply)
		return 0;

	var type = cmpResourceSupply.GetType();

	var rate;
	if (type.specific && this.template.Rates[type.generic+"."+type.specific])
		rate = this.template.Rates[type.generic+"."+type.specific];
	else
		rate = this.template.Rates[type.generic];

	return (rate || 0) * this.template.BaseSpeed;
}

Engine.RegisterComponentType(IID_ResourceGatherer, "ResourceGatherer", ResourceGatherer);
