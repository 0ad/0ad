class CancelButton
{
	constructor(gamesetupPage, startGameButton, readyButton, gameSettingsControl)
	{
		this.gamesetupPage = gamesetupPage;
		this.startGameButton = startGameButton;
		this.readyButton = readyButton;
		this.gameSettingsControl = gameSettingsControl;

		this.cancelButtonResizeHandlers = new Set();

		this.buttonPositions = Engine.GetGUIObjectByName("bottomRightPanel").children;

		this.cancelButton = Engine.GetGUIObjectByName("cancelButton");
		this.cancelButton.caption = this.Caption;
		this.cancelButton.tooltip = Engine.HasXmppClient() ? this.TooltipLobby : this.TooltipMenu;
		this.cancelButton.onPress = gamesetupPage.closePage.bind(gamesetupPage);

		readyButton.registerButtonHiddenChangeHandler(this.onNeighborButtonHiddenChange.bind(this));
		startGameButton.registerButtonHiddenChangeHandler(this.onNeighborButtonHiddenChange.bind(this));
	}

	registerCancelButtonResizeHandler(handler)
	{
		this.cancelButtonResizeHandlers.add(handler);
	}

	onNeighborButtonHiddenChange()
	{
		this.cancelButton.size = this.buttonPositions[
			this.buttonPositions[1].children.every(button => button.hidden) ? 1 : 0].size;

		for (let handler of this.cancelButtonResizeHandlers)
			handler(this.cancelButton);
	}
}

CancelButton.prototype.Caption =
	translate("Back");

CancelButton.prototype.TooltipLobby =
	translate("Return to the lobby.");

CancelButton.prototype.TooltipMenu =
	translate("Return to the main menu.");

CancelButton.prototype.Margin = 0;
