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

Engine.LoadComponentScript("interfaces/ResourceTrickle.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("Player.js");
Engine.LoadComponentScript("ResourceTrickle.js");
Engine.LoadComponentScript("Timer.js");

// Resource Trickle requires this function to be defined before the component is built.
let ApplyValueModificationsToEntity = (valueName, currentValue, entity) => currentValue;
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
let wonderEnt = 1;
let turnLength = 0.2;
let playerEnt = 10;
let cmpTimer = ConstructComponent(SYSTEM_ENTITY, "Timer", {});

let cmpResourceTrickle = ConstructComponent(wonderEnt, "ResourceTrickle", {
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
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 300, "metal": 300 });
TS_ASSERT_EQUALS(cmpResourceTrickle.GetInterval(), 200);

// Since there is no rate > 0, nothing should change.
TS_ASSERT_UNEVAL_EQUALS(cmpResourceTrickle.GetRates(), {});
TS_ASSERT_EQUALS(cmpResourceTrickle.ComputeRates(), false);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 300, "metal": 300 });

// Test that only trickling food works.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "ResourceTrickle/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
// Calling OnValueModification will reset the timer, which can then be called, thus increasing the resources of the player.
cmpResourceTrickle.OnValueModification({ "component": "ResourceTrickle" });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceTrickle.GetRates(), { "food": 1 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 301, "metal": 300 });
TS_ASSERT_EQUALS(cmpResourceTrickle.ComputeRates(), true);

// Reset the trickle modification.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => currentValue;
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpResourceTrickle.OnValueModification({ "component": "ResourceTrickle" });
TS_ASSERT_UNEVAL_EQUALS(cmpResourceTrickle.GetRates(), {});
TS_ASSERT_EQUALS(cmpResourceTrickle.ComputeRates(), false);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 301, "metal": 300 });

ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "ResourceTrickle/Interval")
		return currentValue + 200;
	if (valueName == "ResourceTrickle/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpResourceTrickle.OnValueModification({ "component": "ResourceTrickle" });
TS_ASSERT_EQUALS(cmpResourceTrickle.GetInterval(), 400);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 301, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 302, "metal": 300 });

// Interval becomes a normal timer, thus cancelled after the first execution.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "ResourceTrickle/Interval")
		return currentValue - 200;
	if (valueName == "ResourceTrickle/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpResourceTrickle.OnValueModification({ "component": "ResourceTrickle" });
TS_ASSERT_EQUALS(cmpResourceTrickle.GetInterval(), 0);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 302, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 303, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 303, "metal": 300 });

// Timer became invalidated, check whether it's recreated properly after that.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "ResourceTrickle/Interval")
		return currentValue - 100;
	if (valueName == "ResourceTrickle/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpResourceTrickle.OnValueModification({ "component": "ResourceTrickle" });
TS_ASSERT_EQUALS(cmpResourceTrickle.GetInterval(), 100);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 305, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 307, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 309, "metal": 300 });

// Value is now invalid, timer should be cancelled.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "ResourceTrickle/Interval")
		return currentValue - 201;
	if (valueName == "ResourceTrickle/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpResourceTrickle.OnValueModification({ "component": "ResourceTrickle" });
TS_ASSERT_EQUALS(cmpResourceTrickle.GetInterval(), -1);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 309, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 309, "metal": 300 });

// Timer became invalidated, check whether it's recreated properly after that.
ApplyValueModificationsToEntity = (valueName, currentValue, entity) => {
	if (valueName == "ResourceTrickle/Rates/food")
		return currentValue + 1;

	return currentValue;
};
Engine.RegisterGlobal("ApplyValueModificationsToEntity", ApplyValueModificationsToEntity);
cmpResourceTrickle.OnValueModification({ "component": "ResourceTrickle" });
TS_ASSERT_EQUALS(cmpResourceTrickle.GetInterval(), 200);
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 310, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 311, "metal": 300 });
cmpTimer.OnUpdate({ "turnLength": turnLength });
TS_ASSERT_UNEVAL_EQUALS(cmpPlayer.GetResourceCounts(), { "food": 312, "metal": 300 });
