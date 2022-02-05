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
	"GetResource": (type) => {
		return {
			"subtypes": {
				"meat": "meat",
				"grain": "grain"
			}
		};
	}
};

Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/Treasure.js");
Engine.LoadComponentScript("interfaces/TreasureCollector.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/Trigger.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("Player.js");
Engine.LoadComponentScript("Timer.js");
Engine.LoadComponentScript("Treasure.js");
Engine.LoadComponentScript("TreasureCollector.js");
Engine.LoadComponentScript("Trigger.js");

let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer", {});
Engine.RegisterGlobal("ApplyValueModificationsToEntity", (prop, oVal, ent) => oVal);
ConstructComponent(SYSTEM_ENTITY, "Trigger", {});

const treasure = 11;
const treasurer = 12;
const owner = 1;

AddMock(treasurer, IID_Ownership, {
	"GetOwner": () => owner
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": (id) => owner
});

AddMock(SYSTEM_ENTITY, IID_ObstructionManager, {
	"IsInTargetRange": (ent, target, min, max, invert) => true
});

let cmpPlayer = ConstructComponent(owner, "Player", {
	"SpyCostMultiplier": 1,
	"BarterMultiplier": {
		"Buy": {},
		"Sell": {}
	},
	"Formations": { "_string": "" }
});
let playerSpy = new Spy(cmpPlayer, "AddResources");

let cmpTreasure = ConstructComponent(treasure, "Treasure", {
	"CollectTime": "1000",
	"Resources": {
		"Food": "10"
	}
});
cmpTreasure.OnOwnershipChanged({ "to": 0 });

let cmpTreasurer = ConstructComponent(treasurer, "TreasureCollector", {
	"MaxDistance": "2.0"
});

TS_ASSERT(cmpTreasurer.StartCollecting(treasure));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_EQUALS(playerSpy._called, 1);
