/**
 * Status messages are textual event notifications triggered by multi-user chat room actions.
 */
class StatusMessageFormat
{
	constructor()
	{
		this.args = {};
	}

	/**
	 * escapeText is the responsibility of the caller.
	 */
	format(text)
	{
		this.args.message = text;
		return setStringTags(
			sprintf(this.MessageFormat, this.args),
			this.MessageTags);
	}
}

StatusMessageFormat.prototype.MessageFormat =
	// Translation: Chat status message
	translate("== %(message)s");

StatusMessageFormat.prototype.MessageTags = {
	"font": "sans-bold-13"
};
