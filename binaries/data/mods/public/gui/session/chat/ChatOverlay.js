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
		this.chatLinesNumber = 20;

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
		this.chatLines = Engine.GetGUIObjectByName("chatLines").children;
		this.chatLinesNumber = Math.min(this.chatLinesNumber, this.chatLines.length);
	}

	displayChatMessages()
	{
		for (let i = 0; i < this.chatLinesNumber; ++i)
		{
			let chatMessage = this.chatMessages[i];
			if (chatMessage && chatMessage.text)
			{
				// First scale line width to maximum size.
				let lineSize = this.chatLines[i].size;
				let height = lineSize.bottom - lineSize.top;
				lineSize.top = i * height;
				lineSize.bottom = lineSize.top + height;
				lineSize.rright = 100;
				this.chatLines[i].size = lineSize;

				this.chatLines[i].caption = chatMessage.text;

				// Now read the actual text width and scale the line width accordingly.
				lineSize.rright = 0;
				lineSize.right = lineSize.left + this.chatLines[i].getTextSize().width;
				this.chatLines[i].size = lineSize;

				if (chatMessage.callback)
					this.chatLines[i].onPress = chatMessage.callback;

				if (chatMessage.tooltip)
					this.chatLines[i].tooltip = chatMessage.tooltip;
			}
			this.chatLines[i].hidden = !chatMessage || !chatMessage.text;
			this.chatLines[i].ghost = !chatMessage || !chatMessage.callback;
		}
	}

	/**
	 * Displays this message in the chat overlay and sets up the timer to remove it after a while.
	 */
	onChatMessage(msg, chatMessage)
	{
		this.chatMessages.push(chatMessage);
		this.chatTimers.push(setTimeout(this.removeOldChatMessage.bind(this), this.chatTimeout * 1000));

		if (this.chatMessages.length > this.chatLinesNumber)
			this.removeOldChatMessage();
		else
			this.displayChatMessages();
	}

	/**
	 * Empty all messages currently displayed in the chat overlay.
	 */
	clearChatMessages()
	{
		this.chatMessages = [];
		this.displayChatMessages();

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
		this.displayChatMessages();
	}
}
