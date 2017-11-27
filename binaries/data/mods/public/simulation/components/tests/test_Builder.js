Engine.LoadHelperScript("ValueModification.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Builder.js");
Engine.LoadComponentScript("Builder.js");

const builderId = 6;
const playerId = 1;
const playerEntityID = 2;

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": () => true
});

let cmpBuilder = ConstructComponent(builderId, "Builder", {
	"Rate": 1.0,
	"Entities": { "_string": "structures/{civ}_barracks structures/{civ}_civil_centre" }
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), []);

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => playerEntityID
});

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "iber",
	"GetDisabledTemplates": () => ({}),
	"GetPlayerID": () => playerId
});

AddMock(builderId, IID_Ownership, {
	"GetOwner": () => playerId
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber_barracks", "structures/iber_civil_centre"]);

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": name => name == "structures/iber_civil_centre"
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber_civil_centre"]);

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": () => true
});

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "iber",
	"GetDisabledTemplates": () => ({ "structures/athen_barracks": true }),
	"GetPlayerID": () => playerId
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber_barracks", "structures/iber_civil_centre"]);

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "iber",
	"GetDisabledTemplates": () => ({ "structures/iber_barracks": true }),
	"GetPlayerID": () => playerId
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber_civil_centre"]);

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetRange(), { "max": 2, "min": 0 });

AddMock(builderId, IID_Obstruction, {
	"GetUnitRadius": () => 1.0
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetRange(), { "max": 3, "min": 0 });
