/**
 * This class will ask the player to rejoin the lobby after having been disconnected.
 */
class ConnectionHandler
{
	constructor(xmppMessages)
	{
		// Whether the current player has been kicked or banned
		this.kicked = false;

		// Avoid stacking of multiple dialog boxes
		this.askingReconnect = false;

		xmppMessages.registerXmppMessageHandler("chat", "leave", this.onClientLeave.bind(this));
		xmppMessages.registerXmppMessageHandler("chat", "kicked", this.onClientKicked.bind(this, false));
		xmppMessages.registerXmppMessageHandler("system", "disconnected", this.askReconnect.bind(this));
		xmppMessages.registerXmppMessageHandler("chat", "nick", this.onNickChange.bind(this));
	}

	onNickChange(message)
	{
		if (message.oldnick == g_Nickname)
			g_Nickname = message.newnick;
	}

	onClientLeave(message)
	{
		if (message.nick == g_Nickname)
			Engine.DisconnectXmppClient();
	}

	onClientKicked(banned, message)
	{
		if (message.nick != g_Nickname)
			return;

		this.kicked = true;

		// The current player has been kicked from the room, not from the server
		Engine.DisconnectXmppClient();

		messageBox(
			400, 250,
			new KickStrings().get(banned, message),
			banned ? translate("BANNED") : translate("KICKED"));
	}

	askReconnect()
	{
		if (this.kicked)
			return;

		// Ignore stacked disconnect messages
		if (Engine.IsXmppClientConnected() || this.askingReconnect)
			return;

		this.askingReconnect = true;

		messageBox(
			400, 200,
			translate("You have been disconnected from the lobby. Do you want to reconnect?"),
			translate("Confirmation"),
			[translate("No"), translate("Yes")],
			[
				() => {
					this.askingReconnect = false;
				},
				() => {
					this.askingReconnect = false;
					Engine.ConnectXmppClient();
				}
			]);
	}
}
