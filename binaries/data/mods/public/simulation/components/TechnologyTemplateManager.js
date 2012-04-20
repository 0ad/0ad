/**
 * System component which loads the technology data files
 */
function TechnologyTemplateManager() {}

TechnologyTemplateManager.prototype.Schema =
	"<a:component type='system'/><empty/>";
	
TechnologyTemplateManager.prototype.Init = function()
{
	this.allTechs = {};
	var techNames = this.ListAllTechs();
	for (i in techNames)
		this.GetTemplate(techNames[i]);
};

TechnologyTemplateManager.prototype.GetTemplate = function(template)
{
	if (!this.allTechs[template])
	{
		this.allTechs[template] = Engine.ReadJSONFile("technologies/" + template + ".json");
		if (! this.allTechs[template])
			error("Failed to load technology \"" + template + "\"");
	}
	
	return this.allTechs[template];
};

TechnologyTemplateManager.prototype.ListAllTechs = function()
{
	return Engine.FindJSONFiles("technologies");
}

TechnologyTemplateManager.prototype.GetAllTechs = function()
{
	return this.allTechs;
}

Engine.RegisterComponentType(IID_TechnologyTemplateManager, "TechnologyTemplateManager", TechnologyTemplateManager);
