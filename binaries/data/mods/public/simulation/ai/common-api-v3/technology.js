// Wrapper around a technology template

function Technology(allTemplates, templateName)
{
	this._templateName = templateName;
	var template = allTemplates[templateName];
	
	// check if this is one of two paired technologies.
	this._isPair = template.pair === undefined ? false : true;
	if (this._isPair)
	{
		if (allTemplates[template.pair].top == templateName)
			this._pairedWith = allTemplates[template.pair].bottom;
		else
			this._pairedWith = allTemplates[template.pair].top;
	}
	// check if it only defines a pair:
	this._definesPair = template.top === undefined ? false : true;
	this._template = template;
	this._techTemplates = allTemplates;
}
// returns generic, or specific if civ provided.
Technology.prototype.name = function(civ)
{
	if (civ === undefined)
	{
		return this._template.genericName;
	}
	else
	{
		if (this._template.specificName === undefined || this._template.specificName[civ] === undefined)
			return undefined;
		return this._template.specificName[civ];
	}
};

Technology.prototype.pairDef = function()
{
	return this._definesPair;
};
// in case this defines a pair only, returns the two paired technologies.
Technology.prototype.getPairedTechs = function()
{
	if (!this._definesPair)
		return undefined;
	
	var techOne = new Technology(this._techTemplates, this._template.top);
	var techTwo = new Technology(this._techTemplates, this._template.bottom);
	
	return [techOne,techTwo];
};

Technology.prototype.pair = function()
{
	if (!this._isPair)
		return undefined;
	return this._template.pair;
};

Technology.prototype.pairedWith = function()
{
	if (!this._isPair)
		return undefined;
	return this._pairedWith;
};

Technology.prototype.cost = function()
{
	if (!this._template.cost)
		return undefined;
	return this._template.cost;
};

// seconds
Technology.prototype.researchTime = function()
{
	if (!this._template.researchTime)
		return undefined;
	return this._template.researchTime;
};

Technology.prototype.requirements = function()
{
	if (!this._template.requirements)
		return undefined;
	return this._template.requirements;
};

Technology.prototype.autoResearch = function()
{
	if (!this._template.autoResearch)
		return undefined;
	return this._template.autoResearch;
};

Technology.prototype.supersedes = function()
{
	if (!this._template.supersedes)
		return undefined;
	return this._template.supersedes;
};

Technology.prototype.modifications = function()
{
	if (!this._template.modifications)
		return undefined;
	return this._template.modifications;
};

Technology.prototype.affects = function()
{
	if (!this._template.affects)
		return undefined;
	return this._template.affects;
};

Technology.prototype.isAffected = function(classes)
{
	if (!this._template.affects)
		return false;
	
	for (var index in this._template.affects)
	{
		var reqClasses = this._template.affects[index].split(" ");
		var fitting = true;
		for (var i in reqClasses)
		{
			if (classes.indexOf(reqClasses[i]) === -1)
			{
				fitting = false;
				break;
			}
		}
		if (fitting === true)
			return true;
	}
	return false;
};
