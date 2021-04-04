Engine.LoadHelperScript("ValueModification.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Garrisonable.js");
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("Garrisonable.js");
Engine.LoadComponentScript("GarrisonHolder.js");

const garrisonedEntitiesList = [25, 26, 27, 28, 29, 30, 31, 32, 33];
const garrisonHolderId = 15;
const unitToGarrisonId = 24;
const enemyUnitId = 34;
const largeUnitId = 35;
const player = 1;
const friendlyPlayer = 2;
const enemyPlayer = 3;

let cmpGarrisonHolder = ConstructComponent(garrisonHolderId, "GarrisonHolder", {
	"Max": "10",
	"List": { "_string": "Infantry+Cavalry" },
	"EjectHealth": "0.1",
	"EjectClassesOnDestroy": { "_string": "Infantry" },
	"BuffHeal": "1",
	"LoadingRange": "2.1",
	"Pickup": false
});

AddMock(garrisonHolderId, IID_Ownership, {
	"GetOwner": () => player
});

AddMock(player, IID_Player, {
	"IsAlly": id => id != enemyPlayer,
	"IsMutualAlly": id => id != enemyPlayer,
	"GetPlayerID": () => player
});

AddMock(friendlyPlayer, IID_Player, {
	"IsAlly": id => true,
	"IsMutualAlly": id => true,
	"GetPlayerID": () => friendlyPlayer
});

AddMock(SYSTEM_ENTITY, IID_Timer, {
	"SetInterval": (ent, iid, funcname, time, data) => 1
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => id
});

for (let i = 24; i <= 35; ++i)
{
	AddMock(i, IID_Identity, {
		"GetClassesList": () => ["Infantry", "Cavalry"],
		"GetSelectionGroupName": () => "mace_infantry_archer_a"
	});

	if (i < 28)
		AddMock(i, IID_Ownership, {
			"GetOwner": () => player
		});
	else if (i == 34)
		AddMock(i, IID_Ownership, {
			"GetOwner": () => enemyPlayer
		});
	else
		AddMock(i, IID_Ownership, {
			"GetOwner": () => friendlyPlayer
		});

	if (i == largeUnitId)
		AddMock(i, IID_Garrisonable, {
			"UnitSize": () => 9,
			"TotalSize": () => 9,
			"Garrison": (entity) => cmpGarrisonHolder.Garrison(i),
			"UnGarrison": () => cmpGarrisonHolder.Eject(i)
		});
	else
		AddMock(i, IID_Garrisonable, {
			"UnitSize": () => 1,
			"TotalSize": () => 1,
			"Garrison": entity => cmpGarrisonHolder.Garrison(i),
			"UnGarrison": () => cmpGarrisonHolder.Eject(i)
		});

	AddMock(i, IID_Position, {
		"GetHeightOffset": () => 0,
		"GetPosition": () => new Vector3D(4, 3, 25),
		"GetRotation": () => new Vector3D(4, 0, 6),
		"JumpTo": (posX, posZ) => {},
		"MoveOutOfWorld": () => {},
		"SetHeightOffset": height => {}
	});
}

AddMock(33, IID_Identity, {
	"GetClassesList": () => ["Infantry", "Cavalry"],
	"GetSelectionGroupName": () => "spart_infantry_archer_a"
});

let testGarrisonAllowed = function()
{
	TS_ASSERT_EQUALS(cmpGarrisonHolder.HasEnoughHealth(), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(enemyUnitId), false);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(unitToGarrisonId), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(largeUnitId), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.IsFull(), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Unload(largeUnitId), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Unload(unitToGarrisonId), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(unitToGarrisonId), true);
	for (let entity of garrisonedEntitiesList)
		TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(entity), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(largeUnitId), false);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.IsFull(), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.CanPickup(unitToGarrisonId), false);

	TS_ASSERT_EQUALS(cmpGarrisonHolder.UnloadTemplate("spart_infantry_archer_a", 2, false), true);
	TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [24, 25, 26, 27, 28, 29, 30, 31, 32]);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.UnloadAllByOwner(friendlyPlayer), true);
	TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [24, 25, 26, 27]);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 4);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.IsEjectable(25), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Unload(25), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.IsEjectable(25), false);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Unload(25), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Eject(null, false), true);
	TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [24, 26, 27]);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 3);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.IsFull(), false);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(largeUnitId), false);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.UnloadAll(), true);
	TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), []);
};

// No health component yet.Pick
testGarrisonAllowed();

AddMock(garrisonHolderId, IID_Health, {
	"GetHitpoints": () => 50,
	"GetMaxHitpoints": () => 600
});

cmpGarrisonHolder.AllowGarrisoning(true, "callerID1");
cmpGarrisonHolder.AllowGarrisoning(false, 5);
TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(unitToGarrisonId), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.Unload(unitToGarrisonId), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.IsGarrisoningAllowed(), false);

cmpGarrisonHolder.AllowGarrisoning(true, 5);
TS_ASSERT_EQUALS(cmpGarrisonHolder.IsGarrisoningAllowed(), true);

TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.LoadingRange(), { "max": 2.1, "min": 0 });
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), []);
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetHealRate(), 1);
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetAllowedClasses(), "Infantry+Cavalry");
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetCapacity(), 10);
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 0);
TS_ASSERT_EQUALS(cmpGarrisonHolder.CanPickup(unitToGarrisonId), false);

TS_ASSERT_EQUALS(cmpGarrisonHolder.CanPickup(enemyUnitId), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.IsFull(), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.IsAllowedToGarrison(enemyUnitId), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.IsAllowedToGarrison(largeUnitId), true);
TS_ASSERT_EQUALS(cmpGarrisonHolder.IsAllowedToGarrison(unitToGarrisonId), true);
TS_ASSERT_EQUALS(cmpGarrisonHolder.HasEnoughHealth(), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(unitToGarrisonId), false);

AddMock(garrisonHolderId, IID_Health, {
	"GetHitpoints": () => 600,
	"GetMaxHitpoints": () => 600
});

// No eject health.
cmpGarrisonHolder = ConstructComponent(garrisonHolderId, "GarrisonHolder", {
	"Max": 10,
	"List": { "_string": "Infantry+Cavalry" },
	"EjectClassesOnDestroy": { "_string": "Infantry" },
	"BuffHeal": 1,
	"LoadingRange": 2.1,
	"Pickup": false
});

testGarrisonAllowed();

// Test entity renaming.
let siegeEngineId = 44;
AddMock(siegeEngineId, IID_Identity, {
	"GetClassesList": () => ["Siege"]
});
let archerId = 45;
AddMock(archerId, IID_Identity, {
	"GetClassesList": () => ["Infantry", "Ranged"]
});

let originalClassList = "Infantry+Ranged Siege Cavalry";

cmpGarrisonHolder = ConstructComponent(garrisonHolderId, "GarrisonHolder", {
	"Max": 10,
	"List": { "_string": originalClassList },
	"EjectHealth": 0.1,
	"EjectClassesOnDestroy": { "_string": "Infantry" },
	"BuffHeal": 1,
	"LoadingRange": 2.1,
	"Pickup": false
});

let traderId = 32;
AddMock(traderId, IID_Identity, {
	"GetClassesList": () => ["Trader"]
});

AddMock(siegeEngineId, IID_Position, {
	"GetHeightOffset": () => 0,
	"GetPosition": () => new Vector3D(4, 3, 25),
	"GetRotation": () => new Vector3D(4, 0, 6),
	"JumpTo": (posX, posZ) => {},
	"MoveOutOfWorld": () => {},
	"SetHeightOffset": height => {}
});
let currentSiegePlayer = player;
AddMock(siegeEngineId, IID_Ownership, {
	"GetOwner": () => currentSiegePlayer
});
AddMock(siegeEngineId, IID_Garrisonable, {
	"UnitSize": () => 1,
	"TotalSize": () => 1,
	"Garrison": (entity, renamed) => cmpGarrisonHolder.Garrison(siegeEngineId, renamed),
	"UnGarrison": () => true
});
let cavalryId = 46;
AddMock(cavalryId, IID_Identity, {
	"GetClassesList": () => ["Infantry", "Ranged"]
});
AddMock(cavalryId, IID_Position, {
	"GetHeightOffset": () => 0,
	"GetPosition": () => new Vector3D(4, 3, 25),
	"GetRotation": () => new Vector3D(4, 0, 6),
	"JumpTo": (posX, posZ) => {},
	"MoveOutOfWorld": () => {},
	"SetHeightOffset": height => {}
});

let currentCavalryPlayer = player;
AddMock(cavalryId, IID_Ownership, {
	"GetOwner": () => currentCavalryPlayer
});
AddMock(cavalryId, IID_Garrisonable, {
	"UnitSize": () => 1,
	"TotalSize": () => 1,
	"Garrison": (entity, renamed) => cmpGarrisonHolder.Garrison(cavalryId, renamed),
	"UnGarrison": () => true
});
TS_ASSERT(cmpGarrisonHolder.Garrison(cavalryId));
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 1);

// Eject enemy units.
currentCavalryPlayer = enemyPlayer;
cmpGarrisonHolder.OnGlobalOwnershipChanged({
	"entity": cavalryId,
	"to": enemyPlayer
});
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 0);

let oldApplyValueModificationsToEntity = ApplyValueModificationsToEntity;

TS_ASSERT(cmpGarrisonHolder.Garrison(siegeEngineId));
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [siegeEngineId]);

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (valueName, currentValue, entity) => {
	if (valueName !== "GarrisonHolder/List/_string")
		return valueName;

	return HandleTokens(currentValue, "-Siege Trader");
});

cmpGarrisonHolder.OnValueModification({
	"component": "GarrisonHolder",
	"valueNames": ["GarrisonHolder/List/_string"],
	"entities": [garrisonHolderId]
});

TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetAllowedClasses().split(/\s+/), ["Infantry+Ranged", "Cavalry", "Trader"]);

// The new classes are now cached so we can restore the behavior.
Engine.RegisterGlobal("ApplyValueModificationsToEntity", oldApplyValueModificationsToEntity);
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), []);
TS_ASSERT(!cmpGarrisonHolder.Garrison(siegeEngineId));
TS_ASSERT(cmpGarrisonHolder.Garrison(traderId));
