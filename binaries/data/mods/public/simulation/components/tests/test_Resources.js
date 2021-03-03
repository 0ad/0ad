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

Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/ResourceDropsite.js");
Engine.LoadComponentScript("interfaces/ResourceGatherer.js");
Engine.LoadComponentScript("interfaces/ResourceSupply.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("Player.js");
Engine.LoadComponentScript("ResourceDropsite.js");
Engine.LoadComponentScript("ResourceGatherer.js");
Engine.LoadComponentScript("ResourceSupply.js");

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (prop, oVal, ent) => oVal);

const owner = 1;
const gatherer = 11;
const supply = 12;
const dropsite = 12;

AddMock(supply, IID_Fogging, {
	"Activate": () => {}
});

AddMock(gatherer, IID_Ownership, {
	"GetOwner": () => owner
});

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
	"GetPlayerByID": (id) => owner
});

let cmpPlayer = ConstructComponent(owner, "Player", {
	"SpyCostMultiplier": "1",
	"BarterMultiplier": {
		"Buy": {},
		"Sell": {}
	}
});
let playerSpy = new Spy(cmpPlayer, "AddResources");

let cmpResourceGatherer = ConstructComponent(gatherer, "ResourceGatherer", {
	"MaxDistance": "10",
	"BaseSpeed": "1",
	"Rates": {
		"food.meat": "1"
	},
	"Capacities": {
		"food": "10"
	}
});
cmpResourceGatherer.OnGlobalInitGame();
let cmpResourceSupply = ConstructComponent(supply, "ResourceSupply", {
	"Max": 1000,
	"Type": "food.meat",
	"KillBeforeGather": false,
	"MaxGatherers": 2
});
cmpResourceSupply.RecalculateValues();
let cmpResourceDropsite = ConstructComponent(dropsite, "ResourceDropsite", {
	"Sharable": "true",
	"Types": "food"
});

TS_ASSERT_UNEVAL_EQUALS(cmpResourceSupply.GetCurrentAmount(), 1000);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), []);

// Test gathering.

TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.PerformGather(supply), {
	"amount": 1,
	"exhausted": false,
	"filled": false
});
TS_ASSERT_UNEVAL_EQUALS(cmpResourceSupply.GetCurrentAmount(), 999);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), [{
	"type": "food",
	"amount": 1,
	"max": 10
}]);

// Test committing resources.

cmpResourceGatherer.CommitResources(dropsite);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), []);

TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.PerformGather(supply), {
	"amount": 1,
	"exhausted": false,
	"filled": false
});
cmpResourceGatherer.GiveResources([{
	"type": "wood",
	"amount": 1
}]);
cmpResourceGatherer.CommitResources(dropsite);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), [ {
	"type": "wood",
	"amount": 1,
	"max": 0
}]);
TS_ASSERT_EQUALS(playerSpy._called, 2);
