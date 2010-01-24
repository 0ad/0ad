function Builder() {}

Builder.prototype.Init = function()
{
};

Builder.prototype.GetEntitiesList = function()
{
	var string = this.template.Entities._string;
	
	// Replace the "{civ}" codes with this entity's civ ID
	var cmpIdentity = Engine.QueryInterface(this.entity, IID_Identity);
	if (cmpIdentity)
		string = string.replace(/\{civ\}/g, cmpIdentity.GetCiv());
	
	return string.split(/\s+/);
};

Engine.RegisterComponentType(IID_Builder, "Builder", Builder);
