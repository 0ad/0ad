var PETRA = function(m)
{

/**
 * Manage the trade
 */

m.TradeManager = function(Config)
{
	this.Config = Config;
	this.tradeRoute = undefined;
	this.targetNumTraders = this.Config.Economy.targetNumTraders;
};

m.TradeManager.prototype.init = function(gameState)
{
	this.traders = gameState.getOwnUnits().filter(API3.Filters.byMetadata(PlayerID, "role", "trader"));
	this.traders.allowQuickIter();
	this.traders.registerUpdates();
};

m.TradeManager.prototype.setTradeRoute = function(market1, market2)
{
	this.tradeRoute = { "source": market1, "target": market2 };
};

m.TradeManager.prototype.hasTradeRoute = function()
{
	return (this.tradeRoute !== undefined);
};

m.TradeManager.prototype.assignTrader = function(ent)
{
	unit.setMetadata(PlayerID, "role", "trader");
	this.traders.updateEnt(unit);
};

// TODO take trader ships into account
m.TradeManager.prototype.trainMoreTraders = function(gameState, queues)
{
	var numTraders = gameState.countEntitiesAndQueuedByType(gameState.applyCiv("units/{civ}_support_trader"), true);
	if (numTraders < this.targetNumTraders && queues.trader.countQueuedUnits() === 0)
	{
		var template = gameState.applyCiv("units/{civ}_support_trader")
		queues.trader.addItem(new m.TrainingPlan(gameState, template, { "role": "trader", "base": 0 }, 1, 1));
	}
};

m.TradeManager.prototype.update = function(gameState, queues)
{
	if (!this.tradeRoute)
		return;
	var self = this;
	if (gameState.ai.playedTurn % 100 === 9)
		this.setTradingGoods(gameState);
	this.trainMoreTraders(gameState, queues);
	this.traders.forEach(function(ent) { self.updateTrader(ent) });
};

// TODO deal with garrisoned trader & check if the trade route (i.e. its markets) still exist
m.TradeManager.prototype.updateTrader = function(ent)
{
	if (!ent.isIdle() || !ent.position())
		return;

	if (API3.SquareVectorDistance(this.tradeRoute.target.position(), ent.position()) > API3.SquareVectorDistance(this.tradeRoute.source.position(), ent.position()))
		ent.tradeRoute(this.tradeRoute.target, this.tradeRoute.source);
	else
		ent.tradeRoute(this.tradeRoute.source, this.tradeRoute.target);
};

m.TradeManager.prototype.setTradingGoods = function(gameState)
{
	var tradingGoods = { "food": 0, "wood": 0, "stone": 0, "metal": 0 };
	// first, try to anticipate future needs 
	var stocks = gameState.ai.HQ.GetTotalResourceLevel(gameState);
	var remaining = 100;
	this.targetNumTraders = this.Config.Economy.targetNumTraders;
	for (var type in stocks)
	{
		if (type == "food")
			continue;
		if (stocks[type] < 200)
		{
			tradingGoods[type] = 20;
			this.targetNumTraders += 2;
		}
		else if (stocks[type] < 500)
		{
			tradingGoods[type] = 10;
			this.targetNumTraders += 1;
		}
		remaining -= tradingGoods[type];
	}

	// then add what is needed now
	var mainNeed = Math.floor(remaining * 70 / 100)
	var nextNeed = remaining - mainNeed;

	var mostNeeded = gameState.ai.HQ.pickMostNeededResources(gameState);
	tradingGoods[mostNeeded[0]] += mainNeed;
	tradingGoods[mostNeeded[1]] += nextNeed;
	Engine.PostCommand(PlayerID, {"type": "set-trading-goods", "tradingGoods": tradingGoods});
	if (this.Config.debug == 2)
		warn(" trading goods set to " + uneval(tradingGoods));
};

return m;
}(PETRA);
