Resources = {
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
Engine.LoadComponentScript("interfaces/ResourceDropsite.js");
Engine.LoadComponentScript("interfaces/ResourceGatherer.js");
Engine.LoadComponentScript("interfaces/ResourceSupply.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("ResourceGatherer.js");

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (prop, oVal, ent) => oVal);

const entity = 11;
const target = 12;

let template = {
	"MaxDistance": "10",
	"BaseSpeed": "1",
	"Rates": {
		"food.grain": "1"
	},
	"Capacities": {
		"food": "10"
	}
};

let cmpResourceGatherer = ConstructComponent(entity, "ResourceGatherer", template);
cmpResourceGatherer.RecalculateGatherRates();
cmpResourceGatherer.RecalculateCapacities();

TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), []);
cmpResourceGatherer.GiveResources([{ "type": "food", "amount": 11 }]);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), [ {
	"type": "food",
	"amount": 11,
	"max": 10
}]);
cmpResourceGatherer.DropResources();
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), []);

// Test gathering.
AddMock(target, IID_ResourceSupply, {
	"GetType": () => {
		return {
			"generic": "food",
			"specific": "grain"
		};
	},
	"TakeResources": (amount) => {
		return {
			"amount": amount,
			"exhausted": false
		};
	},
	"GetDiminishingReturns": () => null
});

TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.PerformGather(target), {
	"amount": 1,
	"exhausted": false,
	"filled": false
});
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), [{
	"type": "food",
	"amount": 1,
	"max": 10
}]);

// Test committing resources.
AddMock(target, IID_ResourceDropsite, {
	"ReceiveResources": (resources, ent) => {
		return {
			"food": resources.food
		};
	}
});

cmpResourceGatherer.CommitResources(target);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), []);

cmpResourceGatherer.GiveResources([{
	"type": "food",
	"amount": 11
}, {
	"type": "wood",
	"amount": 1
}]);
cmpResourceGatherer.CommitResources(target);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), [ {
	"type": "wood",
	"amount": 1,
	"max": 0
}]);
