Resources = {
	"BuildSchema": () => {
		let schema = "";
		for (let res of ["food", "metal", "wood"])
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
				"grain": "grain",
				"tree": "tree"
			}
		};
	}
};

Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/ResourceDropsite.js");
Engine.LoadComponentScript("interfaces/ResourceGatherer.js");
Engine.LoadComponentScript("interfaces/ResourceSupply.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/UnitAI.js");
Engine.LoadComponentScript("ResourceGatherer.js");
Engine.LoadComponentScript("Timer.js");

AddMock(SYSTEM_ENTITY, IID_ObstructionManager, {
	"IsInTargetRange": () => true
});

Engine.RegisterGlobal("ApplyValueModificationsToEntity", (prop, oVal, ent) => oVal);
Engine.RegisterGlobal("QueryOwnerInterface", () => {});
let cmpTimer;

const gathererID = 11;
const dropsiteID = 12;
const supplyID = 13;

let template = {
	"MaxDistance": "10",
	"BaseSpeed": "1",
	"Rates": {
		"food.grain": "1",
		"wood.tree": "2"
	},
	"Capacities": {
		"food": "10",
		"wood": "20"
	}
};

let cmpResourceGatherer = ConstructComponent(gathererID, "ResourceGatherer", template);
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

// Test committing resources.
AddMock(dropsiteID, IID_ResourceDropsite, {
	"ReceiveResources": (resources, ent) => {
		return {
			"food": resources.food
		};
	}
});
cmpResourceGatherer.GiveResources([{ "type": "food", "amount": 1 }]);

cmpResourceGatherer.CommitResources(dropsiteID);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), []);

cmpResourceGatherer.GiveResources([{
	"type": "food",
	"amount": 11
}, {
	"type": "wood",
	"amount": 1
}]);
cmpResourceGatherer.CommitResources(dropsiteID);
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(), [ {
	"type": "wood",
	"amount": 1,
	"max": 20
}]);
cmpResourceGatherer.DropResources();


function reset()
{
	cmpResourceGatherer = ConstructComponent(gathererID, "ResourceGatherer", template);
	cmpResourceGatherer.OnGlobalInitGame(); // Force updating values.
	cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer", null);
}

// Test normal gathering.
reset();

// Supply is empty.
AddMock(supplyID, IID_ResourceSupply, {
	"AddActiveGatherer": () => true,
	"GetCurrentAmount": () => 0
});

// Supply is wrong type.
AddMock(supplyID, IID_ResourceSupply, {
	"AddActiveGatherer": () => true,
	"GetCurrentAmount": () => 1,
	"GetType": () => ({ "generic": "bogus" }),
	"GetDiminishingReturns": () => 1

});
TS_ASSERT(!cmpResourceGatherer.StartGathering(supplyID));
TS_ASSERT(!cmpResourceGatherer.StartGathering(supplyID));

// Resource supply is full (or something else).
AddMock(supplyID, IID_ResourceSupply, {
	"AddActiveGatherer": () => false,
	"GetCurrentAmount": () => 1,
	"GetType": () => ({ "generic": "food" }),
	"GetDiminishingReturns": () => 1
});
TS_ASSERT(!cmpResourceGatherer.StartGathering(supplyID));


// Supply is non-empty and right type.
AddMock(supplyID, IID_ResourceSupply, {
	"AddActiveGatherer": () => true,
	"GetCurrentAmount": () => 2,
	"GetType": () => ({ "generic": "food", "specific": "grain" }),
	"GetDiminishingReturns": () => 1,
	"TakeResources": (amount) => {
		return {
			"amount": amount,
			"exhausted": false
		};
	},
	"RemoveGatherer": () => {}

});
TS_ASSERT(cmpResourceGatherer.StartGathering(supplyID));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 1, "max": 10 }]);
TS_ASSERT_EQUALS(cmpResourceGatherer.GetMainCarryingType(), "food");
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetLastCarriedType(), { "generic": "food", "specific": "grain" });
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 2, "max": 10 }]);
cmpResourceGatherer.StopGathering();
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 2, "max": 10 }]);
TS_ASSERT(cmpResourceGatherer.IsCarrying("food"));
TS_ASSERT(cmpResourceGatherer.CanCarryMore("food"));


