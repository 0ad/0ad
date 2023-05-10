Engine.RegisterGlobal("Resources", {
	"BuildSchema": (a, b) => {},
	"GetCodes": () => ["food"]
});
Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("Sound.js");
Engine.LoadComponentScript("interfaces/BuildRestrictions.js");
Engine.LoadComponentScript("interfaces/EntityLimits.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/Trainer.js");
Engine.LoadComponentScript("interfaces/TrainingRestrictions.js");
Engine.LoadComponentScript("interfaces/Trigger.js");
Engine.LoadComponentScript("EntityLimits.js");
Engine.LoadComponentScript("Trainer.js");
Engine.LoadComponentScript("TrainingRestrictions.js");

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (_, value) => value);
Engine.RegisterGlobal("ApplyValueModificationsToTemplate", (_, value) => value);

const playerID = 1;
const playerEntityID = 11;
const entityID = 21;

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": () => true,
	"GetTemplate": name => ({})
});

let cmpTrainer = ConstructComponent(entityID, "Trainer", {
	"Entities": { "_string": "units/{civ}/cavalry_javelineer_b " +
	                         "units/{civ}/infantry_swordsman_b " +
	                         "units/{native}/support_female_citizen" }
});
cmpTrainer.GetUpgradedTemplate = (template) => template;

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => playerEntityID
});

AddMock(playerEntityID, IID_Player, {
	"GetDisabledTemplates": () => ({}),
	"GetPlayerID": () => playerID
});

AddMock(playerEntityID, IID_Identity, {
	"GetCiv": () => "iber",
});

AddMock(entityID, IID_Ownership, {
	"GetOwner": () => playerID
});

AddMock(entityID, IID_Identity, {
	"GetCiv": () => "iber"
});

let GetUpgradedTemplate = (_, template) => template === "units/iber/cavalry_javelineer_b" ? "units/iber/cavalry_javelineer_a" : template;
Engine.RegisterGlobal("GetUpgradedTemplate", GetUpgradedTemplate);
cmpTrainer.CalculateEntitiesMap();
TS_ASSERT_UNEVAL_EQUALS(
	cmpTrainer.GetEntitiesList(),
	["units/iber/cavalry_javelineer_a", "units/iber/infantry_swordsman_b", "units/iber/support_female_citizen"]
);

GetUpgradedTemplate = (_, template) => template;
Engine.RegisterGlobal("GetUpgradedTemplate", GetUpgradedTemplate);
cmpTrainer.CalculateEntitiesMap();
TS_ASSERT_UNEVAL_EQUALS(
	cmpTrainer.GetEntitiesList(),
	["units/iber/cavalry_javelineer_b", "units/iber/infantry_swordsman_b", "units/iber/support_female_citizen"]
);

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": name => name == "units/iber/support_female_citizen",
	"GetTemplate": name => ({})
});

cmpTrainer.CalculateEntitiesMap();
TS_ASSERT_UNEVAL_EQUALS(cmpTrainer.GetEntitiesList(), ["units/iber/support_female_citizen"]);

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": () => true,
	"GetTemplate": name => ({})
});

AddMock(playerEntityID, IID_Player, {
	"GetDisabledTemplates": () => ({ "units/athen/infantry_swordsman_b": true }),
	"GetPlayerID": () => playerID
});

cmpTrainer.CalculateEntitiesMap();
TS_ASSERT_UNEVAL_EQUALS(
	cmpTrainer.GetEntitiesList(),
	["units/iber/cavalry_javelineer_b", "units/iber/infantry_swordsman_b", "units/iber/support_female_citizen"]
);

AddMock(playerEntityID, IID_Player, {
	"GetDisabledTemplates": () => ({ "units/iber/infantry_swordsman_b": true }),
	"GetPlayerID": () => playerID
});

cmpTrainer.CalculateEntitiesMap();
TS_ASSERT_UNEVAL_EQUALS(
	cmpTrainer.GetEntitiesList(),
	["units/iber/cavalry_javelineer_b", "units/iber/support_female_citizen"]
);

AddMock(playerEntityID, IID_Player, {
	"GetDisabledTemplates": () => ({ "units/athen/infantry_swordsman_b": true }),
	"GetPlayerID": () => playerID
});

AddMock(playerEntityID, IID_Identity, {
	"GetCiv": () => "athen",
});

cmpTrainer.CalculateEntitiesMap();
TS_ASSERT_UNEVAL_EQUALS(
	cmpTrainer.GetEntitiesList(),
	["units/athen/cavalry_javelineer_b", "units/iber/support_female_citizen"]
);

