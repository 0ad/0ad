/**
 * Status messages are textual event notifications triggered by local events.
 * The messages may be colorized, hence the caller needs to apply escapeText on player input.
 */
class SystemMessageFormat
{
	constructor()
	{
		this.args = {
			"system": setStringTags(this.System, this.SystemTags)
		};
	}

	format(text)
	{
		this.args.message = text;
		return setStringTags(
			sprintf(this.MessageFormat, this.args),
			this.MessageTags);
	}
}

SystemMessageFormat.prototype.System =
	// Translation: Caption for system notifications shown in the chat panel
	translate("System:");

SystemMessageFormat.prototype.SystemTags = {
	"color": "150 0 0"
};

SystemMessageFormat.prototype.MessageFormat =
	translate("=== %(system)s %(message)s");

SystemMessageFormat.prototype.MessageTags = {
	"font": "sans-bold-13"
};
