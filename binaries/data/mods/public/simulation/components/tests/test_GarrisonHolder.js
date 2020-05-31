Engine.LoadHelperScript("ValueModification.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/Garrisonable.js");
Engine.LoadComponentScript("GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/Auras.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/ProductionQueue.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");

const garrisonedEntitiesList = [25, 26, 27, 28, 29, 30, 31, 32, 33];
const garrisonHolderId = 15;
const unitToGarrisonId = 24;
const enemyUnitId = 34;
const player = 1;
const friendlyPlayer = 2;
const enemyPlayer = 3;

AddMock(garrisonHolderId, IID_Footprint, {
	"PickSpawnPointBothPass": entity => new Vector3D(4, 3, 30),
	"PickSpawnPoint": entity => new Vector3D(4, 3, 30)
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
	"SetTimeout": (ent, iid, funcname, time, data) => 1
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => id
});

for (let i = 24; i <= 34; ++i)
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

	AddMock(i, IID_Garrisonable, {});

	AddMock(i, IID_Position, {
		"GetHeightOffset": () => 0,
		"GetPosition": () => new Vector3D(4, 3, 25),
		"GetRotation": () => new Vector3D(4, 0, 6),
		"GetTurretParent": () => INVALID_ENTITY,
		"IsInWorld": () => true,
		"JumpTo": (posX, posZ) => {},
		"MoveOutOfWorld": () => {},
		"SetTurretParent": (entity, offset) => {},
		"SetHeightOffset": height => {}
	});
}

AddMock(33, IID_Identity, {
	"GetClassesList": () => ["Infantry", "Cavalry"],
	"GetSelectionGroupName": () => "spart_infantry_archer_a"
});

let cmpGarrisonHolder = ConstructComponent(garrisonHolderId, "GarrisonHolder", {
	"Max": 10,
	"List": { "_string": "Infantry+Cavalry" },
	"EjectHealth": 0.1,
	"EjectClassesOnDestroy": { "_string": "Infantry" },
	"BuffHeal": 1,
	"LoadingRange": 2.1,
	"Pickup": false,
	"VisibleGarrisonPoints": {
		"archer1": {
			"X": 12,
			"Y": 5,
			"Z": 6
		},
		"archer2": {
			"X": 15,
			"Y": 5,
			"Z": 6
		}
	}
});

let testGarrisonAllowed = function()
{
	TS_ASSERT_EQUALS(cmpGarrisonHolder.HasEnoughHealth(), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(enemyUnitId), false);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(unitToGarrisonId), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Eject(unitToGarrisonId), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(unitToGarrisonId), true);
	for (let entity of garrisonedEntitiesList)
		TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(entity), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.IsFull(), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.CanPickup(unitToGarrisonId), false);

	TS_ASSERT_EQUALS(cmpGarrisonHolder.UnloadTemplate("spart_infantry_archer_a", 2, false, false), true);
	TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [24, 25, 26, 27, 28, 29, 30, 31, 32]);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.UnloadAllByOwner(friendlyPlayer, false), true);
	TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [24, 25, 26, 27]);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 4);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.IsEjectable(25), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.Unload(25), true);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.IsEjectable(25), false);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.PerformEject([25], false), false);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.PerformEject([], false), true);
	TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [24, 26, 27]);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 3);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.IsFull(), false);
	TS_ASSERT_EQUALS(cmpGarrisonHolder.UnloadAll(), true);
	TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), []);
};

// No health component yet.
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

TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetLoadingRange(), { "max": 2.1, "min": 0 });
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), []);
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetHealRate(), 1);
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetAllowedClasses(), "Infantry+Cavalry");
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetCapacity(), 10);
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 0);
TS_ASSERT_EQUALS(cmpGarrisonHolder.CanPickup(unitToGarrisonId), false);

TS_ASSERT_EQUALS(cmpGarrisonHolder.CanPickup(enemyUnitId), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.IsFull(), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.IsAllowedToGarrison(enemyUnitId), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.IsAllowedToGarrison(unitToGarrisonId), true);
TS_ASSERT_EQUALS(cmpGarrisonHolder.HasEnoughHealth(), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.PerformGarrison(unitToGarrisonId), false);

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
	"Pickup": false,
	"VisibleGarrisonPoints": {
		"archer1": {
			"X": 12,
			"Y": 5,
			"Z": 6
		},
		"archer2": {
			"X": 15,
			"Y": 5,
			"Z": 6
		}
	}
});

testGarrisonAllowed();

let siegeEngineId = 44;
AddMock(siegeEngineId, IID_Identity, {
	"GetClassesList": () => ["Siege"]
});
let archerId = 45;
AddMock(archerId, IID_Identity, {
	"GetClassesList": () => ["Infantry", "Ranged"]
});

// Test visible garrisoning restrictions.
cmpGarrisonHolder = ConstructComponent(garrisonHolderId, "GarrisonHolder", {
	"Max": 10,
	"List": { "_string": "Infantry+Ranged Siege Cavalry" },
	"EjectHealth": 0.1,
	"EjectClassesOnDestroy": { "_string": "Infantry" },
	"BuffHeal": 1,
	"LoadingRange": 2.1,
	"Pickup": false,
	"VisibleGarrisonPoints": {
		"archer1": {
			"X": 12,
			"Y": 5,
			"Z": 6
		},
		"archer2": {
			"X": 15,
			"Y": 5,
			"Z": 6,
			"AllowedClasses": { "_string": "Siege Trader" }
		},
		"archer3": {
			"X": 15,
			"Y": 5,
			"Z": 6,
			"AllowedClasses": { "_string": "Siege Infantry+Ranged Infantry+Cavalry" }
		}
	}
});

