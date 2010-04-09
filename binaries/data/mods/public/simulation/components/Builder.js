function Builder() {}

Builder.prototype.Schema =
	"<element name='Entities'>" +
		"<attribute name='datatype'><value>tokens</value></attribute>" +
		"<text/>" +
	"</element>" +
	"<element name='Rate'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>";

Builder.prototype.Init = function()
{
};

Builder.prototype.GetEntitiesList = function()
{
	var string = this.template.Entities._string;
	
	// Replace the "{civ}" codes with this entity's civ ID
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
		string = string.replace(/\{civ\}/g, cmpIdentity.GetCiv());
	
	return string.split(/\s+/);
};

Builder.prototype.GetRange = function()
{
	return { "max": 16, "min": 0 };
	// maybe this should depend on the unit or target or something?
}

/**
 * Build/repair the target entity. This should only be called after a successful range check.
 * It should be called at a rate of once per second.
 */
Builder.prototype.PerformBuilding = function(target)
{
	var rate = +this.template.Rate;

	// If it's a foundation, then build it
	var cmpFoundation = Engine.QueryInterface(target, IID_Foundation);
	if (cmpFoundation)
	{
		var finished = cmpFoundation.Build(this.entity, rate);
		return { "finished": finished };
	}
	else
	{
		// TODO: do some kind of repairing

		return { "finished": true };
	}
};

Engine.RegisterComponentType(IID_Builder, "Builder", Builder);
