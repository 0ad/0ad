/**
 * This class owns the BarterButtonManager and the accessory elements for bartering.
 */
TradeDialog.prototype.BarterPanel = class
{
	constructor()
	{
		this.barterResources = Engine.GetGUIObjectByName("barterResources");

		let isAvailable = BarterButtonManager.IsAvailable(this.barterResources);
		if (isAvailable)
			this.barterButtonManager = new BarterButtonManager(this.barterResources);

		Engine.GetGUIObjectByName("tradeDialogPanelBarter").hidden = !isAvailable;

		this.barterNoMarketsMessage = Engine.GetGUIObjectByName("barterNoMarketsMessage");
		this.barterHelp = Engine.GetGUIObjectByName("barterHelp");
	}

	update()
	{
		let playerState = GetSimState().players[g_ViewedPlayer];
		let canBarter = playerState && playerState.canBarter;

		this.barterButtonManager.setViewedPlayer(g_ViewedPlayer);
		this.barterButtonManager.update();
		this.barterNoMarketsMessage.hidden = canBarter;
		this.barterResources.hidden = !canBarter;
		this.barterHelp.hidden = !canBarter;
		this.barterHelp.tooltip = sprintf(
			translate(this.InstructionsTooltip), {
				"quantity": this.BarterResourceSellQuantity,
				"hotkey": colorizeHotkey("%(hotkey)s", "session.massbarter"),
				"multiplier": this.Multiplier
			});
	}
};

TradeDialog.prototype.BarterPanel.getWidthOffset = function()
{
	return BarterButton.getWidth(Engine.GetGUIObjectByName("barterResources")) * g_ResourceData.GetBarterableCodes().length;
};

TradeDialog.prototype.BarterPanel.prototype.InstructionsTooltip =
	markForTranslation("Start by selecting the resource you wish to sell from the upper row. For each time the lower buttons are pressed, %(quantity)s of the upper resource will be sold for the displayed quantity of the lower. Press and hold %(hotkey)s to temporarily multiply the traded amount by %(multiplier)s.");
