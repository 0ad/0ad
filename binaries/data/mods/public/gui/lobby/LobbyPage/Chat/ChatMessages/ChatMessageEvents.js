/**
 * This is the class (and only class) that formats textual messages submitted by chat participants.
 */
ChatMessageEvents.PlayerChat = class
{
	constructor(xmppMessages, chatMessagesPanel)
	{
		this.chatMessagesPanel = chatMessagesPanel;
		this.chatMessageFormat = new ChatMessageFormat();
		xmppMessages.registerXmppMessageHandler("chat", "room-message", this.onRoomMessage.bind(this));
		xmppMessages.registerXmppMessageHandler("chat", "private-message", this.onPrivateMessage.bind(this));
		xmppMessages.registerXmppMessageHandler("chat", "headline", this.onAnnouncementMessage.bind(this));
	}

	onRoomMessage(message)
	{
		this.chatMessagesPanel.addText(message.time, this.chatMessageFormat.format(message));
	}

	onPrivateMessage(message)
	{
		// We intend to not support private messages between users
		if (Engine.LobbyGetPlayerRole(message.from) === "moderator")
			// some XMPP clients send trailing whitespace
			this.chatMessagesPanel.addText(message.time, this.chatMessageFormat.format(message));
	}

	onAnnouncementMessage(message)
	{
		if (message.subject.trim().length > 0 || message.text.trim().length > 0)
			this.chatMessagesPanel.addText(message.time, this.chatMessageFormat.format(message));
	}
};
