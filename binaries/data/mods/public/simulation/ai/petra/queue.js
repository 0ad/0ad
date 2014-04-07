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
	for (var i in this.queue)
	{
		if (plan.category === "unit" && this.queue[i].type == plan.type && this.queue[i].number + plan.number <= this.queue[i].maxMerge)
		{
			this.queue[i].addItem(plan.number)
			return;
		}
		else if (plan.category === "technology" && this.queue[i].type === plan.type)
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
	for (var key in this.queue)
		cost.add(this.queue[key].getCost());
	return cost;
};

m.Queue.prototype.length = function()
{
	return this.queue.length;
};

m.Queue.prototype.countQueuedUnits = function()
{
	var count = 0;
	for (var i in this.queue)
		count += this.queue[i].number;
	return count;
};

m.Queue.prototype.countQueuedUnitsWithClass = function(classe)
{
	var count = 0;
	for (var i in this.queue)
		if (this.queue[i].template && this.queue[i].template.hasClass(classe))
			count += this.queue[i].number;
	return count;
};
m.Queue.prototype.countQueuedUnitsWithMetadata = function(data,value)
{
	var count = 0;
	for (var i in this.queue)
		if (this.queue[i].metadata[data] && this.queue[i].metadata[data] == value)
			count += this.queue[i].number;
	return count;
};

m.Queue.prototype.countAllByType = function(t)
{
	var count = 0;
	for (var i in this.queue)
		if (this.queue[i].type === t)
			count += this.queue[i].number;
	return count;
};

return m;
}(PETRA);
