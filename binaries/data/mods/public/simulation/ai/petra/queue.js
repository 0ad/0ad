var PETRA = function(m)
{

/*
 * Holds a list of wanted plans to train or construct
 */

m.Queue = function()
{
	this.plans = [];
	this.paused = false;
	this.switched = 0;
};

m.Queue.prototype.empty = function()
{
	this.plans = [];
};

m.Queue.prototype.addPlan = function(newPlan)
{
	if (!newPlan)
		return;
	for (let plan of this.plans)
	{
		if (newPlan.category === "unit" && plan.type == newPlan.type && plan.number + newPlan.number <= plan.maxMerge)
		{
			plan.addItem(newPlan.number);
			return;
		}
		else if (newPlan.category === "technology" && plan.type === newPlan.type)
			return;
	}
	this.plans.push(newPlan);
};

m.Queue.prototype.check= function(gameState)
{
	while (this.plans.length > 0)
	{
		if (!this.plans[0].isInvalid(gameState))
			return;
		this.plans.shift();
	}
	return;
};

m.Queue.prototype.getNext = function()
{
	if (this.plans.length > 0)
		return this.plans[0];
	return null;
};

m.Queue.prototype.startNext = function(gameState)
{
	if (this.plans.length > 0)
	{
		this.plans.shift().start(gameState);
		return true;
	}
	return false;
};

// returns the maximal account we'll accept for this queue.
// Currently all the cost of the first element and fraction of that of the second
m.Queue.prototype.maxAccountWanted = function(gameState, fraction)
{
	let cost = new API3.Resources();
	if (this.plans.length > 0 && this.plans[0].isGo(gameState))
		cost.add(this.plans[0].getCost());
	if (this.plans.length > 1 && this.plans[1].isGo(gameState) && fraction > 0)
	{
		let costs = this.plans[1].getCost();
		costs.multiply(fraction);
		cost.add(costs);
	}
	return cost;
};

m.Queue.prototype.queueCost = function()
{
	let cost = new API3.Resources();
	for (let plan of this.plans)
		cost.add(plan.getCost());
	return cost;
};

m.Queue.prototype.length = function()
{
	return this.plans.length;
};

m.Queue.prototype.hasQueuedUnits = function()
{
	return this.plans.length > 0;
};

m.Queue.prototype.countQueuedUnits = function()
{
	let count = 0;
	for (let plan of this.plans)
		count += plan.number;
	return count;
};

m.Queue.prototype.hasQueuedUnitsWithClass = function(classe)
{
	return this.plans.some(plan => plan.template && plan.template.hasClass(classe));
};

m.Queue.prototype.countQueuedUnitsWithClass = function(classe)
{
	let count = 0;
	for (let plan of this.plans)
		if (plan.template && plan.template.hasClass(classe))
			count += plan.number;
	return count;
};

m.Queue.prototype.countQueuedUnitsWithMetadata = function(data, value)
{
	let count = 0;
	for (let plan of this.plans)
		if (plan.metadata[data] && plan.metadata[data] == value)
			count += plan.number;
	return count;
};

m.Queue.prototype.Serialize = function()
{
	let plans = [];
	for (let plan of this.plans)
		plans.push(plan.Serialize());

	return { "plans": plans, "paused": this.paused, "switched": this.switched };
};

m.Queue.prototype.Deserialize = function(gameState, data)
{
	this.paused = data.paused;
	this.switched = data.switched;
	this.plans = [];
	for (let dataPlan of data.plans)
	{
		let plan;
		if (dataPlan.prop.category == "unit")
			plan = new m.TrainingPlan(gameState, dataPlan.prop.type);
		else if (dataPlan.prop.category == "building")
			plan = new m.ConstructionPlan(gameState, dataPlan.prop.type);
		else if (dataPlan.prop.category == "technology")
			plan = new m.ResearchPlan(gameState, dataPlan.prop.type);
		else
		{
			API3.warn("Petra deserialization error: plan unknown " + uneval(dataPlan));
			continue;
		}
		plan.Deserialize(gameState, dataPlan);
		this.plans.push(plan);
	}
};

return m;
}(PETRA);
