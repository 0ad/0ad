/**
 * This class handles the buttons used to determine selection of goods carried by traders.
 */
class TradeButtonManager
{
	constructor()
	{
		if (!TradeButtonManager.IsAvailable())
			throw "TradeButtonManager instantiated with no tradeable resources or too few buttons!";

		// Assume that the simulation state will always follow the GUI
		this.tradingGoods = Engine.GuiInterfaceCall("GetTradingGoods", g_ViewedPlayer);

		let resourceCodes = g_ResourceData.GetTradableCodes();
		this.selectedResource = resourceCodes[0];
		this.buttons = resourceCodes.map((resCode, i) => new TradeButton(this, resCode, i));

		hideRemaining("tradeResources", resourceCodes.length);

		this.tradeHelp = Engine.GetGUIObjectByName("tradeHelp");
		this.tradeHelp.hidden = false;
	}

	update()
	{
		this.tradeHelp.tooltip = colorizeHotkey(translate(this.TradeSwapTooltip), "session.fulltradeswap");

		let enabled = controlsPlayer(g_ViewedPlayer);
		for (let button of this.buttons)
			button.update(enabled, this.selectedResource, this.tradingGoods);
	}

	selectResource(resourceCode)
	{
		if (Engine.HotkeyIsPressed("session.fulltradeswap"))
			this.fullTradeSwap(resourceCode);

		this.selectedResource = resourceCode;
		this.update();
	}

	fullTradeSwap(resourceCode)
	{
		for (let resCode in this.tradingGoods)
			this.tradingGoods[resCode] = 0;

		this.tradingGoods[resourceCode] = 100;
		this.setTradingGoods();
	}

	changeResourceAmount(resourceCode, amount)
	{
		this.tradingGoods[this.selectedResource] -= amount;
		this.tradingGoods[resourceCode] += amount;

		this.setTradingGoods();
		this.update();
	}

	setTradingGoods()
	{
		Engine.PostNetworkCommand({
			"type": "set-trading-goods",
			"tradingGoods": this.tradingGoods
		});
	}
}

TradeButtonManager.IsAvailable = function()
{
	let resourceCount = g_ResourceData.GetTradableCodes().length;
	return resourceCount && resourceCount <= Engine.GetGUIObjectByName("tradeResources").children.length;
};

TradeButtonManager.prototype.TradeSwapTooltip =
	markForTranslation("Select one type of goods you want to modify by clicking on it, and then use the arrows of the other types to modify their shares. You can also press %(hotkey)s while selecting one type of goods to bring its share to 100%%.");
