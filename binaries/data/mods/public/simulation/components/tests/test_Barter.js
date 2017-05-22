Engine.LoadHelperScript("Player.js");
Engine.LoadComponentScript("interfaces/Barter.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/Player.js");
Engine.LoadComponentScript("interfaces/StatisticsTracker.js");
Engine.LoadComponentScript("interfaces/Timer.js");
Engine.LoadComponentScript("Barter.js");

// truePrice. Use the same for each resource for those tests.
const truePrice = 110;

Resources = {
	"GetCodes": () => ["wood", "stone", "metal"],
	"GetResource": (resource) => ({ "truePrice": truePrice })
};

const playerID = 1;
const playerEnt = 11;

let timerActivated = false;
let bought = 0;
let sold = 0;
let multiplier = {
	"buy": {
		"wood": 1.0,
		"stone": 1.0,
		"metal": 1.0
	},
	"sell": {
		"wood": 1.0,
		"stone": 1.0,
		"metal": 1.0
	}
};

AddMock(SYSTEM_ENTITY, IID_Timer, {
	"CancelTimer": id => { timerActivated = false; },
	"SetInterval": (ent, iid, funcname, time, repeattime, data) => {
		TS_ASSERT_EQUALS(time, cmpBarter.RESTORE_TIMER_INTERVAL);
		TS_ASSERT_EQUALS(repeattime, cmpBarter.RESTORE_TIMER_INTERVAL);
		timerActivated = true;
		return 7;
	}
});

let cmpBarter = ConstructComponent(SYSTEM_ENTITY, "Barter");

// Init
TS_ASSERT_EQUALS(cmpBarter.restoreTimer, undefined);
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.priceDifferences, { "wood": 0, "stone": 0, "metal": 0 });

AddMock(playerEnt, IID_Player, {
	"GetPlayerID": () => 1,
	"TrySubtractResources": amounts => {
		sold = amounts[Object.keys(amounts)[0]];
		return true;
	},
	"AddResource": (type, amount) => {
		bought = amount;
		return true;
	},
	"GetBarterMultiplier": () => multiplier
});

AddMock(SYSTEM_ENTITY, IID_RangeManager, {
	"GetEntitiesByPlayer": (id) => id == playerID ? [62, 60, 61, 63] : []
});

AddMock(60, IID_Identity, {
	"HasClass": (cl) => true
});

AddMock(60, IID_Foundation, {});

// GetPrices
cmpBarter.priceDifferences = { "wood": 8, "stone": 0, "metal": 0 };
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.GetPrices(playerEnt).buy, {
	"wood": truePrice * (100 + 8 + cmpBarter.CONSTANT_DIFFERENCE) / 100,
	"stone": truePrice * (100 + cmpBarter.CONSTANT_DIFFERENCE) / 100,
	"metal": truePrice * (100 + cmpBarter.CONSTANT_DIFFERENCE) / 100
});
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.GetPrices(playerEnt).sell, {
	"wood": truePrice * (100 + 8 - cmpBarter.CONSTANT_DIFFERENCE) / 100,
	"stone": truePrice * (100 - cmpBarter.CONSTANT_DIFFERENCE) / 100,
	"metal": truePrice * (100 - cmpBarter.CONSTANT_DIFFERENCE) / 100
});

multiplier.buy.stone = 2.0;
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.GetPrices(playerEnt).buy, {
	"wood": truePrice * (100 + 8 + cmpBarter.CONSTANT_DIFFERENCE) / 100,
	"stone": truePrice * (100 + cmpBarter.CONSTANT_DIFFERENCE) * 2.0 / 100,
	"metal": truePrice * (100 + cmpBarter.CONSTANT_DIFFERENCE) / 100
});
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.GetPrices(playerEnt).sell, {
	"wood": truePrice * (100 + 8 - cmpBarter.CONSTANT_DIFFERENCE) / 100,
	"stone": truePrice * (100 - cmpBarter.CONSTANT_DIFFERENCE) / 100,
	"metal": truePrice * (100 - cmpBarter.CONSTANT_DIFFERENCE) / 100
});
multiplier.buy.stone = 1.0;

// PlayerHasMarket
TS_ASSERT(!cmpBarter.PlayerHasMarket(11));

AddMock(61, IID_Identity, {
	"HasClass": (cl) => true
});

TS_ASSERT(cmpBarter.PlayerHasMarket(11));

// ExchangeResources
// Price differences magnitude are caped by 99 - CONSTANT_DIFFERENCE.
// If we have a bigger DIFFERENCE_PER_DEAL than 99 - CONSTANT_DIFFERENCE at each deal we reach the cap.
TS_ASSERT(cmpBarter.DIFFERENCE_PER_DEAL < 99 - cmpBarter.CONSTANT_DIFFERENCE);

