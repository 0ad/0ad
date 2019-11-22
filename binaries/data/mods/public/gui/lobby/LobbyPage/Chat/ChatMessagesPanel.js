/**
 * This class stores and displays the chat history since the login and
 * displays timestamps if enabled.
 */
class ChatMessagesPanel
{
	constructor(xmppMessages)
	{
		this.xmppMessages = xmppMessages;

		this.chatText = Engine.GetGUIObjectByName("chatText");
		this.chatHistory = "";

		if (Engine.ConfigDB_GetValue("user", "chat.timestamp") == "true")
			this.timestampWrapper = new TimestampWrapper();

		this.hasUpdate = false;
		this.flushEvent = this.flushMessages.bind(this);
	}

	addText(timestamp, text)
	{
		if (this.timestampWrapper)
			text = this.timestampWrapper.format(timestamp, text);

		this.chatHistory += this.chatHistory ? "\n" + text : text;

		if (!this.hasUpdate)
		{
			this.hasUpdate = true;
			// Most xmpp messages are not chat messages, hence
			// only subscribe the event handler when relevant to improve performance.
			this.xmppMessages.registerMessageBatchProcessedHandler(this.flushEvent);
		}
	}

	flushMessages()
	{
		if (this.hasUpdate)
		{
			this.chatText.caption = this.chatHistory;
			this.hasUpdate = false;
			this.xmppMessages.unregisterMessageBatchProcessedHandler(this.flushEvent);
		}
	}

	clearChatMessages()
	{
		this.chatHistory = "";
		this.chatText.caption = "";
	}
}
