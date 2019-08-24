Engine.LoadHelperScript("MultiKeyMap.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/RangeOverlayManager.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("Auras.js");
Engine.LoadComponentScript("ModifiersManager.js");

var playerID = [0, 1, 2];
var playerEnt = [10, 11, 12];
var playerState = ["active", "active", "active"];
var sourceEnt = 20;
var targetEnt = 30;
var auraRange = 40;
var template = { "Identity": { "Classes": { "_string": "CorrectClass OtherClass" } } };

global.AuraTemplates = {
	"Get": name => {
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
};

function testAuras(name, test_function)
{
	ResetState();

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": idx => playerEnt[idx],
		"GetNumPlayers": () => 3,
		"GetAllPlayers": () => playerID
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

	AddMock(playerEnt[1], IID_Player, {
		"IsAlly": id => id == playerID[1] || id == playerID[2],
		"IsEnemy": id => id != playerID[1] || id != playerID[2],
		"GetPlayerID": () => playerID[1],
		"GetState": () => playerState[1]
	});

	AddMock(playerEnt[2], IID_Player, {
		"IsAlly": id => id == playerID[1] || id == playerID[2],
		"IsEnemy": id => id != playerID[1] || id != playerID[2],
		"GetPlayerID": () => playerID[2],
		"GetState": () => playerState[2]
	});

	AddMock(targetEnt, IID_Identity, {
		"GetClassesList": () => ["CorrectClass", "OtherClass"]
	});

	AddMock(sourceEnt, IID_Position, {
		"GetPosition2D": () => new Vector2D()
	});

	if (name != "player" || playerEnt.indexOf(targetEnt) == -1)
	{
		AddMock(targetEnt, IID_Position, {
			"GetPosition2D": () => new Vector2D()
		});

		AddMock(targetEnt, IID_Ownership, {
			"GetOwner": () => playerID[1]
		});
	}

	if (playerEnt.indexOf(sourceEnt) == -1)
		AddMock(sourceEnt, IID_Ownership, {
			"GetOwner": () => playerID[1]
		});

	let cmpModifiersManager = ConstructComponent(SYSTEM_ENTITY, "ModifiersManager", {});
	cmpModifiersManager.OnGlobalPlayerEntityChanged({ player: playerID[1], from: -1, to: playerEnt[1] });
	cmpModifiersManager.OnGlobalPlayerEntityChanged({ player: playerID[2], from: -1, to: playerEnt[2] });
	let cmpAuras = ConstructComponent(sourceEnt, "Auras", { "_string": name });
	test_function(name, cmpAuras);
}

targetEnt = playerEnt[playerID[2]];
testAuras("player", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
});
targetEnt = 30;

// Test the case when the aura source is a player entity.
sourceEnt = 11;
testAuras("global", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[1], template), 15);
});
sourceEnt = 20;

testAuras("range", (name, cmpAuras) => {
	cmpAuras.OnRangeUpdate({ "tag": 1, "added": [targetEnt], "removed": [] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[1], template), 5);
	cmpAuras.OnRangeUpdate({ "tag": 1, "added": [], "removed": [targetEnt] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 5);
});

testAuras("garrisonedUnits", (name, cmpAuras) => {
	cmpAuras.OnGarrisonedUnitsChanged({ "added": [targetEnt], "removed": [] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	cmpAuras.OnGarrisonedUnitsChanged({ "added": [], "removed": [targetEnt] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 5);
});

testAuras("garrison", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(cmpAuras.HasGarrisonAura(), true);
	cmpAuras.ApplyGarrisonAura(targetEnt);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	cmpAuras.RemoveGarrisonAura(targetEnt);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 5);
});

testAuras("formation", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(cmpAuras.HasFormationAura(), true);
	cmpAuras.ApplyFormationAura([targetEnt]);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	cmpAuras.RemoveFormationAura([targetEnt]);
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 5);
});

testAuras("global", (name, cmpAuras) => {
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 15);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[1], template), 15);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[2], template), 15);
	AddMock(sourceEnt, IID_Ownership, {
		"GetOwner": () => -1
	});
	cmpAuras.OnOwnershipChanged({ "from": sourceEnt, "to": -1 });
	TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 5, targetEnt), 5);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[1], template), 5);
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[2], template), 5);
});

playerState[1] = "defeated";
testAuras("global", (name, cmpAuras) => {
	cmpAuras.OnGlobalPlayerDefeated({ "playerId": playerID[1] });
	TS_ASSERT_EQUALS(ApplyValueModificationsToTemplate("Component/Value", 5, playerID[2], template), 5);
});
