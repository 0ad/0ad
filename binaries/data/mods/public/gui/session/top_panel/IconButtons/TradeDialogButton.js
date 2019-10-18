/**
 * This class handles the button which opens the diplomacy dialog.
 */
class TradeDialogButton
{
	constructor(playerViewControl, tradeDialog)
	{
		this.tradeButton = Engine.GetGUIObjectByName("tradeButton");
		this.tradeButton.onPress = tradeDialog.toggle.bind(tradeDialog);
		this.isAvailable =
			g_ResourceData.GetTradableCodes().length ||
			g_ResourceData.GetBarterableCodes().length;

		playerViewControl.registerViewedPlayerChangeHandler(this.onViewedPlayerChange.bind(this));
		registerHotkeyChangeHandler(this.onHotkeyChange.bind(this));
	}

	onHotkeyChange()
	{
		this.tradeButton.tooltip =
			colorizeHotkey("%(hotkey)s" + " ", "session.gui.barter.toggle") +
			translate(this.Tooltip);
	}

	onViewedPlayerChange()
	{
		this.tradeButton.hidden = g_ViewedPlayer < 1 || !this.isAvailable;
	}
}

TradeDialogButton.prototype.Tooltip = markForTranslation("Barter & Trade");
