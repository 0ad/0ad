Resources = {
	"GetCodes": () => ["food", "metal", "stone", "wood"],
	"GetTradableCodes": () => ["food", "metal", "stone", "wood"],
	"GetBarterableCodes": () => ["food", "metal", "stone", "wood"],
	"BuildSchema": () => {
		let schema = "";
		for (let res of ["food", "metal"])
		{
			for (let subtype in ["meat", "grain"])
				schema += "<value>" + res + "." + subtype + "</value>";
			schema += "<value> treasure." + res + "</value>";
		}
		return "<choice>" + schema + "</choice>";
	},
	"BuildChoicesSchema": () => {
		let schema = "";
		for (let res of ["food", "metal"])
		{
			for (let subtype in ["meat", "grain"])
				schema += "<value>" + res + "." + subtype + "</value>";
			schema += "<value> treasure." + res + "</value>";
		}
		return "<choice>" + schema + "</choice>";
	},
	"GetResource": (type) => {
		return {
			"subtypes": {
				"meat": "meat",
				"grain": "grain"
			}
		};
	}
};

Engine.LoadComponentScript("interfaces/EntityLimits.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/Researcher.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/TechnologyManager.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/Trigger.js");
Engine.LoadComponentScript("Player.js");
Engine.LoadComponentScript("Researcher.js");
Engine.LoadComponentScript("TechnologyManager.js");
Engine.LoadComponentScript("Timer.js");
Engine.LoadComponentScript("Trigger.js");

Engine.LoadHelperScript("Player.js");

ConstructComponent(SYSTEM_ENTITY, "Trigger");
const cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer", null);

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (_, value) => typeof value === "string" ? value + " some_test": value);

const template = {
	"name": "templateName"
};
Engine.RegisterGlobal("TechnologyTemplates", {
	"GetAll": () => [],
	"Get": (tech) => {
		return template;
	}
});

const playerID = 1;
const playerEntityID = 11;
const researcherID = 21;

let cmpTechnologyManager = ConstructComponent(playerEntityID, "TechnologyManager", null);

AddMock(researcherID, IID_Ownership, {
	"GetOwner": () => playerID
});
AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": id => playerEntityID
});
AddMock(playerEntityID, IID_Identity, {
	"GetCiv": () => "gaia"
});
template.cost = {
	"food": 100
};
template.researchTime = 1.5;

const cmpPlayer = ConstructComponent(playerEntityID, "Player", {
	"SpyCostMultiplier": "1",
	"BarterMultiplier": {
		"Buy": {},
		"Sell": {}
	},
	"Formations": { "_string": "" }
});
const spyPlayerResSub = new Spy(cmpPlayer, "TrySubtractResources");
const spyPlayerResRefund = new Spy(cmpPlayer, "RefundResources");

let cmpResearcher = ConstructComponent(researcherID, "Researcher", {
	"Technologies": {
		"_string": template.name
	}
});

let id = cmpResearcher.QueueTechnology(template.name);

TS_ASSERT_EQUALS(spyPlayerResSub._called, 1);
TS_ASSERT(!cmpTechnologyManager.IsTechnologyResearched(template.name));
TS_ASSERT(cmpTechnologyManager.IsInProgress(template.name));
TS_ASSERT_EQUALS(cmpResearcher.Progress(id, 1000), 1000);
cmpResearcher = SerializationCycle(cmpResearcher);
cmpTechnologyManager = SerializationCycle(cmpTechnologyManager);

cmpResearcher.StopResearching(id);
TS_ASSERT(!cmpTechnologyManager.IsInProgress(template.name));
TS_ASSERT_EQUALS(spyPlayerResRefund._called, 1);

id = cmpResearcher.QueueTechnology(template.name);
TS_ASSERT_EQUALS(spyPlayerResSub._called, 2);
TS_ASSERT_EQUALS(cmpResearcher.Progress(id, 1000), 1000);
cmpResearcher = SerializationCycle(cmpResearcher);
cmpTechnologyManager = SerializationCycle(cmpTechnologyManager);
TS_ASSERT_EQUALS(cmpResearcher.Progress(id, 1000), 500);
TS_ASSERT(cmpTechnologyManager.IsTechnologyResearched(template.name));
TS_ASSERT_EQUALS(spyPlayerResRefund._called, 1);
