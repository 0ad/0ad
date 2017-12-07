Resources = {
        "BuildSchema": (a, b) => {}
};

Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/ProductionQueue.js");
Engine.LoadComponentScript("ProductionQueue.js");

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

AddMock(SYSTEM_ENTITY, IID_DataTemplateManager, {
	"TechnologyExists": name => name == "phase_town_athen" || name == "phase_city_athen"
});

AddMock(playerEntityID, IID_Player, {
	"GetCiv": () => "iber",
	"GetDisabledTechnologies": () => ({}),
	"GetDisabledTemplates": () => ({}),
	"GetPlayerID": () => playerId
});

AddMock(playerEntityID, IID_TechnologyManager, {
	"CheckTechnologyRequirements": () => true,
	"GetTechnologyTemplate": tech => tech,
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
	"GetTechnologyTemplate": tech => tech,
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
