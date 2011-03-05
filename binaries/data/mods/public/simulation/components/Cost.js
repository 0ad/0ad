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
	"<element name='Population' a:help='Population cost'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<element name='PopulationBonus' a:help='Population cap increase while this entity exists'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<element name='BuildTime' a:help='Time taken to construct/train this unit (in seconds)'>" +
		"<ref name='positiveDecimal'/>" +
	"</element>" +
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

Cost.prototype.Serialize = null; // we have no dynamic state to save

Cost.prototype.GetPopCost = function()
{
	return +this.template.Population;
};

Cost.prototype.GetPopBonus = function()
{
	return +this.template.PopulationBonus;
};

Cost.prototype.GetBuildTime = function()
{
	return +this.template.BuildTime;
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
