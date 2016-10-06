/**
 * System component which loads the technology and the aura data files
 */
function DataTemplateManager() {}

DataTemplateManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

DataTemplateManager.prototype.Init = function()
{
	this.allTechs = {};
	this.allAuras = {};

	for (let techName of this.ListAllTechs())
		this.GetTechnologyTemplate(techName);

	for (let auraName of this.ListAllAuras())
		this.GetAuraTemplate(auraName);
};

DataTemplateManager.prototype.GetTechnologyTemplate = function(template)
{
	if (!this.allTechs[template])
	{
		this.allTechs[template] = Engine.ReadJSONFile("technologies/" + template + ".json");
		if (!this.allTechs[template])
			error("Failed to load technology \"" + template + "\"");
	}

	return this.allTechs[template];
};

DataTemplateManager.prototype.GetAuraTemplate = function(template)
{
	if (!this.allAuras[template])
	{
		this.allAuras[template] = Engine.ReadJSONFile("auras/" + template + ".json");
		if (!this.allAuras[template])
			error("Failed to load aura \"" + template + "\"");
	}

	return this.allAuras[template];
};

DataTemplateManager.prototype.ListAllTechs = function()
{
	return Engine.FindJSONFiles("technologies", true);
};

DataTemplateManager.prototype.ListAllAuras = function()
{
	return Engine.FindJSONFiles("auras", true);
};

DataTemplateManager.prototype.GetAllTechs = function()
{
	return this.allTechs;
};

Engine.RegisterSystemComponentType(IID_DataTemplateManager, "DataTemplateManager", DataTemplateManager);
