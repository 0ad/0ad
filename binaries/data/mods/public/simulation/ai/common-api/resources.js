var API3 = function(m)
{

m.Resources = function(amounts, population)
{
	if (!amounts)
		amounts = { food : 0, wood : 0, stone : 0, metal : 0 };
	for (let key of this.types)
		this[key] = amounts[key] || 0;

	this.population = (population && population > 0) ? population : 0;
};

m.Resources.prototype.types = [ "food", "wood", "stone", "metal" ];

m.Resources.prototype.reset = function()
{
	for (let key of this.types)
		this[key] = 0;
	this.population = 0;
};

m.Resources.prototype.canAfford = function(that)
{
	for (let key of this.types)
		if (this[key] < that[key])
			return false;
	return true;
};

m.Resources.prototype.add = function(that)
{
	for (let key of this.types)
		this[key] += that[key];
	this.population += that.population;
};

m.Resources.prototype.subtract = function(that)
{
	for (let key of this.types)
		this[key] -= that[key];
	this.population += that.population;
};

m.Resources.prototype.multiply = function(n)
{
	for (let key of this.types)
		this[key] *= n;
	this.population *= n;
};

m.Resources.prototype.toInt = function()
{
	let sum = 0;
	for (let key of this.types)
		sum += this[key];
	sum += this.population * 50; // based on typical unit costs
	return sum;
};

m.Resources.prototype.Serialize = function()
{
	let amounts = {};
	for (let key of this.types)
		amounts[key] = this[key];
	return { "amounts": amounts, "population": this.population };
};

m.Resources.prototype.Deserialize = function(data)
{
	for (let key in data.amounts)
		this[key] = data.amounts[key];
	this.population = data.population;
};

return m;

}(API3);
