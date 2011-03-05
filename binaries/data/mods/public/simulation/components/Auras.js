function Auras() {}

Auras.prototype.Schema =
	"<oneOrMore>" +
		"<element>" +
			"<choice>" +
				"<name>Allure</name>" +
				"<name>Infidelity</name>" +
				"<name>Heal</name>" +
				"<name>Courage</name>" +
				"<name>Fear</name>" +
			"</choice>" +
			"<interleave>" +
				"<element name='Radius'>" +
					"<data type='nonNegativeInteger'/>" +
				"</element>" +
				"<optional>" +
					"<element name='Bonus'>" +
						"<data type='positiveInteger'/>" +
					"</element>" +
				"</optional>" +
				"<optional>" +
					"<element name='Time'>" +
						"<data type='nonNegativeInteger'/>" +
					"</element>" +
				"</optional>" +
				"<optional>" +
					"<element name='Speed'>" +
						"<data type='positiveInteger'/>" +
					"</element>" +
				"</optional>" +
			"</interleave>" +
		"</element>" +
	"</oneOrMore>";

/*
 * TODO: this all needs to be designed and implemented
 */

Auras.prototype.Serialize = null; // we have no dynamic state to save

Engine.RegisterComponentType(IID_Auras, "Auras", Auras);
