Engine.LoadHelperScript("Player.js");
Engine.LoadHelperScript("Sound.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/ProductionQueue.js");
Engine.LoadComponentScript("interfaces/BuildRestrictions.js");
Engine.LoadComponentScript("interfaces/EntityLimits.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/TrainingRestrictions.js");
Engine.LoadComponentScript("interfaces/Trigger.js");
Engine.LoadComponentScript("interfaces/Upgrade.js");
Engine.LoadComponentScript("EntityLimits.js");
Engine.LoadComponentScript("Timer.js");

Engine.RegisterGlobal("Resources", {
	"BuildSchema": (a, b) => {}
});
Engine.LoadComponentScript("ProductionQueue.js");
Engine.LoadComponentScript("TrainingRestrictions.js");

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (_, value) => value);
Engine.RegisterGlobal("ApplyValueModificationsToTemplate", (_, value) => value);

function testEntitiesList()
{
	Engine.RegisterGlobal("TechnologyTemplates", {
		"Has": name => name == "phase_town_athen" || name == "phase_city_athen",
		"Get": () => ({})
	});

	const productionQueueId = 6;
	const playerId = 1;
	const playerEntityID = 2;

	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		"TemplateExists": () => true,
		"GetTemplate": name => ({})
	});

	let cmpProductionQueue = ConstructComponent(productionQueueId, "ProductionQueue", {
		"Entities": { "_string": "units/{civ}/cavalry_javelineer_b " +
		                         "units/{civ}/infantry_swordsman_b " +
		                         "units/{native}/support_female_citizen" },
		"Technologies": { "_string": "gather_fishing_net " +
		                             "phase_town_{civ} " +
		                             "phase_city_{civ}" }
	});
	cmpProductionQueue.GetUpgradedTemplate = (template) => template;

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => playerEntityID
	});

	AddMock(playerEntityID, IID_Player, {
		"GetCiv": () => "iber",
		"GetDisabledTechnologies": () => ({}),
		"GetDisabledTemplates": () => ({}),
		"GetPlayerID": () => playerId
	});

	AddMock(playerEntityID, IID_TechnologyManager, {
		"CheckTechnologyRequirements": () => true,
		"IsInProgress": () => false,
		"IsTechnologyResearched": () => false
	});

	AddMock(productionQueueId, IID_Ownership, {
		"GetOwner": () => playerId
	});

	AddMock(productionQueueId, IID_Identity, {
		"GetCiv": () => "iber"
	});

	AddMock(productionQueueId, IID_Upgrade, {
		"IsUpgrading": () => false
	});

	cmpProductionQueue.CalculateEntitiesMap();
	TS_ASSERT_UNEVAL_EQUALS(
		cmpProductionQueue.GetEntitiesList(),
		["units/iber/cavalry_javelineer_b", "units/iber/infantry_swordsman_b", "units/iber/support_female_citizen"]
	);
	TS_ASSERT_UNEVAL_EQUALS(
		cmpProductionQueue.GetTechnologiesList(),
		["gather_fishing_net", "phase_town_generic", "phase_city_generic"]
	);

	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		"TemplateExists": name => name == "units/iber/support_female_citizen",
		"GetTemplate": name => ({})
	});

	cmpProductionQueue.CalculateEntitiesMap();
	TS_ASSERT_UNEVAL_EQUALS(cmpProductionQueue.GetEntitiesList(), ["units/iber/support_female_citizen"]);

	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		"TemplateExists": () => true,
		"GetTemplate": name => ({})
	});

	AddMock(playerEntityID, IID_Player, {
		"GetCiv": () => "iber",
		"GetDisabledTechnologies": () => ({}),
		"GetDisabledTemplates": () => ({ "units/athen/infantry_swordsman_b": true }),
		"GetPlayerID": () => playerId
	});

	cmpProductionQueue.CalculateEntitiesMap();
	TS_ASSERT_UNEVAL_EQUALS(
		cmpProductionQueue.GetEntitiesList(),
		["units/iber/cavalry_javelineer_b", "units/iber/infantry_swordsman_b", "units/iber/support_female_citizen"]
	);

	AddMock(playerEntityID, IID_Player, {
		"GetCiv": () => "iber",
		"GetDisabledTechnologies": () => ({}),
		"GetDisabledTemplates": () => ({ "units/iber/infantry_swordsman_b": true }),
		"GetPlayerID": () => playerId
	});

	cmpProductionQueue.CalculateEntitiesMap();
	TS_ASSERT_UNEVAL_EQUALS(
		cmpProductionQueue.GetEntitiesList(),
		["units/iber/cavalry_javelineer_b", "units/iber/support_female_citizen"]
	);

	AddMock(playerEntityID, IID_Player, {
		"GetCiv": () => "athen",
		"GetDisabledTechnologies": () => ({ "gather_fishing_net": true }),
		"GetDisabledTemplates": () => ({ "units/athen/infantry_swordsman_b": true }),
		"GetPlayerID": () => playerId
	});

	cmpProductionQueue.CalculateEntitiesMap();
	TS_ASSERT_UNEVAL_EQUALS(
		cmpProductionQueue.GetEntitiesList(),
		["units/athen/cavalry_javelineer_b", "units/iber/support_female_citizen"]
	);
	TS_ASSERT_UNEVAL_EQUALS(cmpProductionQueue.GetTechnologiesList(), ["phase_town_athen",
	                                                                   "phase_city_athen"]
	);

	AddMock(playerEntityID, IID_TechnologyManager, {
		"CheckTechnologyRequirements": () => true,
		"IsInProgress": () => false,
		"IsTechnologyResearched": tech => tech == "phase_town_athen"
	});
	TS_ASSERT_UNEVAL_EQUALS(cmpProductionQueue.GetTechnologiesList(), [undefined, "phase_city_athen"]);

	AddMock(playerEntityID, IID_Player, {
		"GetCiv": () => "iber",
		"GetDisabledTechnologies": () => ({}),
		"GetPlayerID": () => playerId
	});
	TS_ASSERT_UNEVAL_EQUALS(
		cmpProductionQueue.GetTechnologiesList(),
		["gather_fishing_net", "phase_town_generic", "phase_city_generic"]
	);
}