AddMock(32, IID_Identity, {
	"GetClassesList": () => ["Trader"]
});

TS_ASSERT_EQUALS(cmpGarrisonHolder.Garrison(32), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.AllowedToVisibleGarrisoning(siegeEngineId, cmpGarrisonHolder.visibleGarrisonPoints[0]), true);
TS_ASSERT_EQUALS(cmpGarrisonHolder.AllowedToVisibleGarrisoning(siegeEngineId, cmpGarrisonHolder.visibleGarrisonPoints[1]), true);
TS_ASSERT_EQUALS(cmpGarrisonHolder.AllowedToVisibleGarrisoning(siegeEngineId, cmpGarrisonHolder.visibleGarrisonPoints[2]), true);
TS_ASSERT_EQUALS(cmpGarrisonHolder.AllowedToVisibleGarrisoning(archerId, cmpGarrisonHolder.visibleGarrisonPoints[0]), true);
TS_ASSERT_EQUALS(cmpGarrisonHolder.AllowedToVisibleGarrisoning(archerId, cmpGarrisonHolder.visibleGarrisonPoints[1]), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.AllowedToVisibleGarrisoning(archerId, cmpGarrisonHolder.visibleGarrisonPoints[2]), true);
TS_ASSERT_EQUALS(cmpGarrisonHolder.AllowedToVisibleGarrisoning(33, cmpGarrisonHolder.visibleGarrisonPoints[0]), true);
TS_ASSERT_EQUALS(cmpGarrisonHolder.AllowedToVisibleGarrisoning(33, cmpGarrisonHolder.visibleGarrisonPoints[1]), false);
TS_ASSERT_EQUALS(cmpGarrisonHolder.AllowedToVisibleGarrisoning(33, cmpGarrisonHolder.visibleGarrisonPoints[2]), true);

// If an entity gets renamed (e.g. promotion, upgrade)
// and is no longer able to be visibly garrisoned it
// should be garisoned instead or ejected.
AddMock(siegeEngineId, IID_Position, {
	"GetHeightOffset": () => 0,
	"GetPosition": () => new Vector3D(4, 3, 25),
	"GetRotation": () => new Vector3D(4, 0, 6),
	"GetTurretParent": () => INVALID_ENTITY,
	"IsInWorld": () => true,
	"JumpTo": (posX, posZ) => {},
	"MoveOutOfWorld": () => {},
	"SetTurretParent": (entity, offset) => {},
	"SetHeightOffset": height => {}
});
let currentSiegePlayer = player;
AddMock(siegeEngineId, IID_Ownership, {
	"GetOwner": () => currentSiegePlayer
});
AddMock(siegeEngineId, IID_Garrisonable, {});
let cavalryId = 46;
AddMock(cavalryId, IID_Identity, {
	"GetClassesList": () => ["Infantry", "Ranged"]
});
AddMock(cavalryId, IID_Position, {
	"GetHeightOffset": () => 0,
	"GetPosition": () => new Vector3D(4, 3, 25),
	"GetRotation": () => new Vector3D(4, 0, 6),
	"GetTurretParent": () => INVALID_ENTITY,
	"IsInWorld": () => true,
	"JumpTo": (posX, posZ) => {},
	"MoveOutOfWorld": () => {},
	"SetTurretParent": (entity, offset) => {},
	"SetHeightOffset": height => {}
});

let currentCavalryPlayer = player;
AddMock(cavalryId, IID_Ownership, {
	"GetOwner": () => currentCavalryPlayer
});
AddMock(cavalryId, IID_Garrisonable, {});
TS_ASSERT(cmpGarrisonHolder.Garrison(siegeEngineId));
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 1);
TS_ASSERT(cmpGarrisonHolder.IsVisiblyGarrisoned(siegeEngineId));
cmpGarrisonHolder.OnGlobalEntityRenamed({
	"entity": siegeEngineId,
	"newentity": cavalryId
});
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 1);
TS_ASSERT(!cmpGarrisonHolder.IsVisiblyGarrisoned(siegeEngineId));
TS_ASSERT(!cmpGarrisonHolder.IsVisiblyGarrisoned(archerId));

// Eject enemy units.
currentCavalryPlayer = enemyPlayer;
cmpGarrisonHolder.OnGlobalOwnershipChanged({
	"entity": cavalryId,
	"to": enemyPlayer
});
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 0);

// Visibly garrisoned units should get ejected if they change players.
TS_ASSERT(cmpGarrisonHolder.Garrison(siegeEngineId));
TS_ASSERT(cmpGarrisonHolder.IsVisiblyGarrisoned(siegeEngineId));
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 1);
currentSiegePlayer = enemyPlayer;
cmpGarrisonHolder.OnGlobalOwnershipChanged({
	"entity": siegeEngineId,
	"to": enemyPlayer
});
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 0);
