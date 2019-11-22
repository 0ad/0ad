/**
 * This class formats a chat message that was not formatted with any commands.
 * The nickname and the message content will be assumed to be player input, thus escaped,
 * meaning that one cannot use colorized messages here.
 */
class ChatMessageFormatSay
{
	constructor()
	{
		this.senderArgs = {};
		this.messageArgs = {};
	}

	/**
	 * Sender is formatted, escapeText is the responsibility of the caller.
	 */
	format(sender, text)
	{
		this.senderArgs.sender = sender;
		this.messageArgs.message = text;
		this.messageArgs.sender = setStringTags(
			sprintf(this.ChatSenderFormat, this.senderArgs),
			this.SenderTags);

		return sprintf(this.ChatMessageFormat, this.messageArgs);
	}
}

ChatMessageFormatSay.prototype.ChatSenderFormat = translate("<%(sender)s>");

ChatMessageFormatSay.prototype.ChatMessageFormat = translate("%(sender)s %(message)s");

/**
 * Used for highlighting the sender of chat messages.
 */
ChatMessageFormatSay.prototype.SenderTags = {
	"font": "sans-bold-13"
};
