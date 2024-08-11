/**
 * This class informs clients of the server if an announcement had been broadcasted.
 */
class AnnouncementHandler
{
	constructor(xmppMessages)
	{
		xmppMessages.registerXmppMessageHandler("chat", "headline", this.onAnnouncementMessage.bind(this));
	}

	onAnnouncementMessage(message)
	{
		if (message.subject.trim().length === 0 && message.text.trim().length === 0)
			return;

		messageBox(
			400, 250,
			formatXmppAnnouncement(message.subject, message.text),
			translate("Notice"));
	}
}
