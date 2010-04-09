function Cost() {}

Cost.prototype.Schema =
	"<optional>" +
		"<element name='Population'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='PopulationBonus'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='BuildTime'>" +
			"<ref name='positiveDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<element name='Resources'>" +
		"<interleave>" +
			"<element name='food'><data type='nonNegativeInteger'/></element>" +
			"<element name='wood'><data type='nonNegativeInteger'/></element>" +
			"<element name='stone'><data type='nonNegativeInteger'/></element>" +
			"<element name='metal'><data type='nonNegativeInteger'/></element>" +
		"</interleave>" +
	"</element>";

Cost.prototype.Init = function()
{
};

Cost.prototype.GetPopCost = function()
{
	if ("Population" in this.template)
		return +this.template.Population;
	return 0;
};

Cost.prototype.GetPopBonus = function()
{
	if ("PopulationBonus" in this.template)
		return +this.template.PopulationBonus;
	return 0;
};

Cost.prototype.GetBuildTime = function()
{
	return +(this.template.BuildTime || 1);
}

Cost.prototype.GetResourceCosts = function()
{
	return {
		"food": +this.template.Resources.food,
		"wood": +this.template.Resources.wood,
		"stone": +this.template.Resources.stone,
		"metal": +this.template.Resources.metal
	};
};

Engine.RegisterComponentType(IID_Cost, "Cost", Cost);
