function GarrisonHolder() {}

GarrisonHolder.prototype.Schema =
	"<element name='Max'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

/*
 * TODO: this all needs to be designed and implemented
 */

Engine.RegisterComponentType(IID_GarrisonHolder, "GarrisonHolder", GarrisonHolder);
