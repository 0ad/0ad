function EntityLimitsChanger() {}

EntityLimitsChanger.prototype.Schema =
	"<oneOrMore>" +
		"<element a:help='Take as name the name of the category, and as value the number you want to increase the limit with.'>" +
			"<anyName/>" +
			"<data type='integer'/>" +
		"</element>" +
	"</oneOrMore>";

EntityLimitsChanger.prototype.init = function()
{
};

EntityLimitsChanger.prototype.OnOwnershipChanged = function(msg)
{
	if (!this.changes)
	{
		this.changes = {};
		for (var cat in this.template)
			this.changes[cat] = ApplyValueModificationsToEntity("EntityLimitsChanger/Value", +this.template[cat], this.entity);
	}

	if (msg.from > -1)
	{
		var cmpEntityLimits = QueryPlayerIDInterface(msg.from, IID_EntityLimits);
		if (cmpEntityLimits)
			for (var cat in this.changes)
				cmpEntityLimits.DecreaseLimit(cat, this.changes[cat]);
	}

	if (msg.to > -1)
	{
		var cmpEntityLimits = QueryPlayerIDInterface(msg.to, IID_EntityLimits);
		if (cmpEntityLimits)
			for (var cat in this.changes)
				cmpEntityLimits.IncreaseLimit(cat, this.changes[cat]);
	}
}

EntityLimitsChanger.prototype.OnValueModification = function(msg)
{
	if (msg.component != "EntityLimitsChanger")
		return;

	var cmpEntityLimits = Engine.QueryOwnerInterface(this.entity, IID_EntityLimits);
	if (!cmpEntityLimits)
		return;

	for (var cat in this.changes)
	{
		cmpEntityLimits.DecreaseLimit(cat, this.changes[cat]);
		this.changes[cat] = ApplyValueModificationsToEntity("EntityLimitsChanger/Value", +this.template[cat], this.entity);
		cmpEntityLimits.IncreaseLimit(cat, this.changes[cat]);
	}
};

Engine.RegisterComponentType(IID_EntityLimitsChanger, "EntityLimitsChanger", EntityLimitsChanger);
