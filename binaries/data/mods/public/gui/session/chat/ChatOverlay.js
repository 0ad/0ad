/**
 * This class is concerned with displaying the most recent chat messages on a screen overlay for some seconds.
 */
class ChatOverlay
{
	constructor()
	{
		/**
		 * Maximum number of lines to display simultaneously.
		 */
		this.chatLines = 20;

		/**
		 * Number of seconds after which chatmessages will disappear.
		 */
		this.chatTimeout = 30;

		/**
		 * Holds the timer-IDs used for hiding the chat after chatTimeout seconds.
		 */
		this.chatTimers = [];

		/**
		 * The currently displayed strings, limited by the given timeframe and limit above.
		 */
		this.chatMessages = [];

		this.chatText = Engine.GetGUIObjectByName("chatText");
	}

	/**
	 * Displays this message in the chat overlay and sets up the timer to remove it after a while.
	 */
	onChatMessage(msg, chatMessage)
	{
		this.chatMessages.push(chatMessage);
		this.chatTimers.push(setTimeout(this.removeOldChatMessage.bind(this), this.chatTimeout * 1000));

		if (this.chatMessages.length > this.chatLines)
			this.removeOldChatMessage();
		else
			this.chatText.caption = this.chatMessages.join("\n");
	}

	/**
	 * Empty all messages currently displayed in the chat overlay.
	 */
	clearChatMessages()
	{
		this.chatMessages = [];
		this.chatText.caption = "";

		for (let timer of this.chatTimers)
			clearTimeout(timer);

		this.chatTimers = [];
	}

	/**
	 * Called when the timer has run out for the oldest chatmessage or when the message limit is reached.
	 */
	removeOldChatMessage()
	{
		clearTimeout(this.chatTimers[0]);
		this.chatTimers.shift();
		this.chatMessages.shift();
		this.chatText.caption = this.chatMessages.join("\n");
	}
}
