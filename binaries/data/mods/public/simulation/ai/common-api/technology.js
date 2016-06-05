var API3 = function(m)
{

// Wrapper around a technology template

m.Technology = function(allTemplates, templateName)
{
	this._templateName = templateName;
	var template = allTemplates[templateName];

	// check if this is one of two paired technologies.
	this._isPair = template.pair !== undefined;
	if (this._isPair)
	{
		if (allTemplates[template.pair].top == templateName)
			this._pairedWith = allTemplates[template.pair].bottom;
		else
			this._pairedWith = allTemplates[template.pair].top;
	}
	// check if it only defines a pair:
	this._definesPair = template.top !== undefined;
	this._template = template;
	this._techTemplates = allTemplates;
};

// returns generic, or specific if civ provided.
m.Technology.prototype.name = function(civ)
{
	if (civ === undefined)
		return this._template.genericName;

	if (this._template.specificName === undefined || this._template.specificName[civ] === undefined)
		return undefined;
	return this._template.specificName[civ];
};

m.Technology.prototype.pairDef = function()
{
	return this._definesPair;
};

// in case this defines a pair only, returns the two paired technologies.
m.Technology.prototype.getPairedTechs = function()
{
	if (!this._definesPair)
		return undefined;

	var techOne = new m.Technology(this._techTemplates, this._template.top);
	var techTwo = new m.Technology(this._techTemplates, this._template.bottom);

	return [techOne,techTwo];
};

m.Technology.prototype.pair = function()
{
	if (!this._isPair)
		return undefined;
	return this._template.pair;
};

m.Technology.prototype.pairedWith = function()
{
	if (!this._isPair)
		return undefined;
	return this._pairedWith;
};

m.Technology.prototype.cost = function(productionQueue)
{
	if (!this._template.cost)
		return undefined;
	let cost = {};
	for (let type in this._template.cost)
	{
		cost[type] = +this._template.cost[type];
		if (productionQueue)
			cost[type] *= productionQueue.techCostMultiplier(type);
	}
	return cost;
};

m.Technology.prototype.costSum = function(productionQueue)
{
	let cost = this.cost(productionQueue);
	if (!cost)
		return undefined;
	let ret = 0;
	for (let type in cost)
		ret += cost[type];
	return ret;
};

// seconds
m.Technology.prototype.researchTime = function()
{
	if (!this._template.researchTime)
		return undefined;
	return this._template.researchTime;
};

m.Technology.prototype.requirements = function()
{
	if (!this._template.requirements)
		return undefined;
	return this._template.requirements;
};

m.Technology.prototype.autoResearch = function()
{
	if (!this._template.autoResearch)
		return undefined;
	return this._template.autoResearch;
};

m.Technology.prototype.supersedes = function()
{
	if (!this._template.supersedes)
		return undefined;
	return this._template.supersedes;
};

m.Technology.prototype.modifications = function()
{
	if (!this._template.modifications)
		return undefined;
	return this._template.modifications;
};

m.Technology.prototype.affects = function()
{
	if (!this._template.affects)
		return undefined;
	return this._template.affects;
};

m.Technology.prototype.isAffected = function(classes)
{
	if (!this._template.affects)
		return false;
	
	for (let affect of this._template.affects)
	{
		let reqClasses = affect.split(" ");
		let fitting = true;
		for (let reqClass of reqClasses)
		{
			if (classes.indexOf(reqClass) !== -1)
				continue;
			fitting = false;
			break;
		}
		if (fitting === true)
			return true;
	}
	return false;
};

return m;

}(API3);
