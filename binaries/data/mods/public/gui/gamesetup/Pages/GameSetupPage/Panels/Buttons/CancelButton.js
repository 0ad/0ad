class CancelButton
{
	constructor(setupWindow, startGameButton, readyButton)
	{
		this.setupWindow = setupWindow;

		this.buttonPositions = Engine.GetGUIObjectByName("bottomRightPanel").children;

		this.cancelButton = Engine.GetGUIObjectByName("cancelButton");
		this.cancelButton.caption = this.Caption;
		this.cancelButton.tooltip = Engine.HasXmppClient() ? this.TooltipLobby : this.TooltipMenu;
		this.cancelButton.onPress = setupWindow.closePage.bind(setupWindow);
	}
}

CancelButton.prototype.Caption =
	translate("Back");

CancelButton.prototype.TooltipLobby =
	translate("Return to the lobby.");

CancelButton.prototype.TooltipMenu =
	translate("Return to the main menu.");
