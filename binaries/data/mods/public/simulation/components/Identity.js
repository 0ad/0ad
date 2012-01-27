function Identity() {}

Identity.prototype.Schema =
	"<a:help>Specifies various names and values associated with the unit type, typically for GUI display to users.</a:help>" +
	"<a:example>" +
		"<Civ>hele</Civ>" +
		"<GenericName>Infantry Spearman</GenericName>" +
		"<SpecificName>Hoplite</SpecificName>" +
		"<Icon>units/hele_infantry_spearman.png</Icon>" +
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
		"<element name='Classes'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<list>" +
				"<zeroOrMore>" +
					"<choice>" +
						"<value>Unit</value>" +
						"<value>Infantry</value>" +
						"<value>Melee</value>" +
						"<value>Cavalry</value>" +
						"<value>Ranged</value>" +
						"<value>Mechanical</value>" +
						"<value>Ship</value>" +
						"<value>Siege</value>" +
						"<value>Champion</value>" +
						"<value>Hero</value>" +
						"<value>Elephant</value>" +
						"<value>Chariot</value>" +
						"<value>Mercenary</value>" +
						"<value>Spear</value>" +
						"<value>Sword</value>" +
						"<value>Bow</value>" +
						"<value>Javelin</value>" +
						"<value>Sling</value>" +
						"<value>Support</value>" +
						"<value>Animal</value>" +
						"<value>Organic</value>" +
						"<value>Structure</value>" +
						"<value>Civic</value>" +
						"<value>CivCentre</value>" +
						"<value>Economic</value>" +
						"<value>Defensive</value>" +
						"<value>Gates</value>" +
						"<value>Wall</value>" +
						"<value>BarterMarket</value>" +
						"<value>Village</value>" +
						"<value>Town</value>" +
						"<value>City</value>" +
						"<value>ConquestCritical</value>" +
						"<value>Worker</value>" +
						"<value>Female</value>" +
						"<value>Healer</value>" +
						"<value>Slave</value>" +
						"<value>CitizenSoldier</value>" +
						"<value>Trade</value>" +
						"<value>Warship</value>" +
						"<value>SeaCreature</value>" +
						"<value>ForestPlant</value>" +
						"<value>DropsiteFood</value>" +
						"<value>DropsiteWood</value>" +
						"<value>DropsiteStone</value>" +
						"<value>DropsiteMetal</value>" +
					"</choice>" +
				"</zeroOrMore>" +
			"</list>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Formations'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<list>" +
				"<zeroOrMore>" +
					"<choice>" +
						"<value>Loose</value>" +
						"<value>Box</value>" +
						"<value>ColumnClosed</value>" +
						"<value>LineClosed</value>" +
						"<value>ColumnOpen</value>" +
						"<value>LineOpen</value>" +
						"<value>Flank</value>" +
						"<value>Skirmish</value>" +
						"<value>Wedge</value>" +
						"<value>Testudo</value>" +
						"<value>Phalanx</value>" +
						"<value>Syntagma</value>" +
						"<value>Formation12</value>" +
					"</choice>" +
				"</zeroOrMore>" +
			"</list>" +
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
