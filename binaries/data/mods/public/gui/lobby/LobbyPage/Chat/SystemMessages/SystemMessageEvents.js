/**
 * System messages are highlighted chat notifications that concern the current player.
 */
ChatMessageEvents.System = class
{
	constructor(xmppMessages, chatMessagesPanel, statusMessageFormat, systemMessageFormat)
	{
		this.chatMessagesPanel = chatMessagesPanel;
		this.systemMessageFormat = systemMessageFormat;
		this.kickStrings = new KickStrings();

		xmppMessages.registerXmppMessageHandler("system", "connected", this.onConnected.bind(this));
		xmppMessages.registerXmppMessageHandler("system", "disconnected", this.onDisconnected.bind(this));
		xmppMessages.registerXmppMessageHandler("system", "error", this.onSystemError.bind(this));
		xmppMessages.registerXmppMessageHandler("chat", "kicked", this.onClientKicked.bind(this, false));
		xmppMessages.registerXmppMessageHandler("chat", "banned", this.onClientKicked.bind(this, true));
	}

	// TODO: XmppClient StanzaErrorServiceUnavailable is thrown if the ratings bot is not serving
	// This should be caught more transparently than an unrelatable "Service unavailable" system error chat message
	onSystemError(message)
	{
		this.chatMessagesPanel.addText(
			message.time,
			this.systemMessageFormat.format(
				escapeText(message.text)));
	}

	onConnected(message)
	{
		this.chatMessagesPanel.addText(
			message.time,
			this.systemMessageFormat.format(this.ConnectedCaption));
	}

	onDisconnected(message)
	{
		this.chatMessagesPanel.addText(
			message.time,
			this.systemMessageFormat.format(
				this.DisconnectedCaption + " " +
				escapeText(message.reason + " " + message.certificate_status)));
	}

	onClientKicked(banned, message)
	{
		if (message.nick == g_Nickname)
			this.chatMessagesPanel.addText(
				message.time,
				this.systemMessageFormat.format(
					this.kickStrings.get(banned, message)));
	}
};

ChatMessageEvents.System.prototype.ConnectedCaption = translate("Connected.");
ChatMessageEvents.System.prototype.DisconnectedCaption = translate("Disconnected.");
