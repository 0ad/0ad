function Builder() {}

Builder.prototype.Schema =
	"<a:help>Allows the unit to construct and repair buildings.</a:help>" +
	"<a:example>" +
		"<Rate>1.0</Rate>" +
		"<Entities>" +
			"\n    structures/{civ}_barracks\n    structures/{civ}_civil_centre\n    structures/celt_sb1\n  " +
		"</Entities>" +
	"</a:example>" +
	"<element name='Rate' a:help='Construction speed multiplier (1.0 is normal speed, higher values are faster)'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='Entities' a:help='Space-separated list of entity template names that this unit can build. The special string \"{civ}\" will be automatically replaced by the unit&apos;s four-character civ code'>" +
		"<text/>" +
	"</element>";

Builder.prototype.Init = function()
{
};

Builder.prototype.GetEntitiesList = function()
{
	var string = this.template.Entities;
	
	// Replace the "{civ}" codes with this entity's civ ID
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
		string = string.replace(/\{civ\}/g, cmpIdentity.GetCiv());
	
	return string.split(/\s+/);
};

Builder.prototype.GetRange = function()
{
	return { "max": 2, "min": 0 };
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
