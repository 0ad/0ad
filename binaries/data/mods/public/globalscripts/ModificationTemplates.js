/**
 * @file This provides a cache for Aura and Technology templates.
 * They may not be serialized, otherwise rejoined clients would refer
 * to different objects, triggering an Out-of-sync error.
 */
function ModificationTemplates(path)
{
	let suffix = ".json";

	this.names = deepfreeze(listFiles(path, suffix, true));

	this.templates = {};

	for (let name of this.names)
		this.templates[name] = Engine.ReadJSONFile(path + name + suffix);

	deepfreeze(this.templates);
}

ModificationTemplates.prototype.GetNames = function()
{
	return this.names;
};

ModificationTemplates.prototype.Has = function(name)
{
	return this.names.indexOf(name) != -1;
};

ModificationTemplates.prototype.Get = function(name)
{
	return this.templates[name];
};

ModificationTemplates.prototype.GetAll = function()
{
	return this.templates;
};


function LoadModificationTemplates()
{
	global.AuraTemplates = new ModificationTemplates("simulation/data/auras/");
	global.TechnologyTemplates = new ModificationTemplates("simulation/data/technologies/");
}
