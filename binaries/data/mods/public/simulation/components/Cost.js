function Cost() {}

Cost.prototype.Schema =
	"<a:help>Specifies the construction/training costs of this entity.</a:help>" +
	"<a:example>" +
		"<Population>1</Population>" +
		"<PopulationBonus>15</PopulationBonus>" +
		"<BuildTime>20.0</BuildTime>" +
		"<Resources>" +
			"<food>50</food>" +
			"<wood>0</wood>" +
			"<stone>0</stone>" +
			"<metal>25</metal>" +
		"</Resources>" +
	"</a:example>" +
	"<optional>" +
		"<element name='Population' a:help='Population cost'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='PopulationBonus' a:help='Population cap increase while this entity exists'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='BuildTime' a:help='Time taken to construct/train this unit (in seconds)'>" +
			"<ref name='positiveDecimal'/>" +
		"</element>" +
	"</optional>" +
	"<element name='Resources' a:help='Resource costs to construct/train this unit'>" +
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
