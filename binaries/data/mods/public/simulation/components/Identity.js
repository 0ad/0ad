function Identity() {}

Identity.prototype.Schema =
	"<a:help>Specifies various names and values associated with the unit type, typically for GUI display to users.</a:help>" +
	"<a:example>" +
		"<Civ>hele</Civ>" +
		"<GenericName>Infantry Spearman</GenericName>" +
		"<SpecificName>Hoplite</SpecificName>" +
		"<Icon>units/hele_infantry_spearman.png</Icon>" +
	"</a:example>" +
	"<element name='Civ' a:help='Civilisation that this unit is primarily associated with, typically a 4-letter code. Choices include: gaia (world objects), cart (Carthaginians), celt (Celts), hele (Hellenes), iber (Iberians), pers (Persians), rome (Romans)'>" +
		"<text/>" +
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
		"<element name='SelectionGroupName' a:help='Name used to group ranked entities'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Tooltip'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Rollover'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='History'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Rank'>" +
			"<choice>" +
				"<value>Basic</value>" +
				"<value>Advanced</value>" +
				"<value>Elite</value>" +
			"</choice>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Classes' a:help='Optional list of space-separated classes applying to this entity. Choices include: Unit, Infantry, Melee, Cavalry, Ranged, Mechanical, Ship, Siege, Champion, Hero, Elephant, Chariot, Mercenary, Spear, Sword, Bow, Javelin, Sling, Support, Animal, Organic, Structure, Civic, CivCentre, Economic, Defensive, Gates, Wall, BarterMarket, Village, Town, City, ConquestCritical, Worker, Female, Healer, Slave, CitizenSoldier, Trade, Market, NavalMarket, Warship, SeaCreature, ForestPlant, DropsiteFood, DropsiteWood, DropsiteStone, DropsiteMetal'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Formations' a:help='Optional list of space-separated formations this unit is allowed to use. Choices include: Loose, Box, ColumnClosed, LineClosed, ColumnOpen, LineOpen, Flank, Skirmish, Wedge, Testudo, Phalanx, Syntagma, Formation12'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Icon'>" +
			"<text/>" +
		"</element>" +
	"</optional>";


Identity.prototype.Init = function()
{
};

Identity.prototype.Serialize = null; // we have no dynamic state to save

Identity.prototype.GetCiv = function()
{
	return this.template.Civ;
};

Identity.prototype.GetRank = function()
{
	return (this.template.Rank || "");
};

Identity.prototype.GetClassesList = function()
{
	if (this.template.Classes && "_string" in this.template.Classes)
	{
		var string = this.template.Classes._string;
		return string.split(/\s+/);
	}
	else
	{
		return [];
	}
};

Identity.prototype.HasClass = function(name)
{
	return this.GetClassesList().indexOf(name) != -1;
};

Identity.prototype.GetFormationsList = function()
{
	if (this.template.Formations && "_string" in this.template.Formations)
	{
		var string = this.template.Formations._string;
		return string.split(/\s+/);
	}
	else
	{
		return [];
	}
};

Identity.prototype.CanUseFormation = function(name)
{
	return this.GetFormationsList().indexOf(name) != -1;
};

Identity.prototype.GetSelectionGroupName = function()
{
	return (this.template.SelectionGroupName || "");
}

Engine.RegisterComponentType(IID_Identity, "Identity", Identity);
