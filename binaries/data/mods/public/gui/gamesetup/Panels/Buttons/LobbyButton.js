class LobbyButton
{
	constructor()
	{
		this.lobbyButton = Engine.GetGUIObjectByName("lobbyButton");
		this.lobbyButton.tooltip = this.Tooltip;
		this.lobbyButton.onPress = this.onPress.bind(this);
		this.lobbyButton.hidden = !Engine.HasXmppClient();
	}

	onPress()
	{
		if (Engine.HasXmppClient())
			Engine.PushGuiPage("page_lobby.xml", { "dialog": true });
	}
}

LobbyButton.prototype.Tooltip =
	translate("Show the multiplayer lobby in a dialog window.");
