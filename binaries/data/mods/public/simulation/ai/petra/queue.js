var PETRA = function(m)
{

/*
 * Holds a list of wanted items to train or construct
 */

m.Queue = function()
{
	this.queue = [];
	this.paused = false;
	this.switched = 0;
};

m.Queue.prototype.empty = function()
{
	this.queue = [];
};

m.Queue.prototype.addItem = function(plan)
{
	if (!plan)
		return;
	for (let q of this.queue)
	{
		if (plan.category === "unit" && q.type == plan.type && q.number + plan.number <= q.maxMerge)
		{
			q.addItem(plan.number);
			return;
		}
		else if (plan.category === "technology" && q.type === plan.type)
			return;
	}
	this.queue.push(plan);
};

m.Queue.prototype.check= function(gameState)
{
	while (this.queue.length > 0)
	{
		if (!this.queue[0].isInvalid(gameState))
			return;
		this.queue.shift();
	}
	return;
};

m.Queue.prototype.getNext = function()
{
	if (this.queue.length > 0)
		return this.queue[0];
	else
		return null;
};

m.Queue.prototype.startNext = function(gameState)
{
	if (this.queue.length > 0)
	{
		this.queue.shift().start(gameState);
		return true;
	}
	else
		return false;
};

// returns the maximal account we'll accept for this queue.
// Currently all the cost of the first element and fraction of that of the second
m.Queue.prototype.maxAccountWanted = function(gameState, fraction)
{
	var cost = new API3.Resources();
	if (this.queue.length > 0 && this.queue[0].isGo(gameState))
		cost.add(this.queue[0].getCost());
	if (this.queue.length > 1 && this.queue[1].isGo(gameState) && fraction > 0)
	{
		var costs = this.queue[1].getCost();
		costs.multiply(fraction);
		cost.add(costs);
	}
	return cost;
};

m.Queue.prototype.queueCost = function()
{
	var cost = new API3.Resources();
	for (let q of this.queue)
		cost.add(q.getCost());
	return cost;
};

m.Queue.prototype.length = function()
{
	return this.queue.length;
};

m.Queue.prototype.countQueuedUnits = function()
{
	var count = 0;
	for (let q of this.queue)
		count += q.number;
	return count;
};

m.Queue.prototype.countQueuedUnitsWithClass = function(classe)
{
	var count = 0;
	for (let q of this.queue)
		if (q.template && q.template.hasClass(classe))
			count += q.number;
	return count;
};
m.Queue.prototype.countQueuedUnitsWithMetadata = function(data, value)
{
	var count = 0;
	for (let q of this.queue)
		if (q.metadata[data] && q.metadata[data] == value)
			count += q.number;
	return count;
};

m.Queue.prototype.countAllByType = function(t)
{
	var count = 0;
	for (let q of this.queue)
		if (q.type === t)
			count += q.number;
	return count;
};

m.Queue.prototype.Serialize = function()
{
	let queue = [];
	for (let plan of this.queue)
		queue.push(plan.Serialize());

	return { "queue": queue, "paused": this.paused, "switched": this.switched };
};

m.Queue.prototype.Deserialize = function(gameState, data)
{
	this.paused = data.paused;
	this.switched = data.switched;
	this.queue = [];
	for (let dataPlan of data.queue)
	{
		if (dataPlan.prop.category == "unit")
			var plan = new m.TrainingPlan(gameState, dataPlan.prop.type);
		else if (dataPlan.prop.category == "building")
			var plan = new m.ConstructionPlan(gameState, dataPlan.prop.type);
		else if (dataPlan.prop.category == "technology")
			var plan = new m.ResearchPlan(gameState, dataPlan.prop.type);
		else
		{
			API3.warn("Petra deserialization error: plan unknown " + uneval(dataPlan));
			continue;
		}
		plan.Deserialize(gameState, dataPlan);
		this.queue.push(plan);
	}
};

return m;
}(PETRA);
