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
	var cacheName = name + "Cache";
	var v = this[name][value];
	if (!v)
	{
		v = {};
		this[name][value] = v;
		this[cacheName][value] = {};
	}

	var i = v[id];
	if (!i)
	{
		i = {};
		v[id] = i;
		this[cacheName][value][id] = defaultData;
	}

	var k = i[key];
	if (!k)
	{
		k = [];
		i[key] = k;
	}
	return k;
};

AuraManager.prototype.ApplyBonus = function(value, ents, data, key)
{
	for (let ent of ents)
	{
		var dataList = this.ensureExists("modifications", value, ent, key, {"add":0, "multiply":1});

		dataList.push(data);

		if (dataList.length > 1)
			continue;

		// first time added this aura
		if (data.multiply)
			this.modificationsCache[value][ent].multiply *= data.multiply;

		if (data.add)
			this.modificationsCache[value][ent].add += data.add;
		// post message to the entity to notify it about the change
		Engine.PostMessage(ent, MT_ValueModification, { "entities": [ent], "component": value.split("/")[0], "valueNames": [value] });
	}
};

AuraManager.prototype.ApplyTemplateBonus = function(value, player, classes, data, key)
{
	var dataList = this.ensureExists("templateModifications", value, player, key, {});

	dataList.push(data);

	if (dataList.length > 1)
		return;

	// first time added this aura
	let cache = this.templateModificationsCache[value][player];
	if (!cache[classes])
		cache[classes] = {};

	if (!cache[classes][key])
		cache[classes][key] = { "add": 0, "multiply": 1};

	if (data.multiply)
		cache[classes][key].multiply *= data.multiply;

	if (data.add)
		cache[classes][key].add += data.add;
	Engine.PostMessage(SYSTEM_ENTITY, MT_TemplateModification, { "player": player, "component": value.split("/")[0], "valueNames": [value] });
};

AuraManager.prototype.RemoveBonus = function(value, ents, key)
{
	for (let ent of ents)
	{
		var v = this.modifications[value];
		if (!v)
			continue;
		var e = v[ent];
		if (!e)
			continue;
		var dataList = e[key];
		if (!dataList || !dataList.length)
			continue;

		// get the applied data to remove again
		var data = dataList.pop();

		if (dataList.length > 0)
			continue;

		// out of last aura of this kind, remove modifications
		if (data.add)
			this.modificationsCache[value][ent].add -= data.add;

		if (data.multiply)
			this.modificationsCache[value][ent].multiply /= data.multiply;
		// post message to the entity to notify it about the change
		Engine.PostMessage(ent, MT_ValueModification, { "entities": [ent], "component": value.split("/")[0], "valueNames": [value] });
	}
};

AuraManager.prototype.RemoveTemplateBonus = function(value, player, classes, key)
{
	var v = this.templateModifications[value];
	if (!v)
		return;
	var p = v[player];
	if (!p)
		return;
	var dataList = p[key];
	if (!dataList || !dataList.length)
		return;

	dataList.pop();

	if (dataList.length > 0)
		return;

	this.templateModificationsCache[value][player][classes][key].multiply = 1;
	this.templateModificationsCache[value][player][classes][key].add = 0;

	Engine.PostMessage(SYSTEM_ENTITY, MT_TemplateModification, { "player": player, "component": value.split("/")[0], "valueNames": [value] });
};

AuraManager.prototype.ApplyModifications = function(valueName, value, ent)
{
	var v = this.modificationsCache[valueName];
	if (!v)
		return value;
	var cache = v[ent];
	if (!cache)
		return value;

	value *= cache.multiply;
	value += cache.add;
	return value;
};

AuraManager.prototype.ApplyTemplateModifications = function(valueName, value, player, template)
{
	var v = this.templateModificationsCache[valueName];
	if (!v)
		return value;
	var cache = v[player];
	if (!cache)
		return value;

	if (!template || !template.Identity)
		return value;
	var classes = GetIdentityClasses(template.Identity);

	var usedKeys = new Set();
	var add = 0;
	var multiply = 1;
	for (let c in cache)
	{
		if (!MatchesClassList(classes, c))
			continue;

		for (let key in cache[c])
		{
			// don't add an aura with the same key twice
			if (usedKeys.has(key))
				continue;

			multiply *= cache[c][key].multiply;
			add += cache[c][key].add;
			usedKeys.add(key);
		}
	}
	value *= multiply;
	value += add;
	return value;
};

Engine.RegisterSystemComponentType(IID_AuraManager, "AuraManager", AuraManager);
