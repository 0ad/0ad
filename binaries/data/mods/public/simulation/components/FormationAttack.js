function FormationAttack() {}

FormationAttack.prototype.Schema =
	"<element name='CanAttackAsFormation'>" +
		"<text/>" +
	"</element>";

FormationAttack.prototype.Init = function()
{
	this.canAttackAsFormation = this.template.CanAttackAsFormation == "true";
};

FormationAttack.prototype.CanAttackAsFormation = function()
{
	return this.canAttackAsFormation;
};

FormationAttack.prototype.GetRange = function(target)
{
	var result = {"min": 0, "max": this.canAttackAsFormation ? -1 : 0};
	var cmpFormation = Engine.QueryInterface(this.entity, IID_Formation);
	if (!cmpFormation)
	{
		warn("FormationAttack component used on a non-formation entity");
		return result;
	}
	var members = cmpFormation.GetMembers();
	for (var ent of members)
	{
		var cmpAttack = Engine.QueryInterface(ent, IID_Attack);
		if (!cmpAttack)
			continue;

		var type = cmpAttack.GetBestAttackAgainst(target);
		if (!type)
			continue;

		// if the formation can attack, take the minimum max range (so units are certainly in range),
		// If the formation can't attack, take the maximum max range as the point where the formation will be disbanded
		// Always take the minimum min range (to not get impossible situations)
		var range = cmpAttack.GetRange(type);

		if (this.canAttackAsFormation)
		{
			if (range.max < result.max || result.max < 0)
				result.max = range.max;
		}
		else
		{
			if (range.max > result.max || range.max < 0)
				result.max = range.max;
		}
		if (range.min < result.min)
			result.min = range.min;
	}
	// add half the formation size, so it counts as the range for the units on the first row
	var extraRange = cmpFormation.GetSize().depth/2;

	if (result.max >= 0)
		result.max += extraRange;
	result.min += extraRange;
	return result;
};

Engine.RegisterComponentType(IID_Attack, "FormationAttack", FormationAttack);
