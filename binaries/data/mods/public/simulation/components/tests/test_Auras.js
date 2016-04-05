Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/AuraManager.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("Auras.js");
Engine.LoadComponentScript("AuraManager.js");

var playerEnt1 = 5;
var auraEnt = 20;
var targetEnt = 30;

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	GetPlayerByID: function() { return playerEnt1; },
	GetNumPlayers: function() { return 2; },
});

AddMock(SYSTEM_ENTITY, IID_RangeManager, {
	CreateActiveQuery: function(ent, minRange, maxRange, players, iid, flags) {
		return 1;
	},
	EnableActiveQuery: function(id) { },
	ResetActiveQuery: function(id) { if (mode == 0) return []; else return [enemy]; },
	DisableActiveQuery: function(id) { },
	GetEntityFlagMask: function(identifier) { },
});

AddMock(SYSTEM_ENTITY, IID_TechnologyTemplateManager, {
	GetAuraTemplate: function(name) {
		if (name == "test1")
			return {
				"type": "global",
				"affects": ["CorrectClass"],
				"modifications": [ { "value": "Component/Value", "add": 1 } ],
				"auraName": "name",
				"auraDescription": "description"
			};
		return {};
	},
});

AddMock(playerEnt1, IID_Player, {
	IsAlly: function(id) { return id == 1; },
	IsEnemy: function(id) { return id != 1; },
	GetPlayerID: function() { return 1; },
});

AddMock(targetEnt, IID_Identity, {
	GetClassesList: function() { return ["CorrectClass"]; },
});

AddMock(auraEnt, IID_Position, {
	GetPosition2D: function() { return new Vector2D(); },
});

AddMock(targetEnt, IID_Position, {
	// target ent at 20m distance from aura ent
	GetPosition2D: function() { return new Vector2D(); },
});

AddMock(targetEnt, IID_Ownership, {
	GetOwner: function() { return 1; },
});

AddMock(auraEnt, IID_Ownership, {
	GetOwner: function() { return 1; },
});

ConstructComponent(SYSTEM_ENTITY, "AuraManager", {});
var auras = ConstructComponent(auraEnt, "Auras", {_string: "test1"});

// send the rangeManager message
auras.OnRangeUpdate({"tag": 1, "added": [30], "removed": []});

TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Component/Value", 1, targetEnt), 2);
