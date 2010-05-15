function Stamina() {}

Stamina.prototype.Schema =
	"<element name='Max' a:help='Maximum stamina (msecs of running)'>" +
		"<data type='positiveInteger'/>" +
	"</element>";

/*
 * TODO: this all needs to be designed and implemented
 */

Engine.RegisterComponentType(IID_Stamina, "Stamina", Stamina);
