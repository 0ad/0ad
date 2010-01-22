function Cost() {}

Cost.prototype.Init = function()
{
};

Cost.prototype.GetPopCost = function()
{
	return +this.template.Population;
};

Engine.RegisterComponentType(IID_Cost, "Cost", Cost);
