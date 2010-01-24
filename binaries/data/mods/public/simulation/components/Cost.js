function Cost() {}

Cost.prototype.Init = function()
{
};

Cost.prototype.GetPopCost = function()
{
	if ('Population' in this.template)
		return +this.template.Population;
	return 0;
};

Cost.prototype.GetPopBonus = function()
{
	if ('PopulationBonus' in this.template)
		return +this.template.PopulationBonus;
	return 0;
};

Engine.RegisterComponentType(IID_Cost, "Cost", Cost);
