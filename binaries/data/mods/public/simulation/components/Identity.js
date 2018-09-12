function Identity() {}

Identity.prototype.Schema =
	"<a:help>Specifies various names and values associated with the unit type, typically for GUI display to users.</a:help>" +
	"<a:example>" +
		"<Civ>athen</Civ>" +
		"<GenericName>Athenian Hoplite</GenericName>" +
		"<SpecificName>Hoplī́tēs Athēnaïkós</SpecificName>" +
		"<Icon>units/athen_infantry_spearman.png</Icon>" +
	"</a:example>" +
	"<element name='Civ' a:help='Civilisation that this unit is primarily associated with, typically a 4-letter code. Choices include: gaia (world objects), athen (Athenians), brit (Britons), cart (Carthaginians), gaul (Gauls), iber (Iberians), kush (Kushites), mace (Macedonians), maur (Mauryans), pers (Persians), ptol (Ptolemies), rome (Romans), sele (Seleucids), spart (Spartans)'>" +
		"<text/>" +
	"</element>" +
	"<optional>" +
		"<element name='Lang' a:help='Unit language for voices'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Gender' a:help='Unit gender for voices. Choices includes male or female.'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<element name='GenericName' a:help='Generic English-language name for this class of unit.'>" +
		"<text/>" +
	"</element>" +
	"<optional>" +
		"<element name='SpecificName' a:help='Specific native-language name for this unit type.'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='SelectionGroupName' a:help='Name used to group ranked entities.'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='Tooltip'>" +
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
		"<element name='Classes' a:help='Optional list of space-separated classes applying to this entity. Choices include: Animal, Apadana, ArmyCamp, Ashoka, BarterMarket, Celt, CitizenSoldier, CivCentre, Colony, ConquestCritical, Defensive, Domestic, DropsiteFood, DropsiteMetal, DropsiteStone, DropsiteWood, FemaleCitizen, FishingBoat, ForestPlant, Fortress, GarrisonFortress, GarrisonTower, Gates, Human, Iberian, Immortal, Italian, Juggernaut, Kennel, Lighthouse, LongWall, MercenaryCamp, Naval, NavalMarket, Organic, Palace, Palisade, Player, PtolemyIV, SeaCreature, SiegeWall, SpecialBuilding, StoneWall, Structure, Syssiton, Theater, Tower, Unit'>" +
			"<attribute name='datatype'>" +
				"<value>tokens</value>" +
			"</attribute>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<optional>" +
		"<element name='VisibleClasses' a:help='Optional list of space-separated classes applying to this entity. These classes will also be visible in various GUI elements. If the classes need spaces, underscores will be replaced with spaces. Choices include: Archer, Barracks, Blacksmith, BoltShooter, Camel, Catapult, Cavalry, Champion, Chariot, Citizen, City, Civic, CivilCentre, Corral, DefenseTower, Dock, Dog, Economic, Elephant, ElephantStables, Embassy, Farmstead, Field, Fireship, Healer, Hero, House, Infantry, Javelin, Maceman, Market, Mechanical, Melee, Mercenary, Military, Outpost, Pike, Ram, Ranged, Relic, Resource, SentryTower, Ship, Shipyard, Siege, SiegeTower, Slave, Sling, Soldier, Spear, Stables, Storehouse, Support, Sword, Temple, Town, Trader, Village, Warship, Wonder, Worker, Workshop'>" +
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
		"<element name='RequiredTechnology' a:help='Optional name of a technology which must be researched before the entity can be produced.'>" +
			"<text/>" +
		"</element>" +
	"</optional>" +
	"<element name='Undeletable' a:help='Prevent players from deleting this entity.'>" +
		"<data type='boolean'/>" +
	"</element>";

Identity.prototype.Init = function()
{
	this.classesList = GetIdentityClasses(this.template);
	this.visibleClassesList = GetVisibleIdentityClasses(this.template);
};

Identity.prototype.Deserialize = function ()
{
	this.Init();
};

Identity.prototype.Serialize = null; // we have no dynamic state to save

Identity.prototype.GetCiv = function()
{
	return this.template.Civ;
};

Identity.prototype.GetLang = function()
{
	return this.template.Lang || "greek"; // ugly default
};

Identity.prototype.GetGender = function()
{
	return this.template.Gender || "male"; // ugly default
};

Identity.prototype.GetRank = function()
{
	return this.template.Rank || "";
};

Identity.prototype.GetClassesList = function()
{
	return this.classesList;
};

Identity.prototype.GetVisibleClassesList = function()
{
	return this.visibleClassesList;
};

Identity.prototype.HasClass = function(name)
{
	return this.GetClassesList().indexOf(name) != -1;
};

Identity.prototype.GetFormationsList = function()
{
	if (this.template.Formations && this.template.Formations._string)
		return this.template.Formations._string.split(/\s+/);
	return [];
};

Identity.prototype.CanUseFormation = function(template)
{
	return this.GetFormationsList().indexOf(template) != -1;
};

Identity.prototype.GetSelectionGroupName = function()
{
	return this.template.SelectionGroupName || "";
};

Identity.prototype.GetGenericName = function()
{
	return this.template.GenericName;
};

Identity.prototype.IsUndeletable = function()
{
	return this.template.Undeletable == "true";
};

Engine.RegisterComponentType(IID_Identity, "Identity", Identity);
