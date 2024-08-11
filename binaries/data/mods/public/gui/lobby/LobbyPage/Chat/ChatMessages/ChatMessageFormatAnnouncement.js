/**
 * This class adds an indication that the chat message was an announcement to the given text.
 */
class ChatMessageFormatAnnouncement
{
	constructor()
	{
		this.args = {
			"prefix": setStringTags(this.AnnouncementPrefix, this.AnnouncementPrefixTags)
		};
	}

	/**
	 * Text is formatted, escapeText is the responsibility of the caller.
	 */
	format(subject, text)
	{
		var message = formatXmppAnnouncement(subject, text);
		this.args.message = setStringTags(message, this.AnnouncementMessageTags);
		return sprintf(this.AnnouncementMessageFormat, this.args);
	}
}

ChatMessageFormatAnnouncement.prototype.AnnouncementPrefix = translate("Notice");

ChatMessageFormatAnnouncement.prototype.AnnouncementMessageFormat = translate("== %(prefix)s:\n%(message)s");

/**
 * Color for announcements in the chat.
 */
ChatMessageFormatAnnouncement.prototype.AnnouncementPrefixTags = {
	"font": "sans-bold-13"
};

ChatMessageFormatAnnouncement.prototype.AnnouncementMessageTags = {
	"font": "sans-bold-13"
};
