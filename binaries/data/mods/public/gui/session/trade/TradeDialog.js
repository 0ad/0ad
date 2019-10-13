/**
 * This class manages the trading good selection, idle trader information and barter panel.
 */
class TradeDialog
{
	constructor()
	{
		this.tradePanel = new this.TradePanel();
		this.barterPanel = new this.BarterPanel();

		this.tradeDialogPanel = Engine.GetGUIObjectByName("tradeDialogPanel");

		Engine.GetGUIObjectByName("closeTrade").onPress = this.close.bind(this);
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

	toggle()
	{
		let open = this.isOpen();
		closeOpenDialogs();

		if (!open)
			this.open();
	}

	update()
	{
		if (!this.isOpen())
			return;

		this.updatePanels();
	}

	updatePanels()
	{
		this.barterPanel.update();
		this.tradePanel.update();
	}

	resize()
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
