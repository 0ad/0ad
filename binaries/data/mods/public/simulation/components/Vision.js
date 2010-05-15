function Vision() {}

Vision.prototype.Schema =
	"<optional>" +
		"<element name='Range'>" +
			"<data type='positiveInteger'/>" +
		"</element>" +
	"</optional>" +
	"<element name='RetainInFog'>" +
		"<data type='boolean'/>" +
	"</element>";

/*
 * TODO: this all needs to be designed and implemented
 */

Engine.RegisterComponentType(IID_Vision, "Vision", Vision);
