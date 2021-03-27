Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/TurretHolder.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("TurretHolder.js");

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
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(siegeEngineID, cmpTurretHolder.turretPoints[0]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(siegeEngineID, cmpTurretHolder.turretPoints[1]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(siegeEngineID, cmpTurretHolder.turretPoints[2]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(archerID, cmpTurretHolder.turretPoints[0]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(archerID, cmpTurretHolder.turretPoints[1]), false);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(archerID, cmpTurretHolder.turretPoints[2]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(cavID, cmpTurretHolder.turretPoints[0]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(cavID, cmpTurretHolder.turretPoints[1]), false);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(cavID, cmpTurretHolder.turretPoints[2]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(infID, cmpTurretHolder.turretPoints[0]), true);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(infID, cmpTurretHolder.turretPoints[1]), false);
TS_ASSERT_EQUALS(cmpTurretHolder.AllowedToOccupyTurret(infID, cmpTurretHolder.turretPoints[2]), false);

// Test that one cannot leave a turret that is not occupied.
TS_ASSERT(!cmpTurretHolder.LeaveTurret(archerID));

// Test occupying a turret.
TS_ASSERT(!cmpTurretHolder.OccupiesTurret(archerID));
TS_ASSERT(cmpTurretHolder.OccupyTurret(archerID));
TS_ASSERT(cmpTurretHolder.OccupiesTurret(archerID));

// We're not occupying a turret that we can't occupy.
TS_ASSERT(!cmpTurretHolder.OccupiesTurret(archerID, cmpTurretHolder.turretPoints[1]));
TS_ASSERT(!cmpTurretHolder.OccupyTurret(cavID, cmpTurretHolder.turretPoints[1]));
TS_ASSERT(!cmpTurretHolder.OccupyTurret(cavID, cmpTurretHolder.turretPoints[0]));
TS_ASSERT(cmpTurretHolder.OccupyTurret(cavID, cmpTurretHolder.turretPoints[2]));

// Leave turrets.
TS_ASSERT(cmpTurretHolder.LeaveTurret(archerID));
TS_ASSERT(!cmpTurretHolder.LeaveTurret(cavID, cmpTurretHolder.turretPoints[1]));
TS_ASSERT(cmpTurretHolder.LeaveTurret(cavID, cmpTurretHolder.turretPoints[2]));
