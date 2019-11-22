/**
 * This class informs clients of the server if an announcement had been broadcasted.
 */
class AnnouncementHandler
{
	constructor(xmppMessages)
	{
		xmppMessages.registerXmppMessageHandler("chat", "private-message", this.onPrivateMessage.bind(this));
	}

	onPrivateMessage(message)
	{
		// Announcements and the Message of the Day are sent by the server directly
		if (!message.from)
			messageBox(
				400, 250,
				message.text.trim(),
				translate("Notice"));
	}
}
