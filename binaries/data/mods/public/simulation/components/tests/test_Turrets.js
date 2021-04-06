Engine.LoadHelperScript("ValueModification.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("Position.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/Turretable.js");
Engine.LoadComponentScript("interfaces/TurretHolder.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Turretable.js");
Engine.LoadComponentScript("TurretHolder.js");

const player = 1;
const enemyPlayer = 2;
const friendlyPlayer = 3;
const turret = 10;
const holder = 11;

let createTurretCmp = entity => {
	AddMock(entity, IID_Identity, {
		"GetClassesList": () => ["Ranged"],
		"GetSelectionGroupName": () => "mace_infantry_archer_a"
	});

	AddMock(entity, IID_Ownership, {
		"GetOwner": () => player
	});

	AddMock(entity, IID_Position, {
		"GetHeightOffset": () => 0,
		"GetPosition": () => new Vector3D(4, 3, 25),
		"GetRotation": () => new Vector3D(4, 0, 6),
		"JumpTo": (posX, posZ) => {},
		"MoveOutOfWorld": () => {},
		"SetHeightOffset": height => {},
		"SetTurretParent": entity => {},
		"SetYRotation": angle => {}
	});

	return ConstructComponent(entity, "Turretable", null);
};

AddMock(holder, IID_Footprint, {
	"PickSpawnPointBothPass": entity => new Vector3D(4, 3, 30),
	"PickSpawnPoint": entity => new Vector3D(4, 3, 30)
});

AddMock(holder, IID_Ownership, {
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

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => id
});

AddMock(holder, IID_Position, {
	"GetPosition": () => new Vector3D(4, 3, 25),
	"GetRotation": () => new Vector3D(4, 0, 6)
});

let cmpTurretable = createTurretCmp(turret);

let cmpTurretHolder = ConstructComponent(holder, "TurretHolder", {
	"TurretPoints": {
		"archer1": {
			"X": "12.0",
			"Y": "5.",
			"Z": "6.0"
		},
		"archer2": {
			"X": "15.0",
			"Y": "5.0",
			"Z": "6.0"
		}
	}
});

TS_ASSERT(cmpTurretable.OccupyTurret(holder));
TS_ASSERT_UNEVAL_EQUALS(cmpTurretHolder.GetEntities(), [turret]);
TS_ASSERT(cmpTurretHolder.OccupiesTurretPoint(turret));
TS_ASSERT(cmpTurretable.LeaveTurret());
TS_ASSERT_UNEVAL_EQUALS(cmpTurretHolder.GetEntities(), []);

// Test renaming on a turret.
// Ensure we test renaming from the second spot, not the first.
const newTurret = 31;
let cmpTurretableNew = createTurretCmp(newTurret);
TS_ASSERT(cmpTurretableNew.OccupyTurret(holder));
TS_ASSERT(cmpTurretable.OccupyTurret(holder));
TS_ASSERT(cmpTurretableNew.LeaveTurret());
let previousTurret = cmpTurretHolder.GetOccupiedTurretPointName(turret);
cmpTurretable.OnEntityRenamed({
	"entity": turret,
	"newentity": newTurret
});
let newTurretPos = cmpTurretHolder.GetOccupiedTurretPointName(newTurret);
TS_ASSERT_UNEVAL_EQUALS(newTurretPos, previousTurret);
TS_ASSERT(cmpTurretableNew.LeaveTurret());

// Test initTurrets.
cmpTurretHolder.SetInitEntity("archer1", turret);
cmpTurretHolder.SetInitEntity("archer2", newTurret);
cmpTurretHolder.OnGlobalInitGame();
TS_ASSERT_UNEVAL_EQUALS(cmpTurretHolder.GetEntities(), [turret, newTurret]);
