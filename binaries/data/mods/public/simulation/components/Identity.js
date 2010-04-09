function Identity() {}

Identity.prototype.Schema =
	"<element name='Civ'>" +
		"<text/>" + // TODO: <choice>?
	"</element>" +
	"<element name='GenericName'>" +
		"<text/>" +
	"</element>" +
	"<optional>" +
		"<element name='SpecificName'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<element name='IconCell'>" +
		"<data type='nonNegativeInteger'/>" +
	"</element>" +
	"<element name='IconSheet'>" +
		"<text/>" +
	"</element>";

Identity.prototype.Init = function()
{
};

Identity.prototype.GetCiv = function()
{
	return this.template.Civ;
};

Engine.RegisterComponentType(IID_Identity, "Identity", Identity);