AddMock(playerEntityID, IID_Player, {
	"GetDisabledTemplates": () => ({ "units/iber/infantry_swordsman_b": false }),
	"GetPlayerID": () => playerID
});

AddMock(playerEntityID, IID_Identity, {
	"GetCiv": () => "iber",
});

cmpTrainer.CalculateEntitiesMap();
TS_ASSERT_UNEVAL_EQUALS(
	cmpTrainer.GetEntitiesList(),
	["units/iber/cavalry_javelineer_b", "units/iber/infantry_swordsman_b", "units/iber/support_female_citizen"]
);


// Test Queuing a unit.
const queuedUnit = "units/iber/infantry_swordsman_b";
const cost = {
	"food": 10
};

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": () => true,
	"GetTemplate": name => ({
		"Cost": {
			"BuildTime": 1,
			"Population": 1,
			"Resources": cost
		},
		"TrainingRestrictions": {
			"Category": "some_limit",
			"MatchLimit": "7"
		}
	})
});
AddMock(SYSTEM_ENTITY, IID_Trigger, {
	"CallEvent": () => {}
});
AddMock(SYSTEM_ENTITY, IID_GuiInterface, {
	"PushNotification": () => {},
	"SetSelectionDirty": () => {}
});

const cmpPlayer = AddMock(playerEntityID, IID_Player, {
	"BlockTraining": () => {},
	"GetPlayerID": () => playerID,
	"RefundResources": (resources) => {
		TS_ASSERT_UNEVAL_EQUALS(resources, cost);
	},
	"TrySubtractResources": (resources) => {
		TS_ASSERT_UNEVAL_EQUALS(resources, cost);
		// Just have enough resources.
		return true;
	},
	"TryReservePopulationSlots": () => false, // Always have pop space.
	"UnReservePopulationSlots": () => {}, // Always have pop space.
	"UnBlockTraining": () => {},
	"GetDisabledTemplates": () => ({})
});
const spyCmpPlayerSubtract = new Spy(cmpPlayer, "TrySubtractResources");
const spyCmpPlayerRefund = new Spy(cmpPlayer, "RefundResources");
const spyCmpPlayerPop = new Spy(cmpPlayer, "TryReservePopulationSlots");

ConstructComponent(playerEntityID, "EntityLimits", {
	"Limits": {
		"some_limit": 0
	},
	"LimitChangers": {},
	"LimitRemovers": {}
});
// Test that we can't exceed the entity limit.
TS_ASSERT_EQUALS(cmpTrainer.QueueBatch(queuedUnit, 1), -1);
// And that in that case, the resources are not lost.
// ToDo: This is a bad test, it relies on the order of subtraction in the cmp.
// Better would it be to check the states before and after the queue.
TS_ASSERT_EQUALS(spyCmpPlayerSubtract._called, spyCmpPlayerRefund._called);

ConstructComponent(playerEntityID, "EntityLimits", {
	"Limits": {
		"some_limit": 5
	},
	"LimitChangers": {},
	"LimitRemovers": {}
});
let id = cmpTrainer.QueueBatch(queuedUnit, 1);
TS_ASSERT_EQUALS(spyCmpPlayerSubtract._called, 2);
TS_ASSERT_EQUALS(cmpTrainer.queue.size, 1);


// Test removing a queued batch.
cmpTrainer.StopBatch(id);
TS_ASSERT_EQUALS(spyCmpPlayerRefund._called, 2);
TS_ASSERT_EQUALS(cmpTrainer.queue.size, 0);

const cmpEntLimits = QueryOwnerInterface(entityID, IID_EntityLimits);
TS_ASSERT(cmpEntLimits.AllowedToTrain("some_limit", 5));


// Test finishing a queued batch.
id = cmpTrainer.QueueBatch(queuedUnit, 1);
TS_ASSERT(cmpEntLimits.AllowedToTrain("some_limit", 4));
TS_ASSERT_EQUALS(cmpTrainer.GetBatch(id).progress, 0);
TS_ASSERT_EQUALS(cmpTrainer.Progress(id, 500), 500);
TS_ASSERT_EQUALS(spyCmpPlayerPop._called, 1);
TS_ASSERT_EQUALS(cmpTrainer.GetBatch(id).progress, 0.5);

const spawedEntityIDs = [4, 5, 6, 7, 8];
let spawned = 0;

