Engine.LoadComponentScript("interfaces/ModifiersManager.js");
Engine.LoadComponentScript("ModifiersManager.js");
Engine.LoadHelperScript("MultiKeyMap.js");
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("ValueModification.js");

let cmpModifiersManager = ConstructComponent(SYSTEM_ENTITY, "ModifiersManager", {});
cmpModifiersManager.Init();

// These should be different as that is the general case.
const PLAYER_ID_FOR_TEST = 2;
const PLAYER_ENTITY_ID = 3;

AddMock(SYSTEM_ENTITY, IID_RangeManager, {
	"GetEntitiesByPlayer": function(a) { return []; }
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": (a) => PLAYER_ENTITY_ID
});

AddMock(PLAYER_ENTITY_ID, IID_Player, {
	"GetPlayerID": () => PLAYER_ID_FOR_TEST
});

let entitiesToTest = [5, 6, 7, 8];
for (let ent of entitiesToTest)
	AddMock(ent, IID_Ownership, {
		"GetOwner": () => PLAYER_ID_FOR_TEST
	});

AddMock(5, IID_Identity, {
	"GetClassesList": function() { return "Structure";}
});
AddMock(6, IID_Identity, {
	"GetClassesList": function() { return "Infantry";}
});
AddMock(7, IID_Identity, {
	"GetClassesList": function() { return "Unit";}
});
AddMock(8, IID_Identity, {
	"GetClassesList": function() { return "Structure Unit";}
});

// Sprinkle random serialisation cycles.
function SerializationCycle()
{
	let data = cmpModifiersManager.Serialize();
	cmpModifiersManager = ConstructComponent(SYSTEM_ENTITY, "ModifiersManager", {});
	cmpModifiersManager.Deserialize(data);
}

cmpModifiersManager.OnGlobalPlayerEntityChanged({ player: PLAYER_ID_FOR_TEST, from: -1, to: PLAYER_ENTITY_ID });

cmpModifiersManager.AddModifier("Test_A", "Test_A_0", { "affects": ["Structure"], "add": 10 }, 10, "testLol");

cmpModifiersManager.AddModifier("Test_A", "Test_A_0", { "affects": ["Structure"], "add": 10 }, PLAYER_ENTITY_ID);
cmpModifiersManager.AddModifier("Test_A", "Test_A_1", { "affects": ["Infantry"], "add": 5 }, PLAYER_ENTITY_ID);
cmpModifiersManager.AddModifier("Test_A", "Test_A_2", { "affects": ["Unit"], "add": 3 }, PLAYER_ENTITY_ID);

TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_A", 5, 5), 15);
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_A", 5, 6), 10);
SerializationCycle();
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_A", 5, 5), 15);
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_A", 5, 6), 10);
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_A", 5, 7), 8);
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_A", 5, 8), 18);

TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_B", 5, 8), 5);

cmpModifiersManager.RemoveAllModifiers("Test_A_0", PLAYER_ENTITY_ID);

TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_A", 5, 5), 5);

cmpModifiersManager.AddModifiers("Test_A_0", {
	"Test_A": { "affects": ["Structure"], "add": 10 },
	"Test_B": { "affects": ["Structure"], "add": 8 },
}, PLAYER_ENTITY_ID);

TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_A", 5, 5), 15);
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_B", 5, 8), 13);

// Add two local modifications, only the first should stick.
cmpModifiersManager.AddModifier("Test_C", "Test_C_0", { "affects": ["Structure"], "add": 10 }, 5);
cmpModifiersManager.AddModifier("Test_C", "Test_C_1", { "affects": ["Unit"], "add": 5 }, 5);

SerializationCycle();

TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_C", 5, 5), 15);

// test that local modifications are indeed applied after global managers
cmpModifiersManager.AddModifier("Test_C", "Test_C_2", { "affects": ["Structure"], "replace": 0 }, 5);
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_C", 5, 5), 0);

TS_ASSERT(!cmpModifiersManager.HasAnyModifier("Test_C_3", PLAYER_ENTITY_ID));

SerializationCycle();

// check that things still work properly if we change global modifications
cmpModifiersManager.AddModifier("Test_C", "Test_C_3", { "affects": ["Structure"], "add": 10 }, PLAYER_ENTITY_ID);
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_C", 5, 5), 0);

TS_ASSERT(cmpModifiersManager.HasAnyModifier("Test_C_3", PLAYER_ENTITY_ID));
TS_ASSERT(cmpModifiersManager.HasModifier("Test_C", "Test_C_3", PLAYER_ENTITY_ID));
TS_ASSERT(cmpModifiersManager.HasModifier("Test_C", "Test_C_2", 5));

// test removal
cmpModifiersManager.RemoveModifier("Test_C", "Test_C_2", 5);
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_C", 5, 5), 25);

SerializationCycle();

TS_ASSERT(cmpModifiersManager.HasModifier("Test_C", "Test_C_3", PLAYER_ENTITY_ID));
TS_ASSERT(!cmpModifiersManager.HasModifier("Test_C", "Test_C_2", 5));

//////////////////////////////////////////
// Test that entities keep local modifications but not global ones when changing owner.
AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": (a) => a == PLAYER_ID_FOR_TEST ? PLAYER_ENTITY_ID : PLAYER_ENTITY_ID + 1
});

AddMock(PLAYER_ENTITY_ID + 1, IID_Player, {
	"GetPlayerID": () => PLAYER_ID_FOR_TEST + 1
});

cmpModifiersManager = ConstructComponent(SYSTEM_ENTITY, "ModifiersManager", {});
cmpModifiersManager.Init();

cmpModifiersManager.AddModifier("Test_D", "Test_D_0", { "affects": ["Structure"], "add": 10 }, PLAYER_ENTITY_ID);
cmpModifiersManager.AddModifier("Test_D", "Test_D_1", { "affects": ["Structure"], "add": 1 }, PLAYER_ENTITY_ID + 1);
cmpModifiersManager.AddModifier("Test_D", "Test_D_2", { "affects": ["Structure"], "add": 5 }, 5);

cmpModifiersManager.OnGlobalPlayerEntityChanged({ player: PLAYER_ID_FOR_TEST, from: -1, to: PLAYER_ENTITY_ID });
cmpModifiersManager.OnGlobalPlayerEntityChanged({ player: PLAYER_ID_FOR_TEST + 1, from: -1, to: PLAYER_ENTITY_ID + 1 });

TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_D", 10, 5), 25);
cmpModifiersManager.OnGlobalOwnershipChanged({ entity: 5, from: PLAYER_ID_FOR_TEST, to: PLAYER_ID_FOR_TEST + 1 });
AddMock(5, IID_Ownership, {
	"GetOwner": () => PLAYER_ID_FOR_TEST + 1
});
TS_ASSERT_EQUALS(ApplyValueModificationsToEntity("Test_D", 10, 5), 16);
