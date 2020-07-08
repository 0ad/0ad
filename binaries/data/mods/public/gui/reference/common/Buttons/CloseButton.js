class CloseButton
{
	constructor(parentPage)
	{
		this.closeButton = Engine.GetGUIObjectByName("closeButton");
		this.closeButton.onPress = parentPage.closePage.bind(parentPage);
		this.closeButton.caption = this.Caption;
		this.closeButton.tooltip = colorizeHotkey(parentPage.CloseButtonTooltip, this.Hotkey);
	}
}

CloseButton.prototype.Caption =
	translate("Close");

CloseButton.prototype.Hotkey =
	"close";
