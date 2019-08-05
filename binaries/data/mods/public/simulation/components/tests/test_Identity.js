Engine.LoadComponentScript("Identity.js");

let cmpIdentity = ConstructComponent(5, "Identity", {
	"Civ": "iber",
	"GenericName": "Iberian Skirmisher",
	"Phenotype": { "_string": "male" },
});

TS_ASSERT_EQUALS(cmpIdentity.GetCiv(), "iber");
TS_ASSERT_EQUALS(cmpIdentity.GetLang(), "greek");
TS_ASSERT_EQUALS(cmpIdentity.GetPhenotype(), "male");
TS_ASSERT_EQUALS(cmpIdentity.GetRank(), "");
TS_ASSERT_UNEVAL_EQUALS(cmpIdentity.GetClassesList(), []);
TS_ASSERT_UNEVAL_EQUALS(cmpIdentity.GetVisibleClassesList(), []);
TS_ASSERT_EQUALS(cmpIdentity.HasClass("CitizenSoldier"), false);
TS_ASSERT_UNEVAL_EQUALS(cmpIdentity.GetFormationsList(), []);
TS_ASSERT_EQUALS(cmpIdentity.CanUseFormation("special/formations/skirmish"), false);
TS_ASSERT_EQUALS(cmpIdentity.GetSelectionGroupName(), "");
TS_ASSERT_EQUALS(cmpIdentity.GetGenericName(), "Iberian Skirmisher");

cmpIdentity = ConstructComponent(6, "Identity", {
	"Civ": "iber",
	"Lang": "iberian",
	"Phenotype":  { "_string": "female" },
	"GenericName": "Iberian Skirmisher",
	"SpecificName": "Lusitano Ezpatari",
	"SelectionGroupName": "units/iber_infantry_javelinist_b",
	"Tooltip": "Basic ranged infantry",
	"History": "Iberians, especially the Lusitanians, were good at" +
		" ranged combat and ambushing enemy columns. They throw heavy iron" +
		" javelins and sometimes even add burning pitch to them, making them" +
		" good as a cheap siege weapon.",
	"Rank": "Basic",
	"Classes": { "_string": "CitizenSoldier Human Organic" },
	"VisibleClasses": { "_string": "Javelin" },
	"Formations": { "_string": "special/formations/skirmish" },
	"Icon": "units/iber_infantry_javelinist.png",
	"RequiredTechnology": "phase_town"
});

TS_ASSERT_EQUALS(cmpIdentity.GetCiv(), "iber");
TS_ASSERT_EQUALS(cmpIdentity.GetLang(), "iberian");
TS_ASSERT_EQUALS(cmpIdentity.GetPhenotype(), "female");
TS_ASSERT_EQUALS(cmpIdentity.GetRank(), "Basic");
TS_ASSERT_UNEVAL_EQUALS(cmpIdentity.GetClassesList(), ["CitizenSoldier", "Human", "Organic", "Javelin", "Basic"]);
TS_ASSERT_UNEVAL_EQUALS(cmpIdentity.GetVisibleClassesList(), ["Javelin"]);
TS_ASSERT_EQUALS(cmpIdentity.HasClass("CitizenSoldier"), true);
TS_ASSERT_EQUALS(cmpIdentity.HasClass("Female"), false);
TS_ASSERT_UNEVAL_EQUALS(cmpIdentity.GetFormationsList(), ["special/formations/skirmish"]);
TS_ASSERT_EQUALS(cmpIdentity.CanUseFormation("special/formations/skirmish"), true);
TS_ASSERT_EQUALS(cmpIdentity.CanUseFormation("special/formations/line"), false);
TS_ASSERT_EQUALS(cmpIdentity.GetSelectionGroupName(), "units/iber_infantry_javelinist_b");
TS_ASSERT_EQUALS(cmpIdentity.GetGenericName(), "Iberian Skirmisher");

cmpIdentity = ConstructComponent(7, "Identity", {
	"Phenotype": { "_string": "First Second" },
});
TS_ASSERT_UNEVAL_EQUALS(cmpIdentity.GetPossiblePhenotypes(), ["First", "Second"]);
TS_ASSERT(["First", "Second"].indexOf(cmpIdentity.GetPhenotype()) !== -1);

cmpIdentity = ConstructComponent(8, "Identity", {});
TS_ASSERT_EQUALS(cmpIdentity.GetPhenotype(), "default");
