/**
 * System component which loads the technology data files
 */
function TechnologyTemplateManager() {}

TechnologyTemplateManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

TechnologyTemplateManager.prototype.Init = function()
{
	this.allTechs = {};
	this.allAuras = {};
	var techNames = this.ListAllTechs();
	for (var i in techNames)
		this.GetTemplate(techNames[i]);
};

TechnologyTemplateManager.prototype.GetTemplate = function(template)
{
	if (!this.allTechs[template])
	{
		this.allTechs[template] = Engine.ReadJSONFile("technologies/" + template + ".json");
		if (!this.allTechs[template])
			error("Failed to load technology \"" + template + "\"");
	}

	return this.allTechs[template];
};

TechnologyTemplateManager.prototype.GetAuraTemplate = function(template)
{
	if (!this.allAuras[template])
	{
		this.allAuras[template] = Engine.ReadJSONFile("auras/" + template + ".json");
		if (!this.allAuras[template])
			error("Failed to load aura \"" + template + "\"");
	}

	return this.allAuras[template];
};

TechnologyTemplateManager.prototype.ListAllTechs = function()
{
	return Engine.FindJSONFiles("technologies", true);
};

TechnologyTemplateManager.prototype.GetAllTechs = function()
{
	return this.allTechs;
};

Engine.RegisterSystemComponentType(IID_TechnologyTemplateManager, "TechnologyTemplateManager", TechnologyTemplateManager);
