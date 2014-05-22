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
		"<element name='GateConversionTooltip'>" +
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
		"<element name='Classes' a:help='Optional list of space-separated classes applying to this entity. Choices include: Unit, Infantry, Melee, Cavalry, Ranged, Mechanical, Ship, Siege, Champion, Hero, Elephant, Chariot, Mercenary, Spear, Sword, Bow, Javelin, Sling, Support, Animal, Domestic, Organic, Structure, Civic, CivCentre, Economic, Defensive, Gates, Wall, BarterMarket, Village, Town, City, ConquestCritical, Worker, Female, Healer, Slave, CitizenSoldier, Trade, Market, NavalMarket, Warship, SeaCreature, ForestPlant, DropsiteFood, DropsiteWood, DropsiteStone, DropsiteMetal, GarrisonTower, GarrisonFortress'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='VisibleClasses' a:help='Optional list of space-separated classes applying to this entity. These classes will also be visible in various GUI elements, if the classes need spaces. Underscores will be replaced with spaces.'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Formations' a:help='Optional list of space-separated formations this unit is allowed to use. Choices include: Scatter, Box, ColumnClosed, LineClosed, ColumnOpen, LineOpen, Flank, Skirmish, Wedge, Testudo, Phalanx, Syntagma, BattleLine'>" +
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
	"</optional>" +
	"<optional>" +
		"<element name='RequiredTechnology' a:help='Optional name of a technology which must be researched before the entity can be produced'>" +
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
	return GetIdentityClasses(this.template);
};

Identity.prototype.GetVisibleClassesList = function()
{
	return GetVisibleIdentityClasses(this.template);
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
	return [];
};

Identity.prototype.CanUseFormation = function(template)
{
	return this.GetFormationsList().indexOf(template) != -1;
};

Identity.prototype.GetSelectionGroupName = function()
{
	return (this.template.SelectionGroupName || "");
};

Identity.prototype.GetGenericName = function()
{
	return this.template.GenericName;
};

Engine.RegisterComponentType(IID_Identity, "Identity", Identity);
