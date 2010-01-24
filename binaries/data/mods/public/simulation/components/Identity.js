function Identity() {}

Identity.prototype.Init = function()
{
};

Identity.prototype.GetCiv = function()
{
	return this.template.Civ;
};

Engine.RegisterComponentType(IID_Identity, "Identity", Identity);
