function Promotion() {}

Promotion.prototype.Schema =
	"<element name='Entity'>" +
		"<text/>" +
	"</element>" +
	"<element name='Req'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

/*
 * TODO: this all needs to be designed and implemented
 */

Engine.RegisterComponentType(IID_Promotion, "Promotion", Promotion);
