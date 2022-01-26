Engine.LoadComponentScript("interfaces/EntityLimits.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/Trigger.js");
Engine.LoadComponentScript("TechnologyManager.js");
Engine.LoadComponentScript("Trigger.js");

Engine.LoadHelperScript("Player.js");

ConstructComponent(SYSTEM_ENTITY, "Trigger");

const techTemplate = {};
Engine.RegisterGlobal("TechnologyTemplates", {
	"GetAll": () => [],
	"Get": (tech) => {
		return techTemplate;
	}
});

const playerID = 1;
const playerEntityID = 11;

let cmpTechnologyManager = ConstructComponent(playerEntityID, "TechnologyManager", null);

// Test CheckTechnologyRequirements
const template = { "requirements": { "all": [{ "entity": { "class": "Village", "number": 5 } }, { "civ": "athen" }] } };
cmpTechnologyManager.classCounts.Village = 2;
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "athen")), false);
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "athen"), true), true);
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "maur"), true), false);
cmpTechnologyManager.classCounts.Village = 6;
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "athen")), true);
TS_ASSERT_EQUALS(cmpTechnologyManager.CheckTechnologyRequirements(DeriveTechnologyRequirements(template, "maur")), false);

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => playerEntityID
});

const templateName = "template";
techTemplate.cost = {
	"food": 100
};
const cmpPlayer = AddMock(playerEntityID, IID_Player, {
	"GetPlayerID": () => playerID,
	"TrySubtractResources": (resources) => {
		TS_ASSERT_UNEVAL_EQUALS(resources, techTemplate.cost);
		// Just have enough resources.
		return true;
	},
	"RefundResources": (resources) => {
		TS_ASSERT_UNEVAL_EQUALS(resources, techTemplate.cost);
	},
});
const spyCmpPlayerSubtract = new Spy(cmpPlayer, "TrySubtractResources");
const spyCmpPlayerRefund = new Spy(cmpPlayer, "RefundResources");

TS_ASSERT(cmpTechnologyManager.QueuedResearch(templateName, INVALID_ENTITY, { "food": 1 }));
TS_ASSERT(cmpTechnologyManager.IsInProgress(templateName));
TS_ASSERT_EQUALS(spyCmpPlayerSubtract._called, 1);

// Test refunding before start.
cmpTechnologyManager.StoppedResearch(templateName, true);
TS_ASSERT(!cmpTechnologyManager.IsInProgress(templateName));
TS_ASSERT_EQUALS(spyCmpPlayerRefund._called, 1);

techTemplate.researchTime = 2;
TS_ASSERT(cmpTechnologyManager.QueuedResearch(templateName, INVALID_ENTITY, { "food": 1, "time": 1 }));
TS_ASSERT_EQUALS(cmpTechnologyManager.Progress(templateName, 500), 500);

cmpTechnologyManager = SerializationCycle(cmpTechnologyManager);

cmpTechnologyManager.Pause(templateName);
TS_ASSERT_UNEVAL_EQUALS(cmpTechnologyManager.GetBasicInfoOfStartedTechs(), {
	[templateName]: {
		"paused": true,
		"progress": 0.25,
		"researcher": INVALID_ENTITY,
		"templateName": templateName,
		"timeRemaining": 1500
	},
});

TS_ASSERT(!cmpTechnologyManager.IsTechnologyResearched(templateName));
TS_ASSERT_EQUALS(cmpTechnologyManager.Progress(templateName, 2000), 1500);
TS_ASSERT(cmpTechnologyManager.IsTechnologyResearched(templateName));

TS_ASSERT(cmpTechnologyManager.QueuedResearch(templateName, INVALID_ENTITY, { "food": 1, "time": 1 }));
TS_ASSERT_EQUALS(cmpTechnologyManager.Progress(templateName, 500), 500);
TS_ASSERT(cmpTechnologyManager.IsInProgress(templateName));

cmpTechnologyManager = SerializationCycle(cmpTechnologyManager);

// Test refunding after start.
cmpTechnologyManager.StoppedResearch(templateName, true);
TS_ASSERT(!cmpTechnologyManager.IsInProgress(templateName));
TS_ASSERT_EQUALS(spyCmpPlayerRefund._called, 2);
