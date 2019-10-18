/**
 * This class manages the trading good selection, idle trader information and barter panel.
 */
class TradeDialog
{
	constructor(playerViewControl)
	{
		this.tradePanel = new this.TradePanel();
		this.barterPanel = new this.BarterPanel();

		this.tradeDialogPanel = Engine.GetGUIObjectByName("tradeDialogPanel");

		registerPlayersInitHandler(this.onPlayersInit.bind(this));
		Engine.GetGUIObjectByName("closeTrade").onPress = this.close.bind(this);

		registerSimulationUpdateHandler(this.updateIfOpen.bind(this))
		registerEntitySelectionChangeHandler(this.updateIfOpen.bind(this));
		playerViewControl.registerViewedPlayerChangeHandler(this.onViewedPlayerChange.bind(this));
	}

	open()
	{
		closeOpenDialogs();

		if (g_ViewedPlayer < 1)
			return;

		this.updatePanels();
		this.tradeDialogPanel.hidden = false;
	}

	close()
	{
		this.tradeDialogPanel.hidden = true;
	}

	isOpen()
	{
		return !this.tradeDialogPanel.hidden;
	}

	onViewedPlayerChange()
	{
		if (g_ViewedPlayer >= 1)
			this.updateIfOpen();
		else
			this.close();
	}

	toggle()
	{
		let open = this.isOpen();
		closeOpenDialogs();

		if (!open)
			this.open();
	}

	updateIfOpen()
	{
		if (this.isOpen())
			this.updatePanels();
	}

	updatePanels()
	{
		this.barterPanel.update();
		this.tradePanel.update();
	}

	onPlayersInit()
	{
		let size = this.tradeDialogPanel.size;

		let width = 1/2 * Math.max(
			TradeDialog.prototype.BarterPanel.getWidthOffset(),
			TradeDialog.prototype.TradePanel.getWidthOffset());

		size.left -= width;
		size.right += width;
		this.tradeDialogPanel.size = size;
	}
}
