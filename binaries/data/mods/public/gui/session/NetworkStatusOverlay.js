/**
 * This class shows an overlay if the game is stalled due to some clients not being
 * ready to simulate yet, and in that case provides specifics to that reason.
 */
class NetworkStatusOverlay
{
	constructor()
	{
		this.netStatus = Engine.GetGUIObjectByName("netStatus");
		this.loadingClientsText = Engine.GetGUIObjectByName("loadingClientsText");

		Engine.GetGUIObjectByName("disconnectedExitButton").onPress = endGame;

		registerNetworkStatusChangeHandler(this.onNetStatusMessage.bind(this));
		registerClientsLoadingHandler(this.onClientsLoadingMessage.bind(this));
	}

	onNetStatusMessage(message)
	{
		if (!this.StatusCaption[message.status])
		{
			error("Unrecognized netstatus type '" + message.status + "'");
			return;
		}

		this.netStatus.caption = this.StatusCaption[message.status](message);
		this.netStatus.hidden = message.status == "active";
		this.loadingClientsText.hidden = message.status != "waiting_for_players";
	}

	onClientsLoadingMessage(guids)
	{
		this.loadingClientsText.caption = guids.map(guid => colorizePlayernameByGUID(guid)).join(this.Comma);
	}
}

NetworkStatusOverlay.prototype.StatusCaption = {
	"authenticated": msg => translate("Connection to the server has been authenticated."),
	"connected": msg => translate("Connected to the server."),
	"disconnected": msg => translate("Connection to the server has been lost.") + "\n" +
		getDisconnectReason(msg.reason, true),
	"waiting_for_players": msg => translate("Waiting for players to connect:"),
	"join_syncing": msg => translate("Synchronizing gameplay with other players…"),
	"active": msg => ""
};

NetworkStatusOverlay.prototype.Comma = translateWithContext("Separator for a list of client loading messages", ", ");
