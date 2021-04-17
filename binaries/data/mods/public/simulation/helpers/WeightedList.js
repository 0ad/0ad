function WeightedList()
{
	this.elements = new Map();
	this.totalWeight = 0;
};

WeightedList.prototype.length = function()
{
	return this.elements.size;
};

WeightedList.prototype.push = function(item, weight = 1)
{
	this.elements.set(item, weight);
	this.totalWeight += weight;
};

WeightedList.prototype.remove = function(item)
{
	const weight = this.elements.get(item);
	if (weight)
		this.totalWeight -= weight;
	this.elements.delete(item);
};

WeightedList.prototype.randomItem = function()
{
	const targetWeight = randFloat(0, this.totalWeight);
	let cumulativeWeight = 0;
	for (let [item, weight] of this.elements)
	{
		cumulativeWeight += weight;
		if (cumulativeWeight >= targetWeight)
			return item;
	}
	return undefined;
};

Engine.RegisterGlobal("WeightedList", WeightedList);