function regression_test_d1879()
{
	// Setup
	let playerEnt = 2;
	let playerID = 1;
	let testEntity = 3;
	let spawedEntityIDs = [4, 5, 6, 7, 8];
	let spawned = 0;

	Engine.AddEntity = () => {
		let id = spawedEntityIDs[spawned++];

		ConstructComponent(id, "TrainingRestrictions", {
			"Category": "some_limit"
		});

		AddMock(id, IID_Identity, {
			"GetClassesList": () => []
		});

		AddMock(id, IID_Position, {
			"JumpTo": () => {}
		});

		AddMock(id, IID_Ownership, {
			"SetOwner": (pid) => {
				let cmpEntLimits = QueryOwnerInterface(id, IID_EntityLimits);
				cmpEntLimits.OnGlobalOwnershipChanged({
					"entity": id,
					"from": -1,
					"to": pid
				});
			},
			"GetOwner": () => playerID
		});

		return id;
	};

	ConstructComponent(playerEnt, "EntityLimits", {
		"Limits": {
			"some_limit": 8
		},
		"LimitChangers": {},
		"LimitRemovers": {}
	});

	AddMock(SYSTEM_ENTITY, IID_GuiInterface, {
		"PushNotification": () => {}
	});

	AddMock(SYSTEM_ENTITY, IID_Trigger, {
		"CallEvent": () => {}
	});

	AddMock(SYSTEM_ENTITY, IID_Timer, {
		"SetInterval": (ent, iid, func) => 1,
		"CancelTimer": (id) => {}
	});

	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		"TemplateExists": () => true,
		"GetTemplate": name => ({
			"Cost": {
				"BuildTime": 0,
				"Population": 1,
				"Resources": {}
			},
			"TrainingRestrictions": {
				"Category": "some_limit",
				"MatchLimit": "7"
			}
		})
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => playerEnt
	});

	AddMock(playerEnt, IID_Player, {
		"GetCiv": () => "iber",
		"GetPlayerID": () => playerID,
		"GetTimeMultiplier": () => 0,
		"BlockTraining": () => {},
		"UnBlockTraining": () => {},
		"UnReservePopulationSlots": () => {},
		"TrySubtractResources": () => true,
		"AddResources": () => true,
		"TryReservePopulationSlots": () => false // Always have pop space.
	});

	AddMock(testEntity, IID_Ownership, {
		"GetOwner": () => playerID
	});

	let cmpProdQueue = ConstructComponent(testEntity, "ProductionQueue", {
		"Entities": { "_string": "some_template" },
		"BatchTimeModifier": 1
	});

	let cmpEntLimits = QueryOwnerInterface(testEntity, IID_EntityLimits);
	TS_ASSERT(cmpEntLimits.AllowedToTrain("some_limit", 8));
	TS_ASSERT(!cmpEntLimits.AllowedToTrain("some_limit", 9));
	TS_ASSERT(cmpEntLimits.AllowedToTrain("some_limit", 5, "some_template", 8));
	TS_ASSERT(!cmpEntLimits.AllowedToTrain("some_limit", 10, "some_template", 8));

	// Check that the entity limits do get updated if the spawn succeeds.
	AddMock(testEntity, IID_Footprint, {
		"PickSpawnPoint": () => ({ "x": 0, "y": 1, "z": 0 })
	});

	cmpProdQueue.AddItem("some_template", "unit", 3);

	TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 3);
	TS_ASSERT_EQUALS(cmpEntLimits.GetMatchCounts().some_template, 3);

	cmpProdQueue.ProgressTimeout(null, 0);

	TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 3);
	TS_ASSERT_EQUALS(cmpEntLimits.GetMatchCounts().some_template, 3);

	// Now check that it doesn't get updated when the spawn doesn't succeed.
	AddMock(testEntity, IID_Footprint, {
		"PickSpawnPoint": () => ({ "x": -1, "y": -1, "z": -1 })
	});

	AddMock(testEntity, IID_Upgrade, {
		"IsUpgrading": () => false
	});

	cmpProdQueue.AddItem("some_template", "unit", 3);
	cmpProdQueue.ProgressTimeout(null, 0);

	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 1);
	TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 6);
	TS_ASSERT_EQUALS(cmpEntLimits.GetMatchCounts().some_template, 6);

	// Check that when the batch is removed the counts are subtracted again.
	cmpProdQueue.RemoveItem(cmpProdQueue.GetQueue()[0].id);
	TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 3);
	TS_ASSERT_EQUALS(cmpEntLimits.GetMatchCounts().some_template, 3);
}