Engine.AddEntity = () => {
	const ent = spawedEntityIDs[spawned++];

	ConstructComponent(ent, "TrainingRestrictions", {
		"Category": "some_limit"
	});

	AddMock(ent, IID_Identity, {
		"GetClassesList": () => []
	});

	AddMock(ent, IID_Position, {
		"JumpTo": () => {}
	});

	AddMock(ent, IID_Ownership, {
		"SetOwner": (pid) => {
			QueryOwnerInterface(ent, IID_EntityLimits).OnGlobalOwnershipChanged({
				"entity": ent,
				"from": -1,
				"to": pid
			});
		},
		"GetOwner": () => playerID
	});

	return ent;
};
AddMock(entityID, IID_Footprint, {
	"PickSpawnPoint": () => ({ "x": 0, "y": 1, "z": 0 })
});

cmpTrainer = SerializationCycle(cmpTrainer);

TS_ASSERT_EQUALS(cmpTrainer.Progress(id, 1000), 500);
TS_ASSERT(!cmpTrainer.HasBatch(id));
TS_ASSERT(!cmpEntLimits.AllowedToTrain("some_limit", 5));
TS_ASSERT(cmpEntLimits.AllowedToTrain("some_limit", 4));

TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 1);
TS_ASSERT_EQUALS(cmpEntLimits.GetMatchCounts()["units/iber/infantry_swordsman_b"], 1);


// Now check that it doesn't get updated when the spawn doesn't succeed. (regression_test_d1879)
cmpPlayer.TrySubtractResources = () => true;
cmpPlayer.RefundResources = () => {};
AddMock(entityID, IID_Footprint, {
	"PickSpawnPoint": () => ({ "x": -1, "y": -1, "z": -1 })
});
id = cmpTrainer.QueueBatch(queuedUnit, 2);
TS_ASSERT_EQUALS(cmpTrainer.Progress(id, 2000), 2000);
TS_ASSERT(cmpTrainer.HasBatch(id));

TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 3);
TS_ASSERT_EQUALS(cmpEntLimits.GetMatchCounts()["units/iber/infantry_swordsman_b"], 3);

cmpTrainer = SerializationCycle(cmpTrainer);

// Check that when the batch is removed the counts are subtracted again.
cmpTrainer.StopBatch(id);
TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 1);
TS_ASSERT_EQUALS(cmpEntLimits.GetMatchCounts()["units/iber/infantry_swordsman_b"], 1);

const queuedSecondUnit = "units/iber/cavalry_javelineer_b";
// Check changing the allowed entities has effect.
const id1 = cmpTrainer.QueueBatch(queuedUnit, 1);
const id2 = cmpTrainer.QueueBatch(queuedSecondUnit, 1);
TS_ASSERT_EQUALS(cmpTrainer.queue.size, 2);
TS_ASSERT_EQUALS(cmpTrainer.GetBatch(id1).unitTemplate, queuedUnit);
TS_ASSERT_EQUALS(cmpTrainer.GetBatch(id2).unitTemplate, queuedSecondUnit);

// Add a modifier that replaces unit A with unit C,
// adds a unit D and removes unit B from the roster.
Engine.RegisterGlobal("ApplyValueModificationsToEntity", (_, val) => {
	return typeof val === "string" ? HandleTokens(val, "units/{civ}/cavalry_javelineer_b>units/{civ}/c units/{civ}/d -units/{civ}/infantry_swordsman_b") : val;
});

cmpTrainer.OnValueModification({
	"component": "Trainer",
	"valueNames": ["Trainer/Entities/_string"],
	"entities": [entityID]
});

TS_ASSERT_UNEVAL_EQUALS(
	cmpTrainer.GetEntitiesList(), ["units/iber/c", "units/iber/support_female_citizen", "units/iber/d"]
);
TS_ASSERT_EQUALS(cmpTrainer.queue.size, 1);
TS_ASSERT_EQUALS(cmpTrainer.GetBatch(id1), undefined);
TS_ASSERT_EQUALS(cmpTrainer.GetBatch(id2).unitTemplate, "units/iber/c");


// Test that we can affect an empty trainer.
const emptyTrainer = ConstructComponent(entityID, "Trainer", null);
emptyTrainer.OnValueModification({ "component": "Trainer", "entities": [entityID], "valueNames": ["Trainer/Entities/"] });
TS_ASSERT_UNEVAL_EQUALS(
	emptyTrainer.GetEntitiesList(),
	["units/iber/d"]
);
