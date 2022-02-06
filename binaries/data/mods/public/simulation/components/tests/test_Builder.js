Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Builder.js");
Engine.LoadComponentScript("interfaces/Cost.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Repairable.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Builder.js");
Engine.LoadComponentScript("Health.js");
Engine.LoadComponentScript("Repairable.js");
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

function testEntitiesList()
{
	let cmpBuilder = ConstructComponent(builderId, "Builder", {
		"Rate": "1.0",
		"Entities": { "_string": "structures/{civ}/barracks structures/{civ}/civil_centre structures/{native}/house" }
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), []);

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => playerEntityID
	});

	AddMock(playerEntityID, IID_Player, {
		"GetDisabledTemplates": () => ({}),
		"GetPlayerID": () => playerId
	});

	AddMock(playerEntityID, IID_Identity, {
		"GetCiv": () => "iber",
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
		"GetDisabledTemplates": () => ({ "structures/athen/barracks": true }),
		"GetPlayerID": () => playerId
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber/barracks", "structures/iber/civil_centre", "structures/iber/house"]);

	AddMock(playerEntityID, IID_Player, {
		"GetDisabledTemplates": () => ({ "structures/iber/barracks": true }),
		"GetPlayerID": () => playerId
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/iber/civil_centre", "structures/iber/house"]);

	AddMock(playerEntityID, IID_Player, {
		"GetDisabledTemplates": () => ({ "structures/athen/barracks": true }),
		"GetPlayerID": () => playerId
	});

	AddMock(playerEntityID, IID_Identity, {
		"GetCiv": () => "athen",
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetEntitiesList(), ["structures/athen/civil_centre", "structures/iber/house"]);

	TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetRange(), { "max": 2, "min": 0 });

	AddMock(builderId, IID_Obstruction, {
		"GetSize": () => 1
	});

	TS_ASSERT_UNEVAL_EQUALS(cmpBuilder.GetRange(), { "max": 3, "min": 0 });
}
testEntitiesList();

function testBuildingFoundation()
{
	let cmpBuilder = ConstructComponent(builderId, "Builder", {
		"Rate": "1.0",
		"Entities": { "_string": "" }
	});

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
}
testBuildingFoundation();

function testRepairing()
{
	AddMock(playerEntityID, IID_Player, {
		"IsAlly": (p) => p == playerId
	});

	let cmpBuilder = ConstructComponent(builderId, "Builder", {
		"Rate": "1.0",
		"Entities": { "_string": "" }
	});

	AddMock(target, IID_Ownership, {
		"GetOwner": () => playerId
	});

	AddMock(target, IID_Cost, {
		"GetBuildTime": () => 100
	});

	let cmpTargetHealth = ConstructComponent(target, "Health", {
		"Max": 100,
		"RegenRate": 0,
		"IdleRegenRate": 0,
		"DeathType": "vanish",
		"Unhealable": false
	});

	cmpTargetHealth.SetHitpoints(50);

	DeleteMock(target, IID_Foundation);
	let cmpTargetRepairable = ConstructComponent(target, "Repairable", {
		"RepairTimeRatio": 1,
	});

	let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer");

	TS_ASSERT(cmpTargetRepairable.IsRepairable());
	TS_ASSERT(cmpBuilder.StartRepairing(target));
	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(cmpTargetHealth.GetHitpoints(), 51);
	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(cmpTargetHealth.GetHitpoints(), 52);
	cmpTargetRepairable.SetRepairability(false);
	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(cmpTargetHealth.GetHitpoints(), 52);
	cmpTargetRepairable.SetRepairability(true);
	// Check that we indeed stopped - shouldn't restart on its own.
	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(cmpTargetHealth.GetHitpoints(), 52);
	TS_ASSERT(cmpBuilder.StartRepairing(target));
	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(cmpTargetHealth.GetHitpoints(), 53);
}
testRepairing();
