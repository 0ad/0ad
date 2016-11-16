Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("Auras.js");
Engine.LoadComponentScript("AuraManager.js");

let playerID = 1;
let playerEnt = 10;
let auraEnt = 20;
let targetEnt = 30;
let auraRange = 40;
let template = { "Identity" : { "Classes" : { "_string" : "CorrectClass OtherClass" } } };

function testAuras(name, test_function)
{
	ResetState();

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": () => playerEnt,
		"GetNumPlayers": () => 2,
	});

	AddMock(SYSTEM_ENTITY, IID_RangeManager, {
		"CreateActiveQuery": (ent, minRange, maxRange, players, iid, flags) => 1,
		"EnableActiveQuery": id => {},
		"ResetActiveQuery": id => {},
		"DisableActiveQuery": id => {},
		"DestroyActiveQuery": id => {},
		"GetEntityFlagMask": identifier => {},
	});

	AddMock(SYSTEM_ENTITY, IID_DataTemplateManager, {
		"GetAuraTemplate": (name) => {
			let template = {
				"type": name,
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

	AddMock(playerEnt, IID_Player, {
		"IsAlly": id => id == 1,
		"IsEnemy": id => id != 1,
		"GetPlayerID": () => 1,
	});

	AddMock(targetEnt, IID_Identity, {
		"GetClassesList": () => ["CorrectClass", "OtherClass"],
	});

	AddMock(auraEnt, IID_Position, {
		"GetPosition2D": () => new Vector2D(),
	});

	AddMock(targetEnt, IID_Position, {
		"GetPosition2D": () => new Vector2D(),
	});

	AddMock(auraEnt, IID_Ownership, {
		"GetOwner": () => 1,
	});

	ConstructComponent(SYSTEM_ENTITY, "AuraManager", {});
	let cmpAuras = ConstructComponent(auraEnt, "Auras", { "_string": name });
	test_function(name, cmpAuras);
}

testAuras("global", (name, cmpAuras) => {
	cmpAuras.OnRangeUpdate({ "tag": 1, "added": [targetEnt], "removed": [] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 11);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 1, playerID, template), 11);
});

testAuras("range", (name, cmpAuras) => {
	cmpAuras.OnRangeUpdate({ "tag": 1, "added": [targetEnt], "removed": [] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 11);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 1, playerID, template), 1);
	cmpAuras.OnRangeUpdate({ "tag": 1, "added": [], "removed": [targetEnt] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 1);
});

testAuras("garrisonedUnits", (name, cmpAuras) => {
	cmpAuras.OnGarrisonedUnitsChanged({ "added" : [targetEnt], "removed": [] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 11);
	cmpAuras.OnGarrisonedUnitsChanged({ "added" : [], "removed": [targetEnt] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 1);
});

testAuras("garrison", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(cmpAuras.HasGarrisonAura(), true);
	cmpAuras.ApplyGarrisonBonus(targetEnt);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 11);
	cmpAuras.RemoveGarrisonBonus(targetEnt);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 1);
});

testAuras("formation", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(cmpAuras.HasFormationAura(), true);
	cmpAuras.ApplyFormationBonus([targetEnt]);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 11);
	cmpAuras.RemoveFormationBonus([targetEnt]);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 1);
});
