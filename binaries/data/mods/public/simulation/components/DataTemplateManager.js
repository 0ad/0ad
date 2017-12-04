/**
 * System component which loads the technology and the aura data files
 */
function DataTemplateManager() {}

DataTemplateManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

DataTemplateManager.prototype.Init = function()
{
	this.technologiesPath = "simulation/data/technologies/";
	this.aurasPath = "simulation/data/auras/";

	this.allTechs = {};
	this.allAuras = {};

	for (let techName of this.ListAllTechs())
		this.GetTechnologyTemplate(techName);

	for (let auraName of this.ListAllAuras())
		this.GetAuraTemplate(auraName);

	deepfreeze(this.allTechs);
	deepfreeze(this.allAuras);
};

DataTemplateManager.prototype.GetTechnologyTemplate = function(template)
{
	if (!this.allTechs[template])
	{
		this.allTechs[template] = Engine.ReadJSONFile(this.technologiesPath + template + ".json");
		if (!this.allTechs[template])
			error("Failed to load technology \"" + template + "\"");
	}

	return this.allTechs[template];
};

DataTemplateManager.prototype.GetAuraTemplate = function(template)
{
	if (!this.allAuras[template])
	{
		this.allAuras[template] = Engine.ReadJSONFile(this.aurasPath + template + ".json");
		if (!this.allAuras[template])
			error("Failed to load aura \"" + template + "\"");
	}

	return this.allAuras[template];
};

DataTemplateManager.prototype.ListAllTechs = function()
{
	return Engine.ListDirectoryFiles(this.technologiesPath, "*.json", true).map(file => file.slice(this.technologiesPath.length, -".json".length));
};

DataTemplateManager.prototype.ListAllAuras = function()
{
	return Engine.ListDirectoryFiles(this.aurasPath, "*.json", true).map(file => file.slice(this.aurasPath.length, -".json".length));
};

DataTemplateManager.prototype.GetAllTechs = function()
{
	return this.allTechs;
};

DataTemplateManager.prototype.TechnologyExists = function(template)
{
	return !!this.allTechs[template];
};

Engine.RegisterSystemComponentType(IID_DataTemplateManager, "DataTemplateManager", DataTemplateManager);