function test_batch_adding()
{
	let playerEnt = 2;
	let playerID = 1;
	let testEntity = 3;

	ConstructComponent(playerEnt, "EntityLimits", {
		"Limits": {
			"some_limit": 8
		},
		"LimitChangers": {},
		"LimitRemovers": {}
	});

	AddMock(SYSTEM_ENTITY, IID_GuiInterface, {
		"PushNotification": () => {}
	});

	AddMock(SYSTEM_ENTITY, IID_Trigger, {
		"CallEvent": () => {}
	});

	AddMock(SYSTEM_ENTITY, IID_Timer, {
		"SetInterval": (ent, iid, func) => 1
	});

	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		"TemplateExists": () => true,
		"GetTemplate": name => ({
			"Cost": {
				"BuildTime": 0,
				"Population": 1,
				"Resources": {}
			},
			"TrainingRestrictions": {
				"Category": "some_limit"
			}
		})
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => playerEnt
	});

	AddMock(playerEnt, IID_Player, {
		"GetCiv": () => "iber",
		"GetPlayerID": () => playerID,
		"GetTimeMultiplier": () => 0,
		"BlockTraining": () => {},
		"UnBlockTraining": () => {},
		"UnReservePopulationSlots": () => {},
		"TrySubtractResources": () => true,
		"TryReservePopulationSlots": () => false // Always have pop space.
	});

	AddMock(testEntity, IID_Ownership, {
		"GetOwner": () => playerID
	});

	let cmpProdQueue = ConstructComponent(testEntity, "ProductionQueue", {
		"Entities": { "_string": "some_template" },
		"BatchTimeModifier": 1
	});


	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 0);
	AddMock(testEntity, IID_Upgrade, {
		"IsUpgrading": () => true
	});

	cmpProdQueue.AddItem("some_template", "unit", 3);
	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 0);

	AddMock(testEntity, IID_Upgrade, {
		"IsUpgrading": () => false
	});

	cmpProdQueue.AddItem("some_template", "unit", 3);
	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 1);
}

