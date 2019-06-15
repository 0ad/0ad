Resources = {
        "BuildSchema": (a, b) => {}
};

Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/ProductionQueue.js");
Engine.LoadComponentScript("ProductionQueue.js");

global.TechnologyTemplates = {
	"Has": name => name == "phase_town_athen" || name == "phase_city_athen",
	"Get": () => ({})
};

const productionQueueId = 6;
const playerId = 1;
const playerEntityID = 2;

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": () => true,
	"GetTemplate": name => ({})
});

let cmpProductionQueue = ConstructComponent(productionQueueId, "ProductionQueue", {
	"Entities": { "_string": "units/{civ}_cavalry_javelinist_b " +
	                         "units/{civ}_infantry_swordsman_b " +
	                         "units/{native}_support_female_citizen" },
	"Technologies": { "_string": "gather_fishing_net " +
	                             "phase_town_{civ} " +
	                             "phase_city_{civ}" }
});

cmpProductionQueue.CalculateEntitiesList();
TS_ASSERT_UNEVAL_EQUALS(cmpProductionQueue.GetEntitiesList(), []);

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

cmpProductionQueue.CalculateEntitiesList();
TS_ASSERT_UNEVAL_EQUALS(
	cmpProductionQueue.GetEntitiesList(),
	["units/iber_cavalry_javelinist_b", "units/iber_infantry_swordsman_b", "units/iber_support_female_citizen"]
);
TS_ASSERT_UNEVAL_EQUALS(
	cmpProductionQueue.GetTechnologiesList(),
	["gather_fishing_net", "phase_town_generic", "phase_city_generic"]
);

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": name => name == "units/iber_support_female_citizen",
	"GetTemplate": name => ({})
});

cmpProductionQueue.CalculateEntitiesList();
TS_ASSERT_UNEVAL_EQUALS(cmpProductionQueue.GetEntitiesList(), ["units/iber_support_female_citizen"]);

AddMock(SYSTEM_ENTITY, IID_TemplateManager, {
	"TemplateExists": () => true,
	"GetTemplate": name => ({})
});

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "iber",
	"GetDisabledTechnologies": () => ({}),
	"GetDisabledTemplates": () => ({ "units/athen_infantry_swordsman_b": true }),
	"GetPlayerID": () => playerId
});

cmpProductionQueue.CalculateEntitiesList();
TS_ASSERT_UNEVAL_EQUALS(
	cmpProductionQueue.GetEntitiesList(),
	["units/iber_cavalry_javelinist_b", "units/iber_infantry_swordsman_b", "units/iber_support_female_citizen"]
);

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "iber",
	"GetDisabledTechnologies": () => ({}),
	"GetDisabledTemplates": () => ({ "units/iber_infantry_swordsman_b": true }),
	"GetPlayerID": () => playerId
});

cmpProductionQueue.CalculateEntitiesList();
TS_ASSERT_UNEVAL_EQUALS(
	cmpProductionQueue.GetEntitiesList(),
	["units/iber_cavalry_javelinist_b", "units/iber_support_female_citizen"]
);

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "athen",
	"GetDisabledTechnologies": () => ({ "gather_fishing_net": true }),
	"GetDisabledTemplates": () => ({ "units/athen_infantry_swordsman_b": true }),
	"GetPlayerID": () => playerId
});

cmpProductionQueue.CalculateEntitiesList();
TS_ASSERT_UNEVAL_EQUALS(
	cmpProductionQueue.GetEntitiesList(),
	["units/athen_cavalry_javelinist_b", "units/iber_support_female_citizen"]
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

Engine.LoadComponentScript("interfaces/BuildRestrictions.js");
Engine.LoadComponentScript("interfaces/EntityLimits.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/TrainingRestrictions.js");
Engine.LoadComponentScript("interfaces/Trigger.js");
Engine.LoadComponentScript("EntityLimits.js");
Engine.LoadComponentScript("TrainingRestrictions.js");
Engine.LoadHelperScript("Sound.js");

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (_, value) => value);
Engine.RegisterGlobal("ApplyValueModificationsToTemplate", (_, value) => value);

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
		"SetTimeout": (ent, iid, func) => {}
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

	let cmpEntLimits = QueryOwnerInterface(testEntity, IID_EntityLimits);
	TS_ASSERT(cmpEntLimits.AllowedToTrain("some_limit", 8));
	TS_ASSERT(!cmpEntLimits.AllowedToTrain("some_limit", 9));

	// Check that the entity limits do get updated if the spawn succeeds.
	AddMock(testEntity, IID_Footprint, {
		"PickSpawnPoint": () => ({ "x": 0, "y": 1, "z": 0 })
	});

	cmpProdQueue.AddBatch("some_template", "unit", 3);

	TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 3);

	Engine.QueryInterface(testEntity, IID_ProductionQueue).ProgressTimeout();

	TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 3);

	// Now check that it doesn't get updated when the spawn doesn't succeed.
	AddMock(testEntity, IID_Footprint, {
		"PickSpawnPoint": () => ({ "x": -1, "y": -1, "z": -1 })
	});

	cmpProdQueue.AddBatch("some_template", "unit", 3);
	Engine.QueryInterface(testEntity, IID_ProductionQueue).ProgressTimeout();

	TS_ASSERT_EQUALS(cmpProdQueue.GetQueue().length, 1);
	TS_ASSERT_EQUALS(cmpEntLimits.GetCounts().some_limit, 6);
}

regression_test_d1879();
