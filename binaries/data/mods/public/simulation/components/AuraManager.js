function AuraManager() {}

AuraManager.prototype.Schema =
	"<a:component type='system'/><empty/>";

AuraManager.prototype.Init = function()
{
	this.modificationsCache = {};
	this.modifications = {};
	this.templateModificationsCache = {};
	this.templateModifications = {};
};

AuraManager.prototype.ensureExists = function(name, value, id, key, defaultData)
{
	if (!this[name][value])
	{
		this[name][value] = {};
		this[name+'Cache'][value] = {};
	}

	if (!this[name][value][id])
	{
		this[name][value][id] = {};
		this[name+'Cache'][value][id] = defaultData;
	}

	if (!this[name][value][id][key])
		this[name][value][id][key] = [];
}

AuraManager.prototype.ApplyBonus = function(value, ent, data, key)
{
	this.ensureExists("modifications", value, ent, key, {"add":0, "multiply":1});

	this.modifications[value][ent][key].push(data);

	if (this.modifications[value][ent][key].length > 1)
		return;

	// first time added this aura
	if (data.multiply)
		this.modificationsCache[value][ent].multiply *= data.multiply;

	if (data.add)
		this.modificationsCache[value][ent].add += data.add;

	// post message to the entity to notify it about the change
	Engine.PostMessage(ent, MT_ValueModification, { "entities": [ent], "component": value.split("/")[0], "valueNames": [value] });
};

AuraManager.prototype.ApplyTemplateBonus = function(value, player, classes, data, key)
{
	this.ensureExists("templateModifications", value, player, key, {});

	this.templateModifications[value][player][key].push(data);

	if (this.templateModifications[value][player][key].length > 1)
		return;

	// first time added this aura
	for each (var c in classes)
	{
		if (!this.templateModificationsCache[value][player][c])
			this.templateModificationsCache[value][player][c] = [];

		if (!this.templateModificationsCache[value][player][c][key])
			this.templateModificationsCache[value][player][c][key] = { "add": 0, "multiply": 1};

		if (data.multiply)
			this.templateModificationsCache[value][player][c][key].multiply *= data.multiply;

		if (data.add)
			this.templateModificationsCache[value][player][c][key].add += data.add;

		Engine.PostMessage(SYSTEM_ENTITY, MT_TemplateModification, { "player": player, "component": value.split("/")[0], "valueNames": [value] });
	}
};

AuraManager.prototype.RemoveBonus = function(value, ent, key)
{
	if (!this.modifications[value] ||
	      !this.modifications[value][ent] ||
	      !this.modifications[value][ent][key] ||
	      !this.modifications[value][ent][key].length)
		return;

	// get the applied data to remove again
	var data = this.modifications[value][ent][key].pop();

	if (this.modifications[value][ent][key].length > 0)
		return;

	// out of last aura of this kind, remove modifications
	if (data.add)
		this.modificationsCache[value][ent].add -= data.add;

	if (data.multiply)
		this.modificationsCache[value][ent].multiply /= data.multiply;

	// post message to the entity to notify it about the change
	var effects = {};
	effects[value] = this.modificationsCache[value][ent];
	Engine.PostMessage(ent, MT_ValueModification, { "entities": [ent], "component": value.split("/")[0], "valueNames": [value] });
};

AuraManager.prototype.RemoveTemplateBonus = function(value, player, classes, key)
{
	if (!this.templateModifications[value] ||
	      !this.templateModifications[value][player] ||
	      !this.templateModifications[value][player][key] ||
	      !this.templateModifications[value][player][key].length)
		return;

	this.templateModifications[value][player][key].pop();

	if (this.templateModifications[value][player][key].length > 0)
		return;

	for each (var c in classes)
	{
		this.templateModificationsCache[value][player][c][key].multiply = 1;
		this.templateModificationsCache[value][player][c][key].add = 0;
	}
	Engine.PostMessage(SYSTEM_ENTITY, MT_TemplateModification, { "player": player, "component": value.split("/")[0], "valueNames": [value] });
};

AuraManager.prototype.ApplyModifications = function(valueName, value, ent)
{
	if (!this.modificationsCache[valueName] || !this.modificationsCache[valueName][ent])
		return value;

	value *= this.modificationsCache[valueName][ent].multiply;
	value += this.modificationsCache[valueName][ent].add;
	return value;
};

AuraManager.prototype.ApplyTemplateModifications = function(valueName, value, player, template)
{
	if (!this.templateModificationsCache[valueName] || !this.templateModificationsCache[valueName][player])
		return value;

	var classes = [];
	if (template && template.Identity)
		classes = GetIdentityClasses(template.Identity);

	var keyList = [];

	for (var c in this.templateModificationsCache[valueName][player])
	{
		if (classes.indexOf(c) == -1)
			continue;

		for (var key in this.templateModificationsCache[valueName][player][c])
		{
			// don't add an aura with the same key twice
			if (keyList.indexOf(key) != -1)
				continue;

			value *= this.templateModificationsCache[valueName][player][c][key].multiply;
			value += this.templateModificationsCache[valueName][player][c][key].add;
			keyList.push(key);
		}
	}
	return value;
};

Engine.RegisterSystemComponentType(IID_AuraManager, "AuraManager", AuraManager);
