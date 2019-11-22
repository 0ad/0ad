/**
 * This class formats a chat message that was sent using the /me format command.
 * For example "/me goes away".
 */
class ChatMessageFormatMe
{
	constructor()
	{
		this.args = {};
	}

	/**
	 * Sender is formatted, escapeText is the responsibility of the caller.
	 */
	format(sender, text)
	{
		this.args.sender = setStringTags(sender, this.SenderTags);
		this.args.message = text;
		return sprintf(this.Format, this.args);
	}
}

// Translation: Chat message issued using the ‘/me’ command.
ChatMessageFormatMe.prototype.Format = translate("* %(sender)s %(message)s");

/**
 * Used for highlighting the sender of chat messages.
 */
ChatMessageFormatMe.prototype.SenderTags = {
	"font": "sans-bold-13"
};
