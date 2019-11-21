/**
 * This class adds an indication that the chat message was a private message to the given text.
 */
class ChatMessagePrivateWrapper
{
	constructor()
	{
		this.args = {
			"private": setStringTags(this.PrivateFormat, this.PrivateMessageTags)
		};
	}

	/**
	 * Text is formatted, escapeText is the responsibility of the caller.
	 */
	format(text)
	{
		this.args.message = text;
		return sprintf(this.PrivateMessageFormat, this.args);
	}
}

ChatMessagePrivateWrapper.prototype.PrivateFormat = translate("Private");

ChatMessagePrivateWrapper.prototype.PrivateMessageFormat = translate("(%(private)s) %(message)s");

/**
 * Color for private messages in the chat.
 */
ChatMessagePrivateWrapper.prototype.PrivateMessageTags = {
	"color": "0 150 0"
};