function test_batch_removal()
{
	let playerEnt = 2;
	let playerID = 1;
	let testEntity = 3;

	ConstructComponent(playerEnt, "EntityLimits", {
		"Limits": {
			"some_limit": 8
		},
		"LimitChangers": {},
		"LimitRemovers": {}
	});

	AddMock(SYSTEM_ENTITY, IID_GuiInterface, {
		"PushNotification": () => {}
	});

	AddMock(SYSTEM_ENTITY, IID_Trigger, {
		"CallEvent": () => {}
	});

	let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer", null);

	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		"TemplateExists": () => true,
		"GetTemplate": name => ({
			"Cost": {
				"BuildTime": 0,
				"Population": 1,
				"Resources": {}
			},
			"TrainingRestrictions": {
				"Category": "some_limit"
			}
		})
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => playerEnt
	});

	let cmpPlayer = AddMock(playerEnt, IID_Player, {
		"GetCiv": () => "iber",
		"GetPlayerID": () => playerID,
		"GetTimeMultiplier": () => 0,
		"BlockTraining": () => {},
		"UnBlockTraining": () => {},
		"UnReservePopulationSlots": () => {},
		"TrySubtractResources": () => true,
		"AddResources": () => {},
		"TryReservePopulationSlots": () => 1
	});
	let cmpPlayerBlockSpy = new Spy(cmpPlayer, "BlockTraining");
	let cmpPlayerUnblockSpy = new Spy(cmpPlayer, "UnBlockTraining");

	AddMock(testEntity, IID_Ownership, {
		"GetOwner": () => playerID
	});

	let cmpProdQueue = ConstructComponent(testEntity, "ProductionQueue", {
		"Entities": { "_string": "some_template" },
		"BatchTimeModifier": 1
	});

	cmpProdQueue.AddItem("some_template", "unit", 3);
	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 1);
	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(cmpPlayerBlockSpy._called, 1);

	cmpProdQueue.AddItem("some_template", "unit", 2);
	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 2);

	cmpProdQueue.RemoveItem(1);
	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 1);
	TS_ASSERT_EQUALS(cmpPlayerUnblockSpy._called, 1);

	cmpProdQueue.RemoveItem(2);
	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 0);
	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(cmpPlayerUnblockSpy._called, 2);

	cmpProdQueue.AddItem("some_template", "unit", 3);
	cmpProdQueue.AddItem("some_template", "unit", 3);
	cmpPlayer.TryReservePopulationSlots = () => false;
	cmpProdQueue.RemoveItem(3);
	TS_ASSERT_EQUALS(cmpPlayerUnblockSpy._called, 3);
	cmpTimer.OnUpdate({ "turnLength": 1 });
	TS_ASSERT_EQUALS(cmpPlayerUnblockSpy._called, 4);
}

