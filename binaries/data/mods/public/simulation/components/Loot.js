function Loot() {}

Loot.prototype.Schema =
	"<optional>" +
		"<element name='xp'><data type='nonNegativeInteger'/></element>" +
	"</optional>" +
	"<optional>" +
		"<element name='food'><data type='nonNegativeInteger'/></element>" +
	"</optional>" +
	"<optional>" +
		"<element name='wood'><data type='nonNegativeInteger'/></element>" +
	"</optional>" +
	"<optional>" +
		"<element name='stone'><data type='nonNegativeInteger'/></element>" +
	"</optional>" +
	"<optional>" +
		"<element name='metal'><data type='nonNegativeInteger'/></element>" +
	"</optional>";

Loot.prototype.Serialize = null; // we have no dynamic state to save

Loot.prototype.GetXp = function()
{
	return this.template.xp;
};

Loot.prototype.GetResources = function()
{
	return {
		"food": +(this.template.food || 0),
		"wood": +(this.template.wood || 0),
		"metal": +(this.template.metal || 0),
		"stone": +(this.template.stone || 0)
	};
};

Engine.RegisterComponentType(IID_Loot, "Loot", Loot);