// Test that when gathering a second type the first gathered type is ditched.
reset();
AddMock(supplyID, IID_ResourceSupply, {
	"AddActiveGatherer": () => true,
	"GetCurrentAmount": () => 2,
	"GetType": () => ({ "generic": "food", "specific": "grain" }),
	"GetDiminishingReturns": () => 1,
	"TakeResources": (amount) => {
		return {
			"amount": amount,
			"exhausted": false
		};
	},
	"RemoveGatherer": () => {}

});
TS_ASSERT(cmpResourceGatherer.StartGathering(supplyID));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 1, "max": 10 }]);
cmpResourceGatherer.StopGathering();

AddMock(supplyID, IID_ResourceSupply, {
	"AddActiveGatherer": () => true,
	"GetCurrentAmount": () => 3,
	"GetType": () => ({ "generic": "wood", "specific": "tree" }),
	"GetDiminishingReturns": () => 1,
	"TakeResources": (amount) => {
		return {
			"amount": amount,
			"exhausted": false
		};
	},
	"RemoveGatherer": () => {}

});
TS_ASSERT(cmpResourceGatherer.StartGathering(supplyID));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "wood", "amount": 2, "max": 20 }]);

TS_ASSERT(!cmpResourceGatherer.IsCarrying("food"));
TS_ASSERT(cmpResourceGatherer.CanCarryMore("food"));
TS_ASSERT(cmpResourceGatherer.IsCarrying("wood"));
TS_ASSERT(cmpResourceGatherer.CanCarryMore("wood"));


// Test that we stop gathering when the target is exhausted.
reset();
AddMock(supplyID, IID_ResourceSupply, {
	"AddActiveGatherer": () => true,
	"GetCurrentAmount": () => 1,
	"GetType": () => ({ "generic": "food", "specific": "grain" }),
	"GetDiminishingReturns": () => 1,
	"TakeResources": (amount) => {
		return {
			"amount": amount,
			"exhausted": true
		};
	},
	"RemoveGatherer": () => {}

});
TS_ASSERT(cmpResourceGatherer.StartGathering(supplyID));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 1, "max": 10 }]);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 1, "max": 10 }]);


// Test that we stop gathering when we are filled.
reset();
AddMock(supplyID, IID_ResourceSupply, {
	"AddActiveGatherer": () => true,
	"GetCurrentAmount": () => 11,
	"GetType": () => ({ "generic": "food", "specific": "grain" }),
	"GetDiminishingReturns": () => 1,
	"TakeResources": (amount) => {
		return {
			"amount": amount,
			"exhausted": false
		};
	},
	"RemoveGatherer": () => {}

});
TS_ASSERT(cmpResourceGatherer.StartGathering(supplyID));
cmpTimer.OnUpdate({ "turnLength": 10 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 10, "max": 10 }]);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 10, "max": 10 }]);


// Test that starting to gather twice does not add resources at twice the speed.
reset();
AddMock(supplyID, IID_ResourceSupply, {
	"AddActiveGatherer": () => true,
	"GetCurrentAmount": () => 3,
	"GetType": () => ({ "generic": "food", "specific": "grain" }),
	"GetDiminishingReturns": () => 1,
	"TakeResources": (amount) => {
		return {
			"amount": amount,
			"exhausted": false
		};
	},
	"RemoveGatherer": () => {}

});
TS_ASSERT(cmpResourceGatherer.StartGathering(supplyID));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 1, "max": 10 }]);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 2, "max": 10 }]);
TS_ASSERT(cmpResourceGatherer.StartGathering(supplyID));
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 3, "max": 10 }]);
cmpTimer.OnUpdate({ "turnLength": 1 });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceGatherer.GetCarryingStatus(),
	[{ "type": "food", "amount": 4, "max": 10 }]);
