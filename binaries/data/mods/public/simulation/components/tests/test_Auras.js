Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("Auras.js");
Engine.LoadComponentScript("AuraManager.js");

let playerID = [0, 1, 2];
let playerEnt = [10, 11, 12];
let playerState = "active";
let giverEnt = 20;
let targetEnt = 30;
let auraRange = 40;
let template = { "Identity" : { "Classes" : { "_string" : "CorrectClass OtherClass" } } };

function testAuras(name, test_function)
{
	ResetState();

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": idx => playerEnt[idx],
		"GetNumPlayers": () => 3
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"CreateActiveQuery": (ent, minRange, maxRange, players, iid, flags) => 1,
		"EnableActiveQuery": id => {},
		"ResetActiveQuery": id => {},
		"DisableActiveQuery": id => {},
		"DestroyActiveQuery": id => {},
		"GetEntityFlagMask": identifier => {},
		"GetEntitiesByPlayer": id => [30, 31, 32]
	});

	AddMock(SYSTEM_ENTITY, IID_DataTemplateManager, {
		"GetAuraTemplate": (name) => {
			let template = {
				"type": name,
				"affectedPlayers": ["Ally"],
				"affects": ["CorrectClass"],
				"modifications": [{ "value": "Component/Value", "add": 10 }],
				"auraName": "name",
				"auraDescription": "description"
			};
			if (name == "range")
				template.radius = auraRange;
			return template;
		}
	});

	AddMock(playerEnt[1], IID_Player, {
		"IsAlly": id => id == 1,
		"IsEnemy": id => id != 1,
		"GetPlayerID": () => playerID[1],
		"GetState": () => playerState
	});

	AddMock(targetEnt, IID_Identity, {
		"GetClassesList": () => ["CorrectClass", "OtherClass"]
	});

	AddMock(giverEnt, IID_Position, {
		"GetPosition2D": () => new Vector2D()
	});

	AddMock(targetEnt, IID_Position, {
		"GetPosition2D": () => new Vector2D()
	});

	if (playerEnt.indexOf(giverEnt) == -1)
		AddMock(giverEnt, IID_Ownership, {
			"GetOwner": () => playerID[1]
		});

	AddMock(targetEnt, IID_Ownership, {
		"GetOwner": () => playerID[1]
	});

	ConstructComponent(SYSTEM_ENTITY, "AuraManager", {});
	let cmpAuras = ConstructComponent(giverEnt, "Auras", { "_string": name });
	test_function(name, cmpAuras);
}

testAuras("global", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[1], template), 15);
});

giverEnt = 11;
testAuras("global", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[1], template), 15);
});

// Test the case when the aura giver is a player entity.
giverEnt = 20;
testAuras("range", (name, cmpAuras) => {
	cmpAuras.OnRangeUpdate({ "tag": 1, "added": [targetEnt], "removed": [] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[1], template), 5);
	cmpAuras.OnRangeUpdate({ "tag": 1, "added": [], "removed": [targetEnt] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 5);
});

testAuras("garrisonedUnits", (name, cmpAuras) => {
	cmpAuras.OnGarrisonedUnitsChanged({ "added" : [targetEnt], "removed": [] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	cmpAuras.OnGarrisonedUnitsChanged({ "added" : [], "removed": [targetEnt] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 5);
});

testAuras("garrison", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(cmpAuras.HasGarrisonAura(), true);
	cmpAuras.ApplyGarrisonBonus(targetEnt);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	cmpAuras.RemoveGarrisonBonus(targetEnt);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 1);
});

testAuras("formation", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(cmpAuras.HasFormationAura(), true);
	cmpAuras.ApplyFormationBonus([targetEnt]);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	cmpAuras.RemoveFormationBonus([targetEnt]);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 5);
});

playerState = "defeated";
testAuras("global", (name, cmpAuras) => {
  TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[1], template), 5);
});
