function Builder() {}

Builder.prototype.Schema =
	"<a:help>Allows the unit to construct and repair buildings.</a:help>" +
	"<a:example>" +
		"<Rate>1.0</Rate>" +
		"<Entities datatype='tokens'>" +
			"\n    structures/{civ}_barracks\n    structures/{civ}_civil_centre\n    structures/pers_apadana\n  " +
		"</Entities>" +
	"</a:example>" +
	"<element name='Rate' a:help='Construction speed multiplier (1.0 is normal speed, higher values are faster).'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
	"<element name='Entities' a:help='Space-separated list of entity template names that this unit can build. The special string \"{civ}\" will be automatically replaced by the unit&apos;s four-character civ code. This element can also be empty, in which case no new foundations may be placed by the unit, but they can still repair existing buildings.'>" +
		"<attribute name='datatype'>" +
			"<value>tokens</value>" +
		"</attribute>" +
		"<text/>" +
	"</element>";

Builder.prototype.Init = function()
{
};

Builder.prototype.Serialize = null; // we have no dynamic state to save

Builder.prototype.GetEntitiesList = function()
{
	let string = this.template.Entities._string;
	if (!string)
		return [];

	let cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return [];

	let entities = string.replace(/\{civ\}/g, cmpPlayer.GetCiv()).split(/\s+/);

	let disabledTemplates = cmpPlayer.GetDisabledTemplates();

	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);

	return entities.filter(ent => !disabledTemplates[ent] && cmpTemplateManager.TemplateExists(ent));
};

Builder.prototype.GetRange = function()
{
	let max = 2;
	let cmpObstruction = Engine.QueryInterface(this.entity, IID_Obstruction);
	if (cmpObstruction)
		max += cmpObstruction.GetUnitRadius();

	return { "max": max, "min": 0 };
};

Builder.prototype.GetRate = function()
{
	return ApplyValueModificationsToEntity("Builder/Rate", +this.template.Rate, this.entity);
};

/**
 * Build/repair the target entity. This should only be called after a successful range check.
 * It should be called at a rate of once per second.
 */
Builder.prototype.PerformBuilding = function(target)
{
	let rate = this.GetRate();

	let cmpFoundation = Engine.QueryInterface(target, IID_Foundation);
	if (cmpFoundation)
	{
		cmpFoundation.Build(this.entity, rate);
		return;
	}

	let cmpRepairable = Engine.QueryInterface(target, IID_Repairable);
	if (cmpRepairable)
	{
		cmpRepairable.Repair(this.entity, rate);
		return;
	}
};

Engine.RegisterComponentType(IID_Builder, "Builder", Builder);
