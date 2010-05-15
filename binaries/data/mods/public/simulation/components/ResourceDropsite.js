function ResourceDropsite() {}

ResourceDropsite.prototype.Schema =
	"<element name='Radius'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<element name='Types'>" +
		"<list>" +
			"<oneOrMore>" +
				"<choice>" +
					"<value>food</value>" +
					"<value>wood</value>" +
					"<value>stone</value>" +
					"<value>metal</value>" +
				"</choice>" +
			"</oneOrMore>" +
		"</list>" +
	"</element>";

/*
 * TODO: this all needs to be designed and implemented
 */

Engine.RegisterComponentType(IID_ResourceDropsite, "ResourceDropsite", ResourceDropsite);