function test_token_changes()
{
	const ent = 10;
	let cmpProductionQueue = ConstructComponent(10, "ProductionQueue", {
		"Entities": { "_string": "units/{civ}/a " +
		                         "units/{civ}/b" },
		"Technologies": { "_string": "a " +
		                             "b_{civ} " +
		                             "c_{civ}" },
		"BatchTimeModifier": 1
	});
	cmpProductionQueue.GetUpgradedTemplate = (template) => template;

	// Merges interface of multiple components because it's enough here.
	Engine.RegisterGlobal("QueryOwnerInterface", () => ({
		// player
		"GetCiv": () => "test",
		"GetDisabledTemplates": () => [],
		"GetDisabledTechnologies": () => [],
		"TryReservePopulationSlots": () => false, // Always have pop space.
		"TrySubtractResources": () => true,
		"UnBlockTraining": () => {},
		"AddResources": () => {},
		"GetPlayerID": () => 1,
		// entitylimits
		"ChangeCount": () => {},
		"AllowedToTrain": () => true,
		// techmanager
		"CheckTechnologyRequirements": () => true,
		"IsTechnologyResearched": () => false,
		"IsInProgress": () => false
	}));
	Engine.RegisterGlobal("QueryPlayerIDInterface", QueryOwnerInterface);

	AddMock(SYSTEM_ENTITY, IID_GuiInterface, {
		"SetSelectionDirty": () => {}
	});

	// Test Setup
	cmpProductionQueue.CalculateEntitiesMap();
	TS_ASSERT_UNEVAL_EQUALS(
		cmpProductionQueue.GetEntitiesList(), ["units/test/a", "units/test/b"]
	);
	TS_ASSERT_UNEVAL_EQUALS(
		cmpProductionQueue.GetTechnologiesList(), ["a", "b_generic", "c_generic"]
	);
	// Add a unit of each type to our queue, validate.
	cmpProductionQueue.AddItem("units/test/a", "unit", 1, {});
	cmpProductionQueue.AddItem("units/test/b", "unit", 1, {});
	TS_ASSERT_EQUALS(cmpProductionQueue.GetQueue()[0].unitTemplate, "units/test/a");
	TS_ASSERT_EQUALS(cmpProductionQueue.GetQueue()[1].unitTemplate, "units/test/b");

	// Add a modifier that replaces unit A with unit C,
	// adds a unit D and removes unit B from the roster.
	Engine.RegisterGlobal("ApplyValueModificationsToEntity", (_, val) => {
		return HandleTokens(val, "units/{civ}/a>units/{civ}/c units/{civ}/d -units/{civ}/b");
	});

	cmpProductionQueue.OnValueModification({
		"component": "ProductionQueue",
		"valueNames": ["ProductionQueue/Entities/_string"],
		"entities": [ent]
	});

	TS_ASSERT_UNEVAL_EQUALS(
		cmpProductionQueue.GetEntitiesList(), ["units/test/c", "units/test/d"]
	);
	TS_ASSERT_EQUALS(cmpProductionQueue.GetQueue()[0].unitTemplate, "units/test/c");
	TS_ASSERT_EQUALS(cmpProductionQueue.GetQueue().length, 1);
}

function test_auto_queue()
{
	let playerEnt = 2;
	let playerID = 1;
	let testEntity = 3;

	ConstructComponent(playerEnt, "EntityLimits", {
		"Limits": {
			"some_limit": 8
		},
		"LimitChangers": {},
		"LimitRemovers": {}
	});

	AddMock(SYSTEM_ENTITY, IID_GuiInterface, {
		"PushNotification": () => {}
	});

	AddMock(SYSTEM_ENTITY, IID_Trigger, {
		"CallEvent": () => {}
	});

	AddMock(SYSTEM_ENTITY, IID_Timer, {
		"SetInterval": (ent, iid, func) => 1
	});

	AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
		"TemplateExists": () => true,
		"GetTemplate": name => ({
			"Cost": {
				"BuildTime": 0,
				"Population": 1,
				"Resources": {}
			},
			"TrainingRestrictions": {
				"Category": "some_limit"
			}
		})
	});

	AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
		"GetPlayerByID": id => playerEnt
	});

	AddMock(playerEnt, IID_Player, {
		"GetCiv": () => "iber",
		"GetPlayerID": () => playerID,
		"GetTimeMultiplier": () => 0,
		"BlockTraining": () => {},
		"UnBlockTraining": () => {},
		"UnReservePopulationSlots": () => {},
		"TrySubtractResources": () => true,
		"TryReservePopulationSlots": () => false // Always have pop space.
	});

	AddMock(testEntity, IID_Ownership, {
		"GetOwner": () => playerID
	});

	let cmpProdQueue = ConstructComponent(testEntity, "ProductionQueue", {
		"Entities": { "_string": "some_template" },
		"BatchTimeModifier": 1
	});

	cmpProdQueue.EnableAutoQueue();

	cmpProdQueue.AddItem("some_template", "unit", 3);
	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 1);
	cmpProdQueue.ProgressTimeout(null, 0);
	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 2);
}

testEntitiesList();
regression_test_d1879();
test_batch_adding();
test_batch_removal();
test_auto_queue();
test_token_changes();
