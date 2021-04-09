Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Builder.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/Repairable.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Builder.js");
Engine.LoadComponentScript("Timer.js");

const builderId = 6;
const target = 7;
const playerId = 1;
const playerEntityID = 2;

AddMock(SYSTEM_ENTITY, IID_ObstructionManager, {
	"IsInTargetRange": () => true
});

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": () => true
});

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (prop, oVal, ent) => oVal);


let cmpBuilder = ConstructComponent(builderId, "Builder", {
	"Rate": "1.0",
	"Entities": { "_string": "structures/{civ}/barracks structures/{civ}/civil_centre structures/{native}/house" }
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

AddMock(builderId, IID_Identity, {
	"GetCiv": () => "iber"
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber/barracks", "structures/iber/civil_centre", "structures/iber/house"]);

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": name => name == "structures/iber/civil_centre"
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber/civil_centre"]);

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": () => true
});

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "iber",
	"GetDisabledTemplates": () => ({ "structures/athen/barracks": true }),
	"GetPlayerID": () => playerId
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber/barracks", "structures/iber/civil_centre", "structures/iber/house"]);

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "iber",
	"GetDisabledTemplates": () => ({ "structures/iber/barracks": true }),
	"GetPlayerID": () => playerId
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber/civil_centre", "structures/iber/house"]);

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "athen",
	"GetDisabledTemplates": () => ({ "structures/athen/barracks": true }),
	"GetPlayerID": () => playerId
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/athen/civil_centre", "structures/iber/house"]);

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetRange(), { "max": 2, "min": 0 });

AddMock(builderId, IID_Obstruction, {
	"GetSize": () => 1
});

TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetRange(), { "max": 3, "min": 0 });

// Test repairing.
AddMock(playerEntityID, IID_Player, {
	"IsAlly": (p) => p == playerId
});

AddMock(target, IID_Ownership, {
	"GetOwner": () => playerId
});

let increased = false;
AddMock(target, IID_Foundation, {
	"Build": (entity, amount) => {
		increased = true;
		TS_ASSERT_EQUALS(amount, 1);
	},
	"AddBuilder": () => {}
});

let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer");

TS_ASSERT(cmpBuilder.StartRepairing(target));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT(increased);
increased = false;
cmpTimer.OnUpdate({ "turnLength": 2 });
TS_ASSERT(increased);
