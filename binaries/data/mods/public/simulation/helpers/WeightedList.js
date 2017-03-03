var WeightedList = function()
{
    this.elements = [ ];
    this.totalWeight = 0;
};

WeightedList.prototype.length = function()
{
	return this.elements.length;
};

WeightedList.prototype.push = function(item, weight)
{
	if (weight === undefined)
		weight = 1;
	this.totalWeight += weight;
	this.elements.push({ "item": item, "weight": weight });
};

WeightedList.prototype.removeAt = function(index)
{
	var element = this.elements.splice(index, 1)[0];
	if (element)
		this.totalWeight -= element.weight;
};

WeightedList.prototype.itemAt = function(index)
{
	var element = this.elements[index];
	return element ? element.item : null;
};

WeightedList.prototype.randomIndex = function() {
	var element;
	var targetWeight = randFloat(0, this.totalWeight);
	var cumulativeWeight = 0;
	for (var index = 0; index < this.elements.length; index++)
	{
		element = this.elements[index];
		cumulativeWeight += element.weight;
		if (cumulativeWeight >= targetWeight)
			return index;
	}
	return -1;
};

Engine.RegisterGlobal("WeightedList", WeightedList);
