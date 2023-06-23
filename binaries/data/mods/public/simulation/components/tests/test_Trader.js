Engine.LoadComponentScript("interfaces/Diplomacy.js");
Engine.LoadComponentScript("interfaces/Foundation.js");
Engine.LoadComponentScript("interfaces/Market.js");
Engine.LoadComponentScript("interfaces/Trader.js");
Engine.LoadComponentScript("Trader.js");
Engine.LoadHelperScript("Player.js");

const player = 1;
const enemy = 2;
const ally = 3;
const trader = 5;
const ownMarket = 6;
const otherMarket = 7;

AddMock(SYSTEM_ENTITY, IID_PlayerManager, {
        "GetPlayerByID": (i) => i
});

const cmpTrader = ConstructComponent(trader, "Trader", {
	"GainMultiplier": 0.8,
	"GarrisonGainMultiplier": 0.2
});

AddMock(trader, IID_Identity, {
	"HasClass": (cls) => cls === "Organic",
});

AddMock(ownMarket, IID_Market, {
	"HasType": (t) => t === "land"
});

AddMock(trader, IID_Ownership, {
	"GetOwner": () => player
});

AddMock(ownMarket, IID_Ownership, {
	"GetOwner": () => player
});

AddMock(player, IID_Player, {
	"GetPlayerID": () => player,
});

AddMock(player, IID_Diplomacy, {
	"IsEnemy": (id) => id === enemy,
});

TS_ASSERT(cmpTrader.CanTrade(ownMarket));

// We can't trade with our enemy.
AddMock(enemy, IID_Player, {
	"GetPlayerID": () => enemy,
});

AddMock(otherMarket, IID_Ownership, {
	"GetOwner": () => enemy
});

AddMock(otherMarket, IID_Market, {
	"HasType": (t) => t === "land"
});

TS_ASSERT(!cmpTrader.CanTrade(otherMarket));

// But can with our ally.
AddMock(ally, IID_Player, {
	"GetPlayerID": () => ally,
});

AddMock(otherMarket, IID_Ownership, {
	"GetOwner": () => ally
});

TS_ASSERT(cmpTrader.CanTrade(otherMarket));

// Land traders shouldn't try to trade with naval-only markets.
AddMock(trader, IID_Identity, {
	"HasClass": (cls) => cls === "Organic",
});

AddMock(ownMarket, IID_Market, {
	"HasType": (t) => t === "naval"
});

TS_ASSERT(!cmpTrader.CanTrade(ownMarket));

// And Ships shouldn't try to trade with land markets.
AddMock(trader, IID_Identity, {
	"HasClass": (cls) => cls === "Ship",
});

AddMock(ownMarket, IID_Market, {
	"HasType": (t) => t === "land"
});

TS_ASSERT(!cmpTrader.CanTrade(ownMarket));
