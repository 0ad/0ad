Engine.LoadHelperScript("ValueModification.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("Position.js");
Engine.LoadComponentScript("interfaces/Garrisonable.js");
Engine.LoadComponentScript("interfaces/GarrisonHolder.js");
Engine.LoadComponentScript("interfaces/Health.js");
Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Garrisonable.js");
Engine.LoadComponentScript("GarrisonHolder.js");

const player = 1;
const enemyPlayer = 2;
const friendlyPlayer = 3;
const garrison = 10;
const holder = 11;

let createGarrisonCmp = entity => {
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
		"SetYRotation": angle => {}
	});

	return ConstructComponent(entity, "Garrisonable", {
		"Size": "1"
	});
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

AddMock(SYSTEM_ENTITY, IID_Timer, {
	"SetInterval": (ent, iid, funcname, time, data) => 1
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => id
});

AddMock(garrison, IID_Identity, {
	"GetClassesList": () => ["Ranged"],
	"GetSelectionGroupName": () => "mace_infantry_archer_a"
});

AddMock(garrison, IID_Ownership, {
	"GetOwner": () => player
});

AddMock(garrison, IID_Position, {
	"GetHeightOffset": () => 0,
	"GetPosition": () => new Vector3D(4, 3, 25),
	"GetRotation": () => new Vector3D(4, 0, 6),
	"JumpTo": (posX, posZ) => {},
	"MoveOutOfWorld": () => {},
	"SetHeightOffset": height => {},
	"SetYRotation": angle => {}
});

let cmpGarrisonable = ConstructComponent(garrison, "Garrisonable", {
	"Size": "1"
});

let cmpGarrisonHolder = ConstructComponent(holder, "GarrisonHolder", {
	"Max": "10",
	"List": { "_string": "Ranged" },
	"EjectHealth": "0.1",
	"EjectClassesOnDestroy": { "_string": "Infantry" },
	"BuffHeal": "1",
	"LoadingRange": "2.1",
	"Pickup": "false"
});

TS_ASSERT(cmpGarrisonable.Garrison(holder));
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [garrison]);

cmpGarrisonable.OnEntityRenamed({
	"entity": garrison,
	"newentity": -1
});
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 0);

TS_ASSERT(cmpGarrisonable.Garrison(holder));
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [garrison]);

// Can't garrison twice.
TS_ASSERT(!cmpGarrisonable.Garrison(holder));
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), [garrison]);

TS_ASSERT(cmpGarrisonHolder.Unload(garrison));
TS_ASSERT_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), 0);

// Test initGarrison.
let entities = [21, 22, 23, 24];
for (let entity of entities)
	createGarrisonCmp(entity);
cmpGarrisonHolder.SetInitGarrison(entities);
cmpGarrisonHolder.OnGlobalInitGame();
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), entities);

// They turned against us!
AddMock(entities[0], IID_Ownership, {
	"GetOwner": () => enemyPlayer
});
cmpGarrisonHolder.OnDiplomacyChanged();
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetGarrisonedEntitiesCount(), entities.length - 1);

TS_ASSERT(cmpGarrisonHolder.UnloadAll());
TS_ASSERT_UNEVAL_EQUALS(cmpGarrisonHolder.GetEntities(), []);

