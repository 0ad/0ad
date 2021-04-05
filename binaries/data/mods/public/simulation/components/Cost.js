function Cost() {}

Cost.prototype.Schema =
	"<a:help>Specifies the construction/training costs of this entity.</a:help>" +
	"<a:example>" +
		"<Population>1</Population>" +
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
	"<element name='BuildTime' a:help='Time taken to construct/train this entity (in seconds)'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>" +
	"<element name='Resources' a:help='Resource costs to construct/train this entity'>" +
		Resources.BuildSchema("nonNegativeInteger") +
	"</element>";

Cost.prototype.Init = function()
{
	this.populationCost = +this.template.Population;
};

Cost.prototype.GetPopCost = function()
{
	return this.populationCost;
};


Cost.prototype.GetBuildTime = function()
{
	return ApplyValueModificationsToEntity("Cost/BuildTime", +this.template.BuildTime, this.entity);
};

Cost.prototype.GetResourceCosts = function()
{
	let cmpOwnership = Engine.QueryInterface(this.entity, IID_Ownership);
	if (!cmpOwnership)
	{
		error("GetResourceCosts called without valid ownership on " + this.entity + ".");
		return {};
	}

	let cmpTemplateManager = Engine.QueryInterface(SYSTEM_ENTITY, IID_TemplateManager);
	let entityTemplateName = cmpTemplateManager.GetCurrentTemplateName(this.entity);
	let entityTemplate = cmpTemplateManager.GetTemplate(entityTemplateName);

	let owner = cmpOwnership.GetOwner();
	let costs = {};
	for (let res in this.template.Resources)
		costs[res] = ApplyValueModificationsToTemplate("Cost/Resources/"+res, +this.template.Resources[res], owner, entityTemplate);

	return costs;
};


Cost.prototype.OnValueModification = function(msg)
{
	if (msg.component != "Cost")
		return;

	// Foundations shouldn't have a pop cost.
	var cmpFoundation = Engine.QueryInterface(this.entity, IID_Foundation);
	if (cmpFoundation)
		return;

	// update the population costs
	var newPopCost = Math.round(ApplyValueModificationsToEntity("Cost/Population", +this.template.Population, this.entity));
	var popCostDifference = newPopCost - this.populationCost;
	this.populationCost = newPopCost;

	var cmpPlayer = QueryOwnerInterface(this.entity);
	if (!cmpPlayer)
		return;
	if (popCostDifference)
		cmpPlayer.AddPopulation(popCostDifference);
};

Engine.RegisterComponentType(IID_Cost, "Cost", Cost);
