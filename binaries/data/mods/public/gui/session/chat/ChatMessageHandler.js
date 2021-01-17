/**
 * The purpose of this class is to run a given chat message through parsers until
 * one of them succeeds and then call all callback handlers on the result.
 */
class ChatMessageHandler
{
	constructor()
	{
		/**
		 * Each property is an array of messageformat class instances.
		 * The classes must have a parse function that receives a
		 * msg object and translates into a string.
		 */
		this.messageFormats = {};

		/**
		 * Functions that are called each time a message was parsed.
		 */
		this.messageHandlers = [];

		this.registerMessageFormat("system", new ChatMessageHandler.System());
	}

	/**
	 * @param type - a string denoting the messagetype used by addChatMessage calls.
	 * @param handler - a class instance with a parse function.
	 */
	registerMessageFormat(type, handler)
	{
		if (!this.messageFormats[type])
			this.messageFormats[type] = [];

		this.messageFormats[type].push(handler);
	}

	/**
	 * Receives a class where each enumerable owned property is a chat format
	 * class identified by the property name.
	 */
	registerMessageFormatClass(formatClass)
	{
		for (let type in formatClass)
			this.registerMessageFormat(type, new formatClass[type]());
	}

	registerMessageHandler(handler)
	{
		this.messageHandlers.push(handler);
	}

	handleMessage(msg)
	{
		let formatted = this.parseMessage(msg);
		if (!formatted)
			return;

		for (let handler of this.messageHandlers)
			handler(msg, formatted);
	}

	parseMessage(msg)
	{
		if (!this.messageFormats[msg.type])
		{
			error("Unknown chat message type: " + uneval(msg));
			return undefined;
		}

		for (let messageFormat of this.messageFormats[msg.type])
		{
			let formatted = messageFormat.parse(msg);
			if (formatted && formatted.text)
				return formatted;
		}

		return undefined;
	}
}

ChatMessageHandler.System = class
{
	parse(msg)
	{
		return msg.txt;
	}
};
