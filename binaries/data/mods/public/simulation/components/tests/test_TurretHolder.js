Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/TurretHolder.js");
Engine.LoadComponentScript("interfaces/Turretable.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("TurretHolder.js");
Engine.LoadComponentScript("Turretable.js");

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => id
});

const player = 1;
const enemyPlayer = 2;
const alliedPlayer = 3;
const turretHolderID = 9;
const entitiesToTest = [10, 11, 12, 13];

AddMock(turretHolderID, IID_Ownership, {
	"GetOwner": () => player
});
AddMock(turretHolderID, IID_Position, {
	"GetPosition": () => new Vector3D(4, 3, 25),
	"GetRotation": () => new Vector3D(4, 0, 6),
	"IsInWorld": () => true
});

for (let entity of entitiesToTest)
{
	AddMock(entity, IID_Position, {
		"GetPosition": () => new Vector3D(4, 3, 25),
		"GetRotation": () => new Vector3D(4, 0, 6),
		"SetTurretParent": (parent, offset) => {},
		"IsInWorld": () => true
	});

	AddMock(entity, IID_Ownership, {
		"GetOwner": () => player
	});
}

AddMock(player, IID_Player, {
	"IsAlly": id => id != enemyPlayer,
	"IsMutualAlly": id => id != enemyPlayer,
	"GetPlayerID": () => player
});

AddMock(alliedPlayer, IID_Player, {
	"IsAlly": id => true,
	"IsMutualAlly": id => true,
	"GetPlayerID": () => alliedPlayer
});

let cmpTurretHolder = ConstructComponent(turretHolderID, "TurretHolder", {
	"TurretPoints": {
		"archer1": {
			"X": "12.0",
			"Y": "5.",
			"Z": "6.0"
		},
		"archer2": {
			"X": "15.0",
			"Y": "5.0",
			"Z": "6.0",
			"AllowedClasses": { "_string": "Siege Trader" }
		},
		"archer3": {
			"X": "15.0",
			"Y": "5.0",
			"Z": "6.0",
			"AllowedClasses": { "_string": "Siege Infantry+Ranged Infantry+Cavalry" }
		}
	}
});

let siegeEngineID = entitiesToTest[0];
AddMock(siegeEngineID, IID_Identity, {
	"GetClassesList": () => ["Siege"]
});

let archerID = entitiesToTest[1];
AddMock(archerID, IID_Identity, {
	"GetClassesList": () => ["Infantry", "Ranged"]
});

let cavID = entitiesToTest[2];
AddMock(cavID, IID_Identity, {
	"GetClassesList": () => ["Infantry", "Cavalry"]
});

let infID = entitiesToTest[3];
AddMock(infID, IID_Identity, {
	"GetClassesList": () => ["Infantry"]
});

