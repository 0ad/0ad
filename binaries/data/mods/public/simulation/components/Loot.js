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

/*
 * TODO: this all needs to be designed and implemented
 */

Engine.RegisterComponentType(IID_Loot, "Loot", Loot);
