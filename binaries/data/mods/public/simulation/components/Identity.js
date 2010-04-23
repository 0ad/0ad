function Identity() {}

Identity.prototype.Schema =
	"<a:help>Specifies various names and values associated with the unit type, typically for GUI display to users.</a:help>" +
	"<a:example>" +
		"<Civ>hele</Civ>" +
		"<GenericName>Infantry Spearman</GenericName>" +
		"<SpecificName>Hoplite</SpecificName>" +
		"<IconCell>3</IconCell>" +
		"<IconSheet>PortraitSheet</IconSheet>" +
	"</a:example>" +
	"<element name='Civ' a:help='Civilisation that this unit is primarily associated with'>" +
		"<choice>" +
			"<value a:help='Gaia (world objects)'>gaia</value>" +
			"<value a:help='Carthaginians'>cart</value>" +
			"<value a:help='Celts'>celt</value>" +
			"<value a:help='Hellenes'>hele</value>" +
			"<value a:help='Iberians'>iber</value>" +
			"<value a:help='Persians'>pers</value>" +
			"<value a:help='Romans'>rome</value>" +
		"</choice>" +
	"</element>" +
	"<element name='GenericName' a:help='Generic English-language name for this class of unit'>" +
		"<text/>" +
	"</element>" +
	"<optional>" +
		"<element name='SpecificName' a:help='Specific native-language name for this unit type'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='IconCell'>" +
			"<data type='nonNegativeInteger'/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='IconSheet'>" +
			"<text/>" +
		"</element>" +
	"</optional>";


Identity.prototype.Init = function()
{
};

Identity.prototype.GetCiv = function()
{
	return this.template.Civ;
};

Engine.RegisterComponentType(IID_Identity, "Identity", Identity);