// Test visible garrisoning restrictions.
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(siegeEngineID, cmpTurretHolder.turretPoints[0]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(siegeEngineID, cmpTurretHolder.turretPoints[1]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(siegeEngineID, cmpTurretHolder.turretPoints[2]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(archerID, cmpTurretHolder.turretPoints[0]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(archerID, cmpTurretHolder.turretPoints[1]), false);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(archerID, cmpTurretHolder.turretPoints[2]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(cavID, cmpTurretHolder.turretPoints[0]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(cavID, cmpTurretHolder.turretPoints[1]), false);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(cavID, cmpTurretHolder.turretPoints[2]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(infID, cmpTurretHolder.turretPoints[0]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(infID, cmpTurretHolder.turretPoints[1]), false);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurretPoint(infID, cmpTurretHolder.turretPoints[2]), false);

// Test that one cannot leave a turret that is not occupied.
TS_ASSERT(!cmpTurretHolder.LeaveTurretPoint(archerID));

// Test occupying a turret.
TS_ASSERT(!cmpTurretHolder.OccupiesTurretPoint(archerID));
TS_ASSERT(cmpTurretHolder.OccupyTurretPoint(archerID));
TS_ASSERT(cmpTurretHolder.OccupiesTurretPoint(archerID));

// We're not occupying a turret that we can't occupy.
TS_ASSERT(!cmpTurretHolder.OccupiesTurretPoint(archerID, cmpTurretHolder.turretPoints[1]));
TS_ASSERT(!cmpTurretHolder.OccupyTurretPoint(cavID, cmpTurretHolder.turretPoints[1]));
TS_ASSERT(!cmpTurretHolder.OccupyTurretPoint(cavID, cmpTurretHolder.turretPoints[0]));
TS_ASSERT(cmpTurretHolder.OccupyTurretPoint(cavID, cmpTurretHolder.turretPoints[2]));

// Leave turrets.
TS_ASSERT(cmpTurretHolder.LeaveTurretPoint(archerID));
TS_ASSERT(!cmpTurretHolder.LeaveTurretPoint(cavID, false, cmpTurretHolder.turretPoints[1]));
TS_ASSERT(cmpTurretHolder.LeaveTurretPoint(cavID, false, cmpTurretHolder.turretPoints[2]));

// Incremental Turret creation.
cmpTurretHolder = ConstructComponent(turretHolderID, "TurretHolder", {
	"TurretPoints": {
		"Turret": {
			"X": "15.0",
			"Y": "5.0",
			"Z": "6.0",
			"Template": "units/iber/cavalry_javelineer_c"
		}
	}
});

let spawned = 100;
Engine.AddEntity = function() {
	++spawned;
	if(spawned > 101)
	{
		ConstructComponent(spawned, "Turretable", {});
	}
	if(spawned > 102)
	{
		AddMock(spawned, IID_Ownership, {
			"GetOwner": () => player,
			"SetOwner": () => {}
		});
	}
	if(spawned > 103)
	{
		AddMock(spawned, IID_Position, {
			"GetPosition": () => new Vector3D(4, 3, 25),
			"GetRotation": () => new Vector3D(4, 0, 6),
			"SetTurretParent": () => {},
			"IsInWorld": () => true
		});
	}
	return spawned;
}

const GetUpgradedTemplate = (_, template)  => template === "units/iber/cavalry_javelineer_b" ? "units/iber/cavalry_javelineer_a" : template;
Engine.RegisterGlobal("GetUpgradedTemplate", GetUpgradedTemplate);
cmpTurretHolder.OnOwnershipChanged({
	"to": 1,
	"from": INVALID_PLAYER
});
TS_ASSERT(!cmpTurretHolder.OccupiesTurretPoint(spawned));
cmpTurretHolder.OnOwnershipChanged({
	"to": 1,
	"from": INVALID_PLAYER
});
TS_ASSERT(!cmpTurretHolder.OccupiesTurretPoint(spawned));
cmpTurretHolder.OnOwnershipChanged({
	"to": 1,
	"from": INVALID_PLAYER
});
TS_ASSERT(!cmpTurretHolder.OccupiesTurretPoint(spawned));
cmpTurretHolder.OnOwnershipChanged({
	"to": 1,
	"from": INVALID_PLAYER
});
TS_ASSERT(cmpTurretHolder.OccupiesTurretPoint(spawned));

// Normal turret creation.
Engine.AddEntity = function(t) {
	++spawned;
	// Check that we're using the upgraded template.
	TS_ASSERT(t, "units/iber/cavalry_javelineer_a");
	ConstructComponent(spawned, "Turretable", {});
	AddMock(spawned, IID_Ownership, {
		"GetOwner": () => player,
		"SetOwner": () => {}
	});
	AddMock(spawned, IID_Position, {
		"GetPosition": () => new Vector3D(4, 3, 25),
		"GetRotation": () => new Vector3D(4, 0, 6),
		"SetTurretParent": () => {},
		"IsInWorld": () => true
	});
	return spawned;
}

cmpTurretHolder = ConstructComponent(turretHolderID, "TurretHolder", {
	"TurretPoints": {
		"Turret": {
			"X": "15.0",
			"Y": "5.0",
			"Z": "6.0",
			"Template": "units/iber/cavalry_javelineer_b"
		}
	}
});

cmpTurretHolder.OnOwnershipChanged({
	"to": 1,
	"from": INVALID_PLAYER
});
TS_ASSERT(cmpTurretHolder.OccupiesTurretPoint(spawned));