cmpBarter.priceDifferences = { "wood": 0, "stone": 0, "metal": 0 };
cmpBarter.ExchangeResources(11, "wood", "stone", 100);
TS_ASSERT_EQUALS(cmpBarter.restoreTimer, 7);
TS_ASSERT(timerActivated);
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.priceDifferences, { "wood": -cmpBarter.DIFFERENCE_PER_DEAL, "stone": cmpBarter.DIFFERENCE_PER_DEAL, "metal": 0 });
TS_ASSERT_EQUALS(sold, 100);
TS_ASSERT_EQUALS(bought, Math.round(100 * (100 - cmpBarter.CONSTANT_DIFFERENCE + 0) / (100 + cmpBarter.CONSTANT_DIFFERENCE + 0)));

// With an amount which is not a multiple of 100, price differences are not updated. That sounds wrong.
cmpBarter.priceDifferences = { "wood": 0, "stone": 0, "metal": 0 };
cmpBarter.ExchangeResources(11, "wood", "stone", 40);
TS_ASSERT_EQUALS(cmpBarter.restoreTimer, 7);
TS_ASSERT(timerActivated);
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.priceDifferences, { "wood": 0, "stone": 0, "metal": 0 });
TS_ASSERT_EQUALS(sold, 40);
TS_ASSERT_EQUALS(bought, Math.round(40 * (100 - cmpBarter.CONSTANT_DIFFERENCE + 0) / (100 + cmpBarter.CONSTANT_DIFFERENCE + 0)));

cmpBarter.priceDifferences = { "wood": 0, "stone": 99 - cmpBarter.CONSTANT_DIFFERENCE, "metal": 0 };
cmpBarter.ExchangeResources(11, "wood", "stone", 100);
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.priceDifferences, { "wood": -cmpBarter.DIFFERENCE_PER_DEAL, "stone": 99 - cmpBarter.CONSTANT_DIFFERENCE, "metal": 0 });
TS_ASSERT_EQUALS(bought, Math.round(100 * (100 - cmpBarter.CONSTANT_DIFFERENCE + 0) / (100 + cmpBarter.CONSTANT_DIFFERENCE + 99 - cmpBarter.CONSTANT_DIFFERENCE)));

cmpBarter.priceDifferences = { "wood": -99 + cmpBarter.CONSTANT_DIFFERENCE, "stone": 0, "metal": 0 };
cmpBarter.ExchangeResources(11, "wood", "stone", 100);
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.priceDifferences, { "wood": -99 + cmpBarter.CONSTANT_DIFFERENCE, "stone": cmpBarter.DIFFERENCE_PER_DEAL, "metal": 0 });
TS_ASSERT_EQUALS(bought, Math.round(100 * (100 - cmpBarter.CONSTANT_DIFFERENCE - 99 + cmpBarter.CONSTANT_DIFFERENCE) / (100 + cmpBarter.CONSTANT_DIFFERENCE + 0)));

cmpBarter.priceDifferences = { "wood": 0, "stone": 0, "metal": 0 };
cmpBarter.restoreTimer = undefined;
timerActivated = false;
AddMock(playerEnt, IID_Player, {
	"GetPlayerID": () => 1,
	"TrySubtractResources": () => false,
	"AddResource": () => {},
	"GetBarterMultiplier": () => (multiplier)
});
cmpBarter.ExchangeResources(11, "wood", "stone", 100);

// It seems useless to try to activate the timer if we don't barter any resources.
TS_ASSERT_EQUALS(cmpBarter.restoreTimer, 7);
TS_ASSERT(timerActivated);

TS_ASSERT_UNEVAL_EQUALS(cmpBarter.priceDifferences, { "wood": 0, "stone": 0, "metal": 0 });

DeleteMock(playerEnt, IID_Player);

// ProgressTimeout
// Price difference restoration magnitude is capped by DIFFERENCE_RESTORE.

AddMock(playerEnt, IID_Player, {
	"GetPlayerID": () => 1,
	"TrySubtractResources": () => true,
	"AddResource": () => {}
});

timerActivated = true;
cmpBarter.restoreTimer = 7;
cmpBarter.priceDifferences = { "wood": -cmpBarter.DIFFERENCE_RESTORE, "stone": -1 - cmpBarter.DIFFERENCE_RESTORE, "metal": cmpBarter.DIFFERENCE_RESTORE };
cmpBarter.ProgressTimeout();
TS_ASSERT_EQUALS(cmpBarter.restoreTimer, 7);
TS_ASSERT(timerActivated);
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.priceDifferences, { "wood": 0, "stone": -1, "metal": 0 });

cmpBarter.restoreTimer = 7;
cmpBarter.priceDifferences = { "wood": -cmpBarter.DIFFERENCE_RESTORE, "stone": cmpBarter.DIFFERENCE_RESTORE / 2, "metal": cmpBarter.DIFFERENCE_RESTORE };
cmpBarter.ProgressTimeout();
TS_ASSERT_EQUALS(cmpBarter.restoreTimer, undefined);
TS_ASSERT(!timerActivated);
TS_ASSERT_UNEVAL_EQUALS(cmpBarter.priceDifferences, { "wood": 0, "stone": 0, "metal": 0 });
