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
	"<element name='BuildTime' a:help='Time taken to construct/train this entity (in seconds)'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Resources' a:help='Resource costs to construct/train this entity'>" +
		Resources.BuildSchema("nonNegativeInteger") +
	"</element>";

Cost.prototype.Init = function()
{
	this.populationCost = +this.template.Population;
	this.populationBonus = +this.template.PopulationBonus;
};

Cost.prototype.GetPopCost = function()
{
	return this.populationCost;
};

Cost.prototype.GetPopBonus = function()
{
	return this.populationBonus;
};

Cost.prototype.GetBuildTime = function()
{
	var cmpPlayer = QueryOwnerInterface(this.entity);
	var buildTime = (+this.template.BuildTime) * cmpPlayer.GetTimeMultiplier();
	return ApplyValueModificationsToEntity("Cost/BuildTime", buildTime, this.entity);
};

Cost.prototype.GetResourceCosts = function(owner)
{
	if (!owner)
	{
		let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
		if (!cmpOwnership)
			error("GetResourceCosts called without valid ownership");
		else
			owner = cmpOwnership.GetOwner();
	}

	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let entityTemplateName = cmpTemplateManager.GetCurrentTemplateName(this.entity);
	let entityTemplate = cmpTemplateManager.GetTemplate(entityTemplateName);

	let costs = {};
	for (let res in this.template.Resources)
		costs[res] = ApplyValueModificationsToTemplate("Cost/Resources/"+res, +this.template.Resources[res], owner, entityTemplate);

	return costs;
};

Cost.prototype.OnOwnershipChanged = function(msg)
{
	if (msg.from != INVALID_PLAYER)
	{
		let cmpPlayer = QueryPlayerIDInterface(msg.from);
		if (cmpPlayer)
			cmpPlayer.AddPopulationBonuses(-this.GetPopBonus());
	}
	if (msg.to != INVALID_PLAYER)
	{
		let cmpPlayer = QueryPlayerIDInterface(msg.to);
		if (cmpPlayer)
			cmpPlayer.AddPopulationBonuses(this.GetPopBonus());
	}
};

Cost.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Cost")
		return;

	// foundations shouldn't give a pop bonus and a pop cost
	var cmpFoundation = Engine.QueryInterface(this.entity, IID_Foundation);
	if (cmpFoundation)
		return;

	// update the population costs
	var newPopCost = Math.round(ApplyValueModificationsToEntity("Cost/Population", +this.template.Population, this.entity));
	var popCostDifference = newPopCost - this.populationCost;
	this.populationCost = newPopCost;

	// update the population bonuses
	var newPopBonus = Math.round(ApplyValueModificationsToEntity("Cost/PopulationBonus", +this.template.PopulationBonus, this.entity));
	var popDifference = newPopBonus - this.populationBonus;
	this.populationBonus = newPopBonus;

	var cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return;
	if (popCostDifference)
		cmpPlayer.AddPopulation(popCostDifference);
	if (popDifference)
		cmpPlayer.AddPopulationBonuses(popDifference);
};

Engine.RegisterComponentType(IID_Cost, "Cost", Cost);
