/**
 * This class owns the TradeButtonManager and trader information texts.
 */
TradeDialog.prototype.TradePanel = class
{
	constructor()
	{
		if (TradeButtonManager.IsAvailable())
			this.tradeButtonManager = new TradeButtonManager();

		this.traderStatusText = new TraderStatusText();
	}

	update()
	{
		for (let name in this)
			this[name].update();
	}
};

TradeDialog.prototype.TradePanel.getWidthOffset = function()
{
	return TradeButton.getWidth() * g_ResourceData.GetTradableCodes().length;
};
