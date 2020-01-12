/**
 * This class wraps a string with a timestamp dating to when the message was sent.
 */
class TimestampWrapper
{
	constructor()
	{
		this.timeArgs = {};
		this.timestampArgs = {};
	}

	format(text)
	{
		this.timeArgs.time =
			Engine.FormatMillisecondsIntoDateStringLocal(Date.now(), this.TimeFormat);

		this.timestampArgs.time = sprintf(this.TimestampFormat, this.timeArgs);
		this.timestampArgs.message = text;

		return sprintf(this.TimestampedMessageFormat, this.timestampArgs);
	}
}

TimestampWrapper.prototype.TimestampedMessageFormat =
	translate("%(time)s %(message)s");

TimestampWrapper.prototype.TimestampFormat =
	translate("\\[%(time)s]");

TimestampWrapper.prototype.TimeFormat =
	translate("HH:mm");
