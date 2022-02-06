Resources = {
	"GetCodes": () => ["food", "metal"],
	"GetTradableCodes": () => ["food", "metal"],
	"GetBarterableCodes": () => ["food", "metal"],
	"GetResource": () => ({}),
	"BuildSchema": (type) => {
		let schema = "";
		for (let res of Resources.GetCodes())
			schema +=
				"<optional>" +
					"<element name='" + res + "'>" +
						"<ref name='" + type + "'/>" +
					"</element>" +
				"</optional>";
		return "<interleave>" + schema + "</interleave>";
	}
};

Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/Upkeep.js");
Engine.LoadComponentScript("Player.js");
Engine.LoadComponentScript("Timer.js");
Engine.LoadComponentScript("Upkeep.js");

// Upkeep requires this function to be defined before the component is built.
let ApplyValueModificationsToEntity = (valueName, currentValue, entity) => currentValue;
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
let testedEnt = 10;
let turnLength = 0.2;
let playerEnt = 1;
let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer", {});

let cmpUpkeep = ConstructComponent(testedEnt, "Upkeep", {
	"Interval": "200",
	"Rates": {
		"food": "0",
		"metal": "0"
	}
});

let cmpPlayer = ConstructComponent(playerEnt, "Player", {
	"SpyCostMultiplier": "1",
	"BarterMultiplier": {
		"Buy": {
			"food": "1",
			"metal": "1"
		},
		"Sell": {
			"food": "1",
			"metal": "1"
		}
	},
	"Formations": { "_string": "" },
});

let QueryOwnerInterface = () => cmpPlayer;
Engine.RegisterGlobal("QueryOwnerInterface", QueryOwnerInterface);
Engine.RegisterGlobal("QueryPlayerIDInterface", () => null);
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 300, "metal": 300 });
TS_ASSERT_EQUALS(cmpUpkeep.GetInterval(), 200);

// Since there is no rate > 0, nothing should change.
TS_ASSERT_UNEVAL_EQUALS(cmpUpkeep.GetRates(), {});
TS_ASSERT_EQUALS(cmpUpkeep.ComputeRates(), false);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 300, "metal": 300 });

// Test that only requiring food works.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "Upkeep/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
// Calling OnValueModification will reset the timer, which can then be called, thus decreasing the resources of the player.
cmpUpkeep.OnValueModification({ "component": "Upkeep" });
TS_ASSERT_UNEVAL_EQUALS(cmpUpkeep.GetRates(), { "food": 1 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 299, "metal": 300 });
TS_ASSERT_EQUALS(cmpUpkeep.ComputeRates(), true);

// Reset the pay modification.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => currentValue;
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpUpkeep.OnValueModification({ "component": "Upkeep" });
TS_ASSERT_UNEVAL_EQUALS(cmpUpkeep.GetRates(), {});
TS_ASSERT_EQUALS(cmpUpkeep.ComputeRates(), false);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 299, "metal": 300 });

ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "Upkeep/Interval")
		return currentValue + 200;
	if (valueName == "Upkeep/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpUpkeep.OnValueModification({ "component": "Upkeep" });
TS_ASSERT_EQUALS(cmpUpkeep.GetInterval(), 400);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 299, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 298, "metal": 300 });

// Interval becomes a normal timer, thus cancelled after the first execution.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "Upkeep/Interval")
		return currentValue - 200;
	if (valueName == "Upkeep/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpUpkeep.OnValueModification({ "component": "Upkeep" });
TS_ASSERT_EQUALS(cmpUpkeep.GetInterval(), 0);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 298, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 297, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 297, "metal": 300 });

// Timer became invalidated, check whether it's recreated properly after that.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "Upkeep/Interval")
		return currentValue - 100;
	if (valueName == "Upkeep/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpUpkeep.OnValueModification({ "component": "Upkeep" });
TS_ASSERT_EQUALS(cmpUpkeep.GetInterval(), 100);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 295, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 293, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 291, "metal": 300 });

// Value is now invalid, timer should be cancelled.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "Upkeep/Interval")
		return currentValue - 201;
	if (valueName == "Upkeep/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpUpkeep.OnValueModification({ "component": "Upkeep" });
TS_ASSERT_EQUALS(cmpUpkeep.GetInterval(), -1);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 291, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 291, "metal": 300 });

// Timer became invalidated, check whether it's recreated properly after that.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "Upkeep/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpUpkeep.OnValueModification({ "component": "Upkeep" });
TS_ASSERT_EQUALS(cmpUpkeep.GetInterval(), 200);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 290, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 289, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 288, "metal": 300 });

// Test multiple upkeep resources.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "Upkeep/Rates/food")
		return currentValue + 1;
	if (valueName == "Upkeep/Rates/metal")
		return currentValue + 2;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpUpkeep.OnValueModification({ "component": "Upkeep" });
TS_ASSERT_EQUALS(cmpUpkeep.GetInterval(), 200);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 287, "metal": 298 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 286, "metal": 296 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 285, "metal": 294 });

// Test we don't go into negative resources.
let cmpGUI = AddMock(SYSTEM_ENTITY, IID_GuiInterface, {
	"PushNotification": () => {}
});
let notificationSpy = new Spy(cmpGUI, "PushNotification");
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "Upkeep/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpUpkeep.OnValueModification({ "component": "Upkeep" });
TS_ASSERT_EQUALS(cmpUpkeep.GetInterval(), 200);
cmpTimer.OnUpdate({ "turnLength": turnLength * 285 });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 0, "metal": 294 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 0, "metal": 294 });
TS_ASSERT_EQUALS(notificationSpy._called, 1);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 0, "metal": 294 });
TS_ASSERT_EQUALS(notificationSpy._called, 2);
